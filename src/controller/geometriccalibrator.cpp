#include "./controller/geometriccalibrator.h"

#include <algorithm>
#include <thread>

#include <opencv2/opencv.hpp>
#include <calimiro/calimiro.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "./image/image.h"
#include "./image/image_list.h"
#include "./image/image_opencv.h"

#if HAVE_LINUX
#include "./image/image_v4l2.h"
#endif

#if HAVE_GPHOTO
#include "./image/image_gphoto.h"
#endif

namespace Splash
{

/*************/
GeometricCalibrator::GeometricCalibrator(RootObject* root)
    : ControllerObject(root)
{
    _type = "geometricCalibrator";
    _renderingPriority = GraphObject::Priority::POST_WINDOW;
    registerAttributes();
}

/*************/
GeometricCalibrator::~GeometricCalibrator()
{
    _abortCalibration = true;
    _finalizeCalibration = true;
}

/*************/
void GeometricCalibrator::calibrate()
{
    if (!_grabber)
    {
        Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - No grabber linked to the geometric calibrator" << Log::endl;
        return;
    }

    if (_running)
    {
        Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Geometric calibration is already running" << Log::endl;
        return;
    }

    _running = true;
    _calibrationFuture = std::async(std::launch::async, [=]() {
        auto state = saveCurrentState();
        setupCalibrationState(state);
        auto params = calibrationFunc(state);
        restoreState(state);
        _running = false;

        if (params.has_value())
        {
            applyCalibration(state, params.value());
            return true;
        }

        return false;
    });
}

/*************/
bool GeometricCalibrator::linkIt(const shared_ptr<GraphObject>& obj)
{
    if (_grabber)
        return false;

    auto image = dynamic_pointer_cast<Image>(obj);
    if (!image)
        return false;

    _grabber = image;
    return true;
}

/*************/
void GeometricCalibrator::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    auto image = dynamic_pointer_cast<Image>(obj);
    if (!image)
        return;

    if (_grabber == image)
        _grabber.reset();
}

/*************/
GeometricCalibrator::ConfigurationState GeometricCalibrator::saveCurrentState()
{
    ConfigurationState state = {.cameraList = getObjectsOfType("camera"),
        .windowList = getObjectsOfType("window"),
        .objectTypes = getObjectTypes(),
        .objectLinks = getObjectLinks(),
        .objectReversedLinks = getObjectReversedLinks()};

    for (const auto& windowName : state.windowList)
    {
        state.windowLayouts.push_back(getObjectAttribute(windowName, "layout"));
        state.windowTextureCount.push_back(getObjectAttribute(windowName, "textureList").size());
    }

    return state;
}

/*************/
void GeometricCalibrator::restoreState(const GeometricCalibrator::ConfigurationState& state)
{
    Log::get() << Log::MESSAGE << "GeometricCalibrator::" << __FUNCTION__ << " - Cleaning calibration objects" << Log::endl;

    for (size_t index = 0; index < state.windowList.size(); ++index)
    {
        auto& windowName = state.windowList[index];
        auto filterName = _worldFilterPrefix + std::to_string(index);
        setWorldAttribute("unlink", {filterName, windowName});
        setWorldAttribute("unlink", {_worldBlackImage, filterName});
        setWorldAttribute("unlink", {_worldGreyImage, filterName});
        setWorldAttribute("unlink", {_worldImageName, filterName});
        setWorldAttribute("deleteObject", {filterName});
    }

    setWorldAttribute("deleteObject", {_worldBlackImage});
    setWorldAttribute("deleteObject", {_worldGreyImage});
    setWorldAttribute("deleteObject", {_worldImageName});

    // Reset the original windows state
    for (size_t index = 0; index < state.windowList.size(); ++index)
    {
        auto& windowName = state.windowList[index];
        setObjectAttribute(windowName, "layout", {state.windowLayouts[index]});
        for (const auto& objectName : state.objectLinks.at(windowName))
            setWorldAttribute("link", {objectName, windowName});
    }
}

