#include "./image/image_gphoto.h"

#include <cmath>
#include <fcntl.h>
#include <unistd.h>

#include "./utils/log.h"
#include "./utils/timer.h"

using namespace std;

namespace Splash
{

/*************/
Image_GPhoto::Image_GPhoto(RootObject* root, const std::string& cameraName)
    : Image(root)
{
    init();
    read(cameraName);
}

/*************/
Image_GPhoto::~Image_GPhoto()
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    for (auto& camera : _cameras)
        releaseCamera(camera);

    gp_context_unref(_gpContext);
    gp_port_info_list_free(_gpPorts);
    gp_abilities_list_free(_gpCams);

#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Image_GPhoto::~Image_GPhoto - Destructor" << Log::endl;
#endif
}

/*************/
bool Image_GPhoto::read(const string& cameraName)
{
    // If filename is empty, we connect to the first available camera
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_cameras.size() == 0)
    {
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Cannot find any camera to read from" << Log::endl;
        return false;
    }

    if (cameraName == "")
    {
        // Default behavior: select the first available camera
        _selectedCameraIndex = 0;
        initCamera(_cameras[_selectedCameraIndex]);
        return true;
    }
    else
    {
        for (unsigned int i = 0; i < _cameras.size(); ++i)
        {
            GPhotoCamera& camera = _cameras[i];

            if (camera.model == cameraName)
            {
                _selectedCameraIndex = i;
                initCamera(_cameras[_selectedCameraIndex]);
                return true;
            }
        }
    }

    return false;
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

    Log::get() << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - " << (gp_list_count(availableCameras) > 0 ? gp_list_count(availableCameras) : 0) << " cameras detected"
               << Log::endl;

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
            Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Unable to initialize camera " << camera.model << " on port " << camera.port << Log::endl;
        }
        else if (!camera.canTether && !camera.canImport)
        {
            releaseCamera(camera);
            Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " on port " << camera.port << " does not support import or tethering"
                       << Log::endl;
        }
        else
        {
            releaseCamera(camera);
            _cameras.push_back(camera);
            Log::get() << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " on port " << camera.port << " initialized correctly" << Log::endl;
        }
    }
}

/*************/
bool Image_GPhoto::capture()
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_selectedCameraIndex == -1)
    {
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - A camera must be selected before trying to capture" << Log::endl;
        return false;
    }

    GPhotoCamera& camera = _cameras[_selectedCameraIndex];

    CameraFilePath filePath{};
    int res;
    if ((res = gp_camera_capture(camera.cam, GP_CAPTURE_IMAGE, &filePath, _gpContext)) == GP_OK)
    {
        if (string(filePath.name).find(".jpg") != string::npos || string(filePath.name).find(".JPG") != string::npos)
        {
            CameraFile* destination;
            int handle = open((string("/tmp/") + string(filePath.name)).c_str(), O_CREAT | O_WRONLY, 0666);
            if (handle != -1)
            {
                gp_file_new_from_fd(&destination, handle);
                if (!gp_camera_file_get(camera.cam, filePath.folder, filePath.name, GP_FILE_TYPE_NORMAL, destination, _gpContext) == GP_OK)
                {
                    Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Unable to download file " << string(filePath.folder) << "/" << string(filePath.name)
                               << Log::endl;
                }
#ifdef DEBUG
                else
                {
                    Log::get() << Log::DEBUGGING << "Image_GPhoto::" << __FUNCTION__ << " - Sucessfully downloaded file " << string(filePath.folder) << "/" << string(filePath.name)
                               << Log::endl;
                }
#endif
                close(handle);
            }

            // Read the downloaded file
            readFile(string("/tmp/") + string(filePath.name));
        }
        else
        {
            Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Captured image filetype is not jpeg. Maybe the camera is set to RAW?" << Log::endl;
            res = GP_ERROR;
        }

        gp_camera_file_delete(camera.cam, filePath.folder, filePath.name, _gpContext);

        // Delete the file
        if (remove((string("/tmp/") + string(filePath.name)).c_str()) == -1)
            Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Unable to delete file /tmp/" << filePath.name << Log::endl;
    }

    this_thread::sleep_for(chrono::milliseconds(1000));

    if (res != GP_OK)
        return false;
    else
        return true;
}

/*************/
bool Image_GPhoto::doSetProperty(const string& name, const string& value)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_selectedCameraIndex == -1)
    {
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - A camera must be selected before trying to capture" << Log::endl;
        return false;
    }

    GPhotoCamera& camera = _cameras[_selectedCameraIndex];
    CameraWidget* cameraConfig;
    gp_camera_get_config(camera.cam, &cameraConfig, _gpContext);
    OnScopeExit { gp_widget_free(cameraConfig); };

    CameraWidget* widget;
    if (gp_widget_get_child_by_name(cameraConfig, name.c_str(), &widget) == GP_OK)
    {
        if (gp_widget_set_value(widget, value.c_str()) != GP_OK)
        {
            Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Unable to set parameter " << name << " to value " << value << Log::endl;
            return false;
        }

        if (gp_camera_set_config(camera.cam, cameraConfig, _gpContext) != GP_OK)
        {
            Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Setting parameter " << name << " is not supported for this camera" << Log::endl;
            return false;
        }

        Log::get() << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - Parameter " << name << " set to " << value << Log::endl;
        return true;
    }
    else
    {
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Parameter " << name << " does not seem to be available" << Log::endl;
    }

    return false;
}

