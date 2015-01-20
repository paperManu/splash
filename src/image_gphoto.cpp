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
    initCamera(_cameras[_selectedCameraIndex]);

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
            releaseCamera(camera);
            SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Unable to initialize camera " << camera.model << " on port " << camera.port << Log::endl;
        }
        else if (!camera.canTether && !camera.canImport)
        {
            releaseCamera(camera);
            SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " on port " << camera.port << " does not support import or tethering" << Log::endl;
        }
        else
        {
            releaseCamera(camera);
            _cameras.push_back(camera);
            SLog::log << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " on port " << camera.port << " initialized correctly" << Log::endl;
        }
    }
}

/*************/
void Image_GPhoto::capture()
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
        gp_camera_file_delete(camera.cam, filePath.folder, filePath.name, _gpContext);
    }

    // Read the downloaded file
    readFile(string("/tmp/") + string(filePath.name));

    // Delete the file
    remove((string("/tmp/") + string(filePath.name)).c_str());
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

    CameraWidget* widget;
    if (gp_widget_get_child_by_name(camera->configuration, name.c_str(), &widget) == GP_OK)
    {
        gp_widget_set_value(widget, value.c_str());
        gp_camera_set_config(camera->cam, camera->configuration, _gpContext);
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

    CameraWidget* widget;
    if (gp_widget_get_child_by_name(camera->configuration, name.c_str(), &widget) == GP_OK)
    {
        const char* cvalue = nullptr;
        gp_widget_get_value(widget, &cvalue);
        value = string(cvalue);
        return true;
    }

    return false;
}

/*************/
float Image_GPhoto::getFloatFromShutterspeedString(std::string speed)
{
    float num = 1.f;
    float denom = 1.f;
    if (speed.find("1/") != string::npos)
        denom = stof(speed.substr(2));
    else
    {
        try
        {
            num = stof(speed);
        }
        catch (std::invalid_argument)
        {
            num = 1.f;
        }
    }
    return num / denom;
}

/*************/
string Image_GPhoto::getShutterspeedStringFromFloat(float duration)
{
    float diff = numeric_limits<float>::max();
    string selectedSpeed = "1/20"; // If not correct value has been found, return this arbitrary value

    for (auto& speed : _cameras[_selectedCameraIndex].shutterspeeds)
    {
        float value = getFloatFromShutterspeedString(speed);
        if (std::abs(duration - value) < diff)
        {
            diff = std::abs(duration - value);
            selectedSpeed = speed;
        }
    }

    return selectedSpeed;
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

        if (gp_camera_init(camera.cam, _gpContext) != GP_OK)
            return false;

        gp_camera_get_config(camera.cam, &camera.configuration, _gpContext);

        // Get the available shutterspeeds
        initCameraProperty(camera, "shutterspeed", camera.shutterspeeds);
        initCameraProperty(camera, "aperture", camera.apertures);
        initCameraProperty(camera, "iso", camera.isos);

        return true;
    }
    else
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " already initialized" << Log::endl;
        return false;
    }
}

/*************/
void Image_GPhoto::initCameraProperty(GPhotoCamera& camera, string property, vector<string>& values)
{
    values.clear();
    CameraWidget* widget;
    if (gp_widget_get_child_by_name(camera.configuration, property.c_str(), &widget) == GP_OK)
    {
        const char* value = nullptr;
        int propCount = gp_widget_count_choices(widget);
        for (int i = 0; i < propCount; ++i)
        {
            gp_widget_get_choice(widget, i, &value);
            values.push_back({value});
        }
    }
    else
    {
        SLog::log << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Property " << property << " is not available for camera " << camera.model << Log::endl;
    }
}

/*************/
void Image_GPhoto::releaseCamera(GPhotoCamera& camera)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    gp_camera_exit(camera.cam, _gpContext);
    if (camera.cam != nullptr)
        gp_camera_unref(camera.cam);
    if (camera.configuration != nullptr)
        gp_widget_unref(camera.configuration);

    camera.cam = nullptr;
    camera.configuration = nullptr;
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

    _attribFunctions["aperture"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;
        return doSetProperty("aperture", args[0].asString());
    }, [&]() -> Values {
        string value;
        if (doGetProperty("aperture", value))
            return {value};
        else
            return {};
    });

    _attribFunctions["isospeed"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;
        return doSetProperty("iso", args[0].asString());
    }, [&]() -> Values {
        string value;
        if (doGetProperty("iso", value))
            return {value};
        else
            return {};
    });
    _attribFunctions["shutterspeed"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;
        return doSetProperty("shutterspeed", getShutterspeedStringFromFloat(args[0].asFloat()));
    }, [&]() -> Values {
        string value;
        doGetProperty("shutterspeed", value);
        float duration = getFloatFromShutterspeedString(value);
        return {duration};
    });

    // Actions
    _attribFunctions["capture"] = AttributeFunctor([&](Values args) {
        SThread::pool.enqueue([&]() {
            capture();
        });
        return true;
    });

    _attribFunctions["detect"] = AttributeFunctor([&](Values args) {
        SThread::pool.enqueue([&]() {
            detectCameras();
        });
        return true;
    });

    // Status
    _attribFunctions["ready"] = AttributeFunctor([&](Values args) {
        return false;
    }, [&]() -> Values {
        lock_guard<recursive_mutex> lock(_gpMutex);
        if (_selectedCameraIndex == -1)
            return {0};
        else
            return {1};
    });
}

} // end of namespace