/*************/
void GeometricCalibrator::setupCalibrationState(const GeometricCalibrator::ConfigurationState& state)
{
    // Unlink everything from the Windows, except the GUI (we might need it...)
    for (const auto& windowName : state.windowList)
    {
        for (const auto& objectName : state.objectLinks.at(windowName))
        {
            if (state.objectTypes.at(objectName) == "gui")
                continue;
            setWorldAttribute("unlink", {objectName, windowName});
        }
    }

    // Create the patterns display pipeline
    // Camera output is overridden with filters mimicking the windows layout
    setWorldAttribute("addObject", {"image", _worldImageName});
    setWorldAttribute("addObject", {"image", _worldBlackImage});
    setWorldAttribute("addObject", {"image", _worldGreyImage});

    waitForObjectCreation(_worldGreyImage);
    setObjectAttribute(_worldGreyImage, "pattern", {1});

    for (size_t index = 0; index < state.windowList.size(); ++index)
    {
        // Don't create a filter for a window with only a GUI
        if (state.windowTextureCount[index] == 0)
            continue;

        auto filterName = _worldFilterPrefix + std::to_string(index);
        setWorldAttribute("addObject", {"filter_custom", filterName});

        auto& windowName = state.windowList[index];
        setWorldAttribute("link", {_worldBlackImage, filterName});
        setWorldAttribute("link", {_worldGreyImage, filterName});
        setWorldAttribute("link", {_worldImageName, filterName});
        setWorldAttribute("link", {filterName, windowName});
    }

    // Set the parameters for the filter
    for (size_t index = 0; index < state.windowList.size(); ++index)
    {
        // No filter will be created for a window with only a GUI
        if (state.windowTextureCount[index] == 0)
            continue;

        auto filterName = _worldFilterPrefix + std::to_string(index);
        waitForObjectCreation(filterName);
        setObjectAttribute(filterName, "fileFilterSource", {std::string(DATADIR) + "/shaders/geometric_calibration_filter.frag"});

        for (auto attrList = getObjectAttributes(filterName); attrList.find("subdivs") == attrList.cend(); attrList = getObjectAttributes(filterName))
            std::this_thread::sleep_for(15ms);
        auto windowSize = getObjectAttribute(state.windowList[index], "size");
        setObjectAttribute(filterName, "subdivs", {state.windowTextureCount[index]});
        setObjectAttribute(filterName, "texLayout", {0, 0, 0, 0});
        setObjectAttribute(filterName, "sizeOverride", windowSize);
    }
}