/*************/
bool Image_GPhoto::doGetProperty(const string& name, string& value)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (_selectedCameraIndex == -1)
    {
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - A camera must be selected before trying to capture" << Log::endl;
        return false;
    }

    GPhotoCamera& camera = _cameras[_selectedCameraIndex];
    CameraWidget* cameraConfig;
    gp_camera_get_config(camera.cam, &cameraConfig, _gpContext);
    OnScopeExit { gp_widget_free(cameraConfig); };

    CameraWidget* widget;
    if (gp_widget_get_child_by_name(cameraConfig, name.c_str(), &widget) == GP_OK)
    {
        const char* cvalue = nullptr;
        gp_widget_get_value(widget, &cvalue);
        value = string(cvalue);
        return true;
    }

    return false;
}

/*************/
float Image_GPhoto::getFloatFromShutterspeedString(const string& speed)
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
        catch (std::invalid_argument&)
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
        if (speed == "Bulb")
            continue;
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
    _type = "image_gphoto";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    lock_guard<recursive_mutex> lock(_gpMutex);

    _gpContext = gp_context_new();
    gp_abilities_list_new(&_gpCams);
    gp_abilities_list_load(_gpCams, _gpContext);

    Log::get() << Log::MESSAGE << "Image_GPhoto::" << __FUNCTION__ << " - Loaded " << gp_abilities_list_count(_gpCams) << " camera drivers" << Log::endl;

    detectCameras();
}

/*************/
bool Image_GPhoto::initCamera(GPhotoCamera& camera)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    if (camera.cam == nullptr)
    {
        gp_camera_new(&camera.cam);

        int m = gp_abilities_list_lookup_model(_gpCams, camera.model.c_str());
        CameraAbilities abilities{};
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

        // Get the available shutterspeeds
        initCameraProperty(camera, "shutterspeed", camera.shutterspeeds);
        initCameraProperty(camera, "aperture", camera.apertures);
        initCameraProperty(camera, "iso", camera.isos);

        return true;
    }
    else
    {
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Camera " << camera.model << " already initialized" << Log::endl;
        return false;
    }
}

/*************/
void Image_GPhoto::initCameraProperty(GPhotoCamera& camera, const string& property, vector<string>& values)
{
    values.clear();
    CameraWidget* cameraConfig;
    gp_camera_get_config(camera.cam, &cameraConfig, _gpContext);

    CameraWidget* widget;
    if (gp_widget_get_child_by_name(cameraConfig, property.c_str(), &widget) == GP_OK)
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
        Log::get() << Log::WARNING << "Image_GPhoto::" << __FUNCTION__ << " - Property " << property << " is not available for camera " << camera.model << Log::endl;
    }

    gp_widget_free(cameraConfig);
}

/*************/
void Image_GPhoto::releaseCamera(GPhotoCamera& camera)
{
    lock_guard<recursive_mutex> lock(_gpMutex);

    gp_camera_exit(camera.cam, _gpContext);
    if (camera.cam != nullptr)
        gp_camera_unref(camera.cam);

    camera.cam = nullptr;
}

/*************/
void Image_GPhoto::registerAttributes()
{
    Image::registerAttributes();

    addAttribute("cooldown",
        [&](const Values& args) {
            _cooldownTime = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_cooldownTime}; },
        {'n'});
    setAttributeDescription("cooldown", "Cooldown after a capture, some cameras need some rest before changing parameters");
    setAttributeParameter("cooldown", true);

    addAttribute("aperture",
        [&](const Values& args) { return doSetProperty("aperture", args[0].as<string>()); },
        [&]() -> Values {
            string value;
            if (doGetProperty("aperture", value))
                return {value};
            else
                return {};
        },
        {});
    setAttributeDescription("aperture", "Set the aperture of the lens");
    setAttributeParameter("aperture", true);

    addAttribute("isospeed",
        [&](const Values& args) { return doSetProperty("iso", args[0].as<string>()); },
        [&]() -> Values {
            string value;
            if (doGetProperty("iso", value))
                return {value};
            else
                return {};
        },
        {});
    setAttributeDescription("isospeed", "Set the ISO value of the camera");
    setAttributeParameter("isospeed", true);

    addAttribute("shutterspeed",
        [&](const Values& args) {
            doSetProperty("shutterspeed", getShutterspeedStringFromFloat(args[0].as<float>()));
            return true;
        },
        [&]() -> Values {
            string value;
            doGetProperty("shutterspeed", value);
            float duration = getFloatFromShutterspeedString(value);
            return {duration};
        },
        {});
    setAttributeDescription("shutterspeed", "Set the camera shutter speed");
    setAttributeParameter("shutterspeed", true);

    // Actions
    addAttribute("capture", [&](const Values&) {
        runAsyncTask([=]() { capture(); });
        return true;
    });
    setAttributeDescription("capture", "Ask for the camera to shoot");

    addAttribute("detect", [&](const Values&) {
        runAsyncTask([=]() { detectCameras(); });
        return true;
    });
    setAttributeDescription("detect", "Ask for camera detection");

    // Status
    addAttribute("ready",
        [&](const Values&) { return false; },
        [&]() -> Values {
            lock_guard<recursive_mutex> lock(_gpMutex);
            if (_selectedCameraIndex == -1)
                return {0};
            else
                return {1};
        });
    setAttributeDescription("ready", "Ask whether the camera is ready to shoot");
}

} // namespace Splash
