#include "image_gphoto.h"

#include <unistd.h>
#include <fcntl.h>

#include "log.h"
#include "timer.h"
#include "threadpool.h"

using namespace std;

namespace Splash
{

/*************/
Image_GPhoto::Image_GPhoto()
{
    _type = "image_gphoto";

    registerAttributes();
    init();
    detectCameras();

    read("");
}

/*************/
Image_GPhoto::~Image_GPhoto()
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    for (auto& camera : _cameras)
        releaseCamera(camera);

#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Image_GPhoto::~Image_GPhoto - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_GPhoto::read(const string& filename)
{
    // If filename is empty, we connect to the first available camera
    lock_guard<recursive_mutex> lock(_gpMutex);    

    if (_cameras.size() == 0)
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Cannot find any camera to read from" << Log::endl;
        return false;
    }

    // TODO: handle selection by camera name
    _selectedCameraIndex = 0;

    return true;
}

/*************/
void Image_GPhoto::detectCameras()
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_gpPorts != nullptr)
        gp_port_info_list_free(_gpPorts);
    gp_port_info_list_new(&_gpPorts);
    gp_port_info_list_load(_gpPorts);

    CameraList* availableCameras = nullptr;
    gp_list_new(&availableCameras);
    gp_abilities_list_detect(_gpCams, _gpPorts, availableCameras, _gpContext);

    SLog::log << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - " << (gp_list_count(availableCameras) > 0 ? gp_list_count(availableCameras) : 0) << " cameras detected" << Log::endl;

    // Create the list of tetherable cameras
    for (int i = 0; i < gp_list_count(availableCameras); ++i)
    {
        GPhotoCamera camera;
        const char* s;
        gp_list_get_name(availableCameras, i, &s);
        camera.model = string(s);
        gp_list_get_value(availableCameras, i, &s);
        camera.port = string(s);

        if (!initCamera(camera))
        {
            SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Unable to initialize camera " << camera.model << " on port " << camera.port << Log::endl;
            releaseCamera(camera);
            continue;
        }

        if (!camera.canTether && !camera.canImport)
        {
            SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " on port " << camera.port << " does not support import or tethering" << Log::endl;
            releaseCamera(camera);
            continue;
        }

        SLog::log << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " on port " << camera.port << " initialized correctly" << Log::endl;
        _cameras.push_back(camera);
    }
}

/*************/
void Image_GPhoto::doCapture()
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_selectedCameraIndex == -1)
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - A camera must be selected before trying to capture" << Log::endl;
        return;
    }

    GPhotoCamera camera = _cameras[_selectedCameraIndex];

    CameraFilePath filePath;
    int res = GP_OK;
    if ((res = gp_camera_capture(camera.cam, GP_CAPTURE_IMAGE, &filePath, _gpContext)) == GP_OK)
    {
        CameraFile* destination;
        int handle = open((string("/tmp/") + string(filePath.name)).c_str(), O_CREAT | O_WRONLY, 0666);
        if (handle != -1)
        {
            gp_file_new_from_fd(&destination, handle);
            if (gp_camera_file_get(camera.cam, filePath.folder, filePath.name, GP_FILE_TYPE_NORMAL, destination, _gpContext) == GP_OK)
                SLog::log << Log::DEBUGGING << "Image_GPhoto::" << __FUNCTION__ << " - Sucessfully downloaded file " << string(filePath.folder) << "/" << string(filePath.name) << Log::endl;
            close(handle);
        }
    }

    // Read the downloaded file
    readFile(string("/tmp/") + string(filePath.name));
}

/*************/
bool Image_GPhoto::doSetProperty(string name, string value)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_selectedCameraIndex == -1)
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - A camera must be selected before trying to capture" << Log::endl;
        return false;
    }

    GPhotoCamera* camera = &(_cameras[_selectedCameraIndex]);

    CameraWidget* config;
    CameraWidget* widget;
    gp_camera_get_config(camera->cam, &config, _gpContext);
    if (gp_widget_get_child_by_name(config, name.c_str(), &widget) == GP_OK)
    {
        gp_widget_set_value(widget, value.c_str());
        gp_camera_set_config(camera->cam, config, _gpContext);
        return true;
    }

    return false;
}

/*************/
bool Image_GPhoto::doGetProperty(string name, string& value)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_selectedCameraIndex == -1)
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - A camera must be selected before trying to capture" << Log::endl;
        return false;
    }

    GPhotoCamera* camera = &(_cameras[_selectedCameraIndex]);

    CameraWidget* config;
    CameraWidget* widget;
    gp_camera_get_config(camera->cam, &config, _gpContext);
    if (gp_widget_get_child_by_name(config, name.c_str(), &widget) == GP_OK)
    {
        const char* cvalue = nullptr;
        gp_widget_get_value(widget, &cvalue);
        value = string(cvalue);
        return true;
    }

    return false;
}

/*************/
void Image_GPhoto::init()
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    _gpContext = gp_context_new();
    gp_abilities_list_new(&_gpCams);
    gp_abilities_list_load(_gpCams, _gpContext);

    SLog::log << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - Loaded " << gp_abilities_list_count(_gpCams) << " camera drivers" << Log::endl;
}

/*************/
bool Image_GPhoto::initCamera(GPhotoCamera& camera)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (camera.cam == nullptr)
    {
        gp_camera_new(&camera.cam);

        int m = gp_abilities_list_lookup_model(_gpCams, camera.model.c_str());
        CameraAbilities abilities;
        gp_abilities_list_get_abilities(_gpCams, m, &abilities);
        gp_camera_set_abilities(camera.cam, abilities);

        int p = gp_port_info_list_lookup_path(_gpPorts, camera.port.c_str());
        GPPortInfo portInfo;
        gp_port_info_list_get_info(_gpPorts, p, &portInfo);
        gp_camera_set_port_info(camera.cam, portInfo);

        if (abilities.operations & GP_OPERATION_CAPTURE_IMAGE)
        {
            camera.canTether = true;
            if (abilities.operations & GP_OPERATION_CONFIG)
                camera.canConfig = true;
        }
        if (!(abilities.file_operations & GP_FILE_OPERATION_NONE))
            camera.canImport = true;

        gp_camera_get_config(camera.cam, &camera.configuration, _gpContext);

        return true;
    }
    else
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " already initialized" << Log::endl;
        return false;
    }
}

/*************/
void Image_GPhoto::releaseCamera(GPhotoCamera& camera)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    gp_camera_exit(camera.cam, _gpContext);
    gp_camera_unref(camera.cam);
    gp_widget_unref(camera.configuration);
}

/*************/
void Image_GPhoto::registerAttributes()
{
    _attribFunctions["srgb"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        _srgb = (args[0].asInt() > 0) ? true : false;
        return true;
    }, [&]() -> Values {
        return {_srgb};
    });

    _attribFunctions["shutterspeed"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;
        return doSetProperty("shutterspeed", args[0].asString());
    }, [&]() -> Values {
        string value;
        doGetProperty("shutterspeed", value);
        return {value};
    });

    // Actions
    _attribFunctions["capture"] = AttributeFunctor([&](Values args) {
        SThread::pool.enqueue([&]() {
            doCapture();
        });
        return true;
    });

    _attribFunctions["detect"] = AttributeFunctor([&](Values args) {
        SThread::pool.enqueue([&]() {
            detectCameras();
        });
        return true;
    });
}

} // end of namespace