/*************/
std::optional<GeometricCalibrator::Calibration> GeometricCalibrator::calibrationFunc(const GeometricCalibrator::ConfigurationState& state)
{
    // Begin calibration
    calimiro::Workspace workspace;
    calimiro::Structured_Light structuredLight(&_logger, _structuredLightScale);

    // Set all cameras to display a pattern
    for (size_t index = 0; index < state.windowList.size(); ++index)
    {
        auto filterName = _worldFilterPrefix + std::to_string(index);
        setObjectAttribute(filterName, "texLayout", {1, 1, 1, 1});
    }

    Image imageObject(_root);
    _finalizeCalibration = false;
    size_t positionIndex = 0;
    while (!_finalizeCalibration)
    {
        if (!_nextPosition)
        {
            std::this_thread::sleep_for(50ms);
            continue;
        }
        _nextPosition = false;
        setObjectAttribute("gui", "hide", {1});

        // If an error happens while capturing this position, this flag will be set to true
        bool abortCurrentPosition = false;

        // Set all cameras to black
        for (size_t index = 0; index < state.windowList.size(); ++index)
        {
            auto filterName = _worldFilterPrefix + std::to_string(index);
            setObjectAttribute(filterName, "texLayout", {0, 0, 0, 0});
        }

        // For each position, capture the patterns for all cameras
        std::vector<cv::Mat2i> decodedProjectors;
        for (size_t cameraIndex = 0; cameraIndex < state.cameraList.size(); ++cameraIndex)
        {
            const auto& cameraName = state.cameraList[cameraIndex];
            auto cameraSize = getObjectAttribute(cameraName, "size");
            auto camWidth = cameraSize[0].as<int>();
            auto camHeight = cameraSize[1].as<int>();
            auto patterns = structuredLight.create(camWidth, camHeight);

            // Convert patterns to RGB
            for (auto& pattern : patterns)
            {
                cv::Mat3b rgbPattern(pattern.size());
                if (pattern.channels() == 1)
                {
                    cvtColor(pattern, rgbPattern, cv::COLOR_GRAY2RGB);
                    pattern = rgbPattern;
                }
            }

            // Find which window displays the camera, and what is its ID in its layout
            std::string targetFilterName;
            std::string targetWindowName;
            int targetLayoutIndex = 0;
            for (size_t index = 0; index < state.windowList.size(); ++index)
            {
                auto& windowName = state.windowList[index];
                targetLayoutIndex = 0;
                for (const auto& objectName : state.objectLinks.at(windowName))
                {
                    if (state.objectTypes.at(objectName) != "camera")
                        continue;
                    if (objectName == cameraName)
                    {
                        targetFilterName = _worldFilterPrefix + std::to_string(index);
                        targetWindowName = windowName;
                        break;
                    }
                    ++targetLayoutIndex;
                }
                if (!targetFilterName.empty())
                    break;
            }
            if (targetFilterName.empty() || targetWindowName.empty())
                continue;

            Values layout({0, 0, 0, 0});
            layout[targetLayoutIndex] = 2; // This index will display the third texture, named _worldImageName
            setObjectAttribute(targetFilterName, "texLayout", layout);

            std::vector<cv::Mat1b> capturedPatterns{};
            for (size_t patternIndex = 0; patternIndex < patterns.size(); ++patternIndex)
            {
                auto& pattern = patterns[patternIndex];

                auto channels = pattern.channels();
                ImageBufferSpec spec(camWidth, camHeight, channels, 8 * channels, ImageBufferSpec::Type::UINT8, "RGB");
                spec.videoFrame = false;
                ImageBuffer imageBuffer(spec, pattern.data);

                imageObject.set(imageBuffer);
                imageObject.update(); // We have to force the update, as Image is double-buffered
                auto serializedImage = imageObject.serialize();

                // Send the buffer, and make sure it has been received and displayed
                sendBuffer(_worldImageName, serializedImage);
                for (int64_t updatedTimestamp = 0; updatedTimestamp != imageObject.getTimestamp();
                     updatedTimestamp = getObjectAttribute(targetWindowName, "timestamp")[0].as<int64_t>())
                    std::this_thread::sleep_for(15ms);

                // Wait for a few more frames to be drawn, to account for double buffering,
                // and exposure time of the input grabber
                std::this_thread::sleep_for(_captureDelay);

                const auto updateTime = Timer::getTime();
                // Some grabber need to be asked to capture a frame
                setObjectAttribute(_grabber->getName(), "capture", {1});
                while (updateTime > imageBuffer.getSpec().timestamp)
                {
                    imageBuffer = _grabber->get();
                    std::this_thread::sleep_for(100ms);
                }

                if (imageBuffer.empty())
                    return {};

                spec = imageBuffer.getSpec();
                cv::Mat capturedImage;

                if (spec.format.find("RGB") != std::string::npos)
                {
                    assert(spec.channels == 4 || spec.channels == 3); // All Image classes should output RGB or RGBA (when uncompressed)
                    capturedImage = cv::Mat(spec.height, spec.width, spec.channels == 4 ? CV_8UC4 : CV_8UC3, imageBuffer.data());
                    cv::cvtColor(capturedImage, capturedImage, cv::COLOR_RGB2GRAY);
                }
                else if (spec.format.find("BGR") != std::string::npos)
                {
                    assert(spec.channels == 4 || spec.channels == 3);
                    auto bgraImage = cv::Mat(spec.height, spec.width, spec.channels == 4 ? CV_8UC4 : CV_8UC3, imageBuffer.data());
                    cv::cvtColor(bgraImage, capturedImage, cv::COLOR_BGR2GRAY);
                }
                else if (spec.format == "YUYV")
                {
                    assert(spec.channels == 3 && spec.bpp == 16);
                    auto yuvImage = cv::Mat(spec.height, spec.width, CV_8UC2, imageBuffer.data());
                    cv::cvtColor(yuvImage, capturedImage, cv::COLOR_YUV2RGB_YUYV);
                }
                else
                {
                    Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Format " << spec.format << " is not supported" << Log::endl;
                    return {};
                }

                cv::Mat1b grayscale(spec.height, spec.width);
                int fromTo[] = {0, 0};
                cv::mixChannels(&capturedImage, 1, &grayscale, 1, fromTo, 1);
                capturedPatterns.push_back(grayscale);

                std::string directory = workspace.getWorkPath() + "/pos_" + std::to_string(positionIndex);
                std::filesystem::create_directory(directory);
                cv::imwrite(directory + "/prj" + std::to_string(cameraIndex) + "_pattern" + std::to_string(patternIndex) + ".jpg", capturedImage);

#ifdef DEBUG
                std::string patternImageFilePath = directory + "/prj" + std::to_string(cameraIndex) + "_pattern" + std::to_string(patternIndex) + ".png";
                if (!imageObject.write(patternImageFilePath))
                    Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Could not write image to " << patternImageFilePath << Log::endl;
#endif
            }

            layout[targetLayoutIndex] = 0; // This index will display the first texture, _worldBlackImage
            setObjectAttribute(targetFilterName, "texLayout", layout);

            if (abortCurrentPosition)
                break;

            auto decoded = structuredLight.decode(camWidth, camHeight, capturedPatterns);
            auto shadowMask = structuredLight.getShadowMask();
            auto decodedCoords = structuredLight.getDecodedCoordinates(camWidth, camHeight);

            if (!decoded || !shadowMask || !decodedCoords)
                return {};

            calimiro::Workspace::ImageList imagesToSave(
                {{"decoded_images/pos_" + std::to_string(positionIndex) + "_proj" + std::to_string(cameraIndex) + "_shadow_mask.jpg", shadowMask.value()},
                    {"decoded_images/pos_" + std::to_string(positionIndex) + "_proj" + std::to_string(cameraIndex) + "_x.jpg", decodedCoords.value().first},
                    {"decoded_images/pos_" + std::to_string(positionIndex) + "_proj" + std::to_string(cameraIndex) + "_y.jpg", decodedCoords.value().second}});
            if (!workspace.saveImagesFromList(imagesToSave))
                return {};
            decodedProjectors.push_back(decoded.value());
        }

        // Set all cameras to display a pattern
        for (size_t index = 0; index < state.windowList.size(); ++index)
        {
            auto filterName = _worldFilterPrefix + std::to_string(index);
            setObjectAttribute(filterName, "texLayout", {1, 1, 1, 1});
        }

        if (abortCurrentPosition)
            continue;

        auto mergedProjectors = workspace.combineDecodedProjectors(decodedProjectors);
        if (!mergedProjectors)
            return {};
        workspace.exportMatrixToYaml(mergedProjectors.value(), "decoded_matrix/pos_" + std::to_string(positionIndex));

        ++positionIndex;
        setObjectAttribute("gui", "show", {1});
    }
    _finalizeCalibration = false;

    if (_abortCalibration)
    {
        _abortCalibration = false;
        Log::get() << Log::MESSAGE << "GeometricCalibrator::" << __FUNCTION__ << " - Geometric calibration aborted" << Log::endl;
        return {};
    }

    if (positionIndex < 3)
    {
        Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Not enough positions have been captured to do the calibration" << Log::endl;
        return {};
    }

    // Compute the calibration
    auto calimiroCameraModel = _cameraModel == CameraModel::Pinhole ? calimiro::Reconstruction::PINHOLE_CAMERA_RADIAL3 : calimiro::Reconstruction::PINHOLE_CAMERA_FISHEYE;
    calimiro::Reconstruction reconstruction(&_logger, workspace.getWorkPath(), calimiroCameraModel);
    reconstruction.sfmInitImageListing(_cameraFocal);
    reconstruction.computeFeatures();
    reconstruction.computeMatches();
    reconstruction.incrementalSfM();
    reconstruction.computeStructureFromKnownPoses();
    reconstruction.convertSfMStructure();

    // Generate the mesh
    auto points =
        calimiro::utils::readPly(&_logger, std::filesystem::path(workspace.getWorkPath()) / calimiro::constants::cOutputDirectory / calimiro::constants::cPointCloudStructureFromKnownPoses_ply);
    auto geometry = calimiro::Geometry(&_logger, points, {}, {}, {});
    auto geometryNormals = geometry.computeNormalsPointSet();
    auto geometryMesh = geometryNormals.marchingCubes(600);
    auto geometryMeshClean = geometryMesh.simplifyGeometry();
    auto geometryMeshUvs = geometryMeshClean.uvCoordinatesSphere();

    calimiro::Obj objFile(&_logger, geometryMeshUvs);
    objFile.writeMesh(std::filesystem::path(workspace.getWorkPath()) / _finalMeshName);

    Calibration calibration;
    calibration.meshPath = workspace.getWorkPath() + "/" + _finalMeshName;

    // Compute projectors calibrations, and set the Camera objects
    for (size_t cameraIndex = 0; cameraIndex < state.cameraList.size(); ++cameraIndex)
    {
        const auto& cameraName = state.cameraList[cameraIndex];
        auto cameraSize = getObjectAttribute(cameraName, "size");
        auto camHeight = cameraSize[1].as<int>();

        calimiro::MapXYZs pixelMap(&_logger, workspace.getWorkPath());
        pixelMap.pixelToProj(camHeight, _structuredLightScale);
        auto matchesByProj = pixelMap.sampling(15);

        std::shared_ptr<calimiro::Camera> cameraModel{nullptr};
        cameraModel = std::make_shared<calimiro::cameramodel::Pinhole>(cameraSize[0].as<int>(), cameraSize[1].as<int>());

        std::vector<int> inliers;
        std::vector<double> parameters;
        calimiro::Kernel kernel(&_logger, cameraModel, matchesByProj[cameraIndex + 1]); // Projectors start at 1 in Calimiro
        parameters = kernel.Ransac(inliers);

        if (parameters.empty())
        {
            Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Unable to compute calibration parameters for camera " << cameraName << Log::endl;
            return {};
        }

        Log::get() << Log::MESSAGE << "GeometricCalibrator::" << __FUNCTION__ << " - Camera " << cameraName << " parameters (fov, cx, cy, eye[3], rot[3], k1): ";
        for (const auto& p : parameters)
            Log::get() << p << " ";
        Log::get() << Log::endl;

        double fov = parameters[0];
        double cx = parameters[1];
        double cy = parameters[2];

        glm::dvec3 euler{0.0, 0.0, 0.0};
        glm::dvec4 eye{0.0, 0.0, 0.0, 0.0};
        for (int i = 0; i < 3; ++i)
        {
            eye[i] = parameters[i + 3];
            euler[i] = parameters[i + 6];
        }

        glm::dmat4 rotateMat = glm::yawPitchRoll(euler[0], euler[1], euler[2]);
        glm::dvec4 target = rotateMat * glm::dvec4(1.0, 0.0, 0.0, 0.0);
        glm::dvec4 up = rotateMat * glm::dvec4(0.0, 0.0, 1.0, 0.0);
        target += eye;
        up = glm::normalize(up);

        CalibrationParams params = {.cameraName = cameraName, .fov = fov, .cx = cx, .cy = cy, .eye = eye, .target = target, .up = up};
        calibration.params.push_back(params);
    }

    return {calibration};
}

/*************/
void GeometricCalibrator::applyCalibration(const GeometricCalibrator::ConfigurationState& state, const Calibration& calibration)
{
    Log::get() << Log::MESSAGE << "GeometricCalibrator::" << __FUNCTION__ << " - Calibration set" << Log::endl;

    // Load the mesh into a new Mesh object, create a new Object to replace the one connected to the cameras
    const std::string calibrationPrefixName = "calibration_";
    setWorldAttribute("addObject", {"mesh", calibrationPrefixName + "mesh"});
    setWorldAttribute("addObject", {"object", calibrationPrefixName + "object"});
    setWorldAttribute("addObject", {"image", calibrationPrefixName + "image"});
    setWorldAttribute("link", {calibrationPrefixName + "mesh", calibrationPrefixName + "object"});
    setWorldAttribute("link", {calibrationPrefixName + "image", calibrationPrefixName + "object"});

    for (size_t cameraIndex = 0; cameraIndex < state.cameraList.size(); ++cameraIndex)
    {
        const auto& cameraName = state.cameraList[cameraIndex];
        for (const auto& object : state.objectLinks.at(cameraName))
            setWorldAttribute("unlink", {object, cameraName});
        setWorldAttribute("link", {calibrationPrefixName + "object", cameraName});
    }

    waitForObjectCreation(calibrationPrefixName + "mesh");
    setObjectAttribute(calibrationPrefixName + "mesh", "file", {calibration.meshPath});

    // Apply projector calibration
    for (const auto& params : calibration.params)
    {
        setObjectAttribute(params.cameraName, "fov", {params.fov});
        setObjectAttribute(params.cameraName, "principalPoint", {params.cx, params.cy});
        setObjectAttribute(params.cameraName, "eye", {params.eye.x, params.eye.y, params.eye.z});
        setObjectAttribute(params.cameraName, "target", {params.target.x, params.target.y, params.target.z});
        setObjectAttribute(params.cameraName, "up", {params.up.x, params.up.y, params.up.z});
    }
}

/*************/
void GeometricCalibrator::registerAttributes()
{
    addAttribute("calibrate", [&](const Values&) {
        calibrate();
        return true;
    });
    setAttributeDescription("calibrate", "Run the geometric calibration");

    addAttribute("nextPosition", [&](const Values&) {
        _nextPosition = true;
        return true;
    });
    setAttributeDescription("nextPosition", "Signals the calibration algorithm to capture the next position");

    addAttribute("finalizeCalibration", [&](const Values&) {
        _finalizeCalibration = true;
        return true;
    });
    setAttributeDescription("finalizeCalibration", "Signals the calibration algorithm to finalize the calibration process");

    addAttribute("abortCalibration", [&](const Values&) {
        _abortCalibration = true;
        _finalizeCalibration = true;
        return true;
    });
    setAttributeDescription("abortCalibration", "Signals the calibration algorithm to abort");

    addAttribute("cameraFocal",
        [&](const Values& args) {
            _cameraFocal = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_cameraFocal}; },
        {'r'});
    setAttributeDescription("cameraFocal", "Capture camera focal, in pixels (relatively to the sensor size)");

    addAttribute("cameraModel",
        [&](const Values& args) {
            auto model = args[0].as<string>();
            if (model.find("PINHOLE") == 0)
                _cameraModel = CameraModel::Pinhole;
            else if (model.find("FISHEYE") == 0)
                _cameraModel = CameraModel::Fisheye;
            return true;
        },
        [&]() -> Values {
            if (_cameraModel == CameraModel::Pinhole)
                return {"PINHOLE"};
            else if (_cameraModel == CameraModel::Fisheye)
                return {"FISHEYE"};
            assert(false);
            return {};
        },
        {'s'});
    setAttributeDescription("cameraModel", "Camera model used for reconstruction, either PINHOLE or FISHEYE");

    addAttribute("captureDelay",
        [&](const Values& args) {
            _captureDelay = std::chrono::milliseconds(args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_captureDelay.count()}; },
        {'i'});
    setAttributeDescription("captureDelay", "Delay between the display of the next pattern and grabbing it through the camera");

    addAttribute("patternScale",
        [&](const Values& args) {
            _structuredLightScale = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_structuredLightScale}; },
        {'r'});
    setAttributeDescription("patternScale", "Scale of the structured light pattern to be projected");
}

} // namespace Splash
