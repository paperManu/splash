#include "./controller/geometriccalibrator.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include <opencv2/opencv.hpp>
#include <slaps/slaps.h>

#include "./image/image.h"
#include "./image/image_gphoto.h"

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
void GeometricCalibrator::update()
{
    if (!_runCalibration)
        return;

    if (!_running)
    {
        _running = true;
        _calibrationFuture = std::async(std::launch::async, [=]() { return calibrationFunc(); });
    }

    if (_finished)
    {
        _calibrationFuture = std::future<bool>();
        _runCalibration = false;
    }
}

/*************/
bool GeometricCalibrator::calibrationFunc()
{
    // Save current windows state
    const auto cameraList = getObjectsOfType("camera");
    const auto windowList = getObjectsOfType("window");
    const auto objectTypes = getObjectTypes();
    const auto objectLinks = getObjectLinks();
    const auto objectReversedLinks = getObjectReversedLinks();

    std::vector<Values> windowLayouts;
    std::vector<uint8_t> windowTextureCount;
    for (const auto& windowName : windowList)
    {
        windowLayouts.push_back(getObjectAttribute(windowName, "layout"));
        windowTextureCount.push_back(getObjectAttribute(windowName, "textureList").size());
        for (const auto& objectName : objectLinks.at(windowName))
        {
            if (objectTypes.at(objectName) == "gui")
                continue;
            setWorldAttribute("unlink", {objectName, windowName});
        }
    }

    // Hide the GUI
    setObjectAttribute("gui", "hide");

    // Create the patterns display pipeline
    // Camera output is overridden with filters mimicking the windows layout
    Image imageObject(_root);
    const string worldImageName = "__pattern_image";
    const string worldBlackImage = "__black_image";
    const string worldFilterPrefix = "__pattern_filter_";
    setWorldAttribute("addObject", {"image", worldImageName});
    setWorldAttribute("addObject", {"image", worldBlackImage});

    for (size_t index = 0; index < windowList.size(); ++index)
    {
        auto filterName = worldFilterPrefix + std::to_string(index);
        setWorldAttribute("addObject", {"filter", filterName});

        auto& windowName = windowList[index];
        setWorldAttribute("link", {worldBlackImage, filterName});
        setWorldAttribute("link", {worldImageName, filterName});
        setWorldAttribute("link", {filterName, windowName});
    }

    // Set the parameters for the filter
    for (size_t index = 0; index < windowList.size(); ++index)
    {
        auto filterName = worldFilterPrefix + std::to_string(index);
        for (auto objects = getObjectList(); std::find(objects.cbegin(), objects.cend(), filterName) == objects.cend(); objects = getObjectList())
            std::this_thread::sleep_for(15ms);
        setObjectAttribute(filterName, "fileFilterSource", {std::string(DATADIR) + "/shaders/geometric_calibration_filter.frag"});

        for (auto attrList = getObjectAttributes(filterName); attrList.find("subdivs") == attrList.cend(); attrList = getObjectAttributes(filterName))
            std::this_thread::sleep_for(15ms);
        auto windowSize = getObjectAttribute(windowList[index], "size");
        setObjectAttribute(filterName, "subdivs", {windowTextureCount[index]});
        setObjectAttribute(filterName, "texLayout", {0, 0, 0, 0});
        setObjectAttribute(filterName, "sizeOverride", windowSize);
    }

    // Get the name of the object generated for linking the pattern image to the filter
    std::map<std::string, std::string> patternTextureNames;
    for (size_t index = 0; index < windowList.size(); ++index)
    {
        auto filterName = worldFilterPrefix + std::to_string(index);
        auto filterLinks = getObjectLinks()[filterName];
        auto patternTextureIt =
            std::find_if(filterLinks.cbegin(), filterLinks.cend(), [&](const auto& v) { return v != worldImageName && v.find(worldImageName) != std::string::npos; });
        if (patternTextureIt == filterLinks.end())
            return false;
        patternTextureNames[filterName] = *patternTextureIt;
    }

    // Whenever we exit this function, cleanup our mess
    OnScopeExit
    {
        for (size_t index = 0; index < windowList.size(); ++index)
        {
            auto& windowName = windowList[index];
            auto filterName = worldFilterPrefix + std::to_string(index);
            setWorldAttribute("unlink", {filterName, windowName});
            setWorldAttribute("unlink", {worldBlackImage, filterName});
            setWorldAttribute("unlink", {worldImageName, filterName});
            setWorldAttribute("deleteObject", {filterName});
        }

        setWorldAttribute("deleteObject", {worldBlackImage});
        setWorldAttribute("deleteObject", {worldImageName});

        // Reset the original windows state
        for (size_t index = 0; index < windowList.size(); ++index)
        {
            auto& windowName = windowList[index];
            setObjectAttribute(windowName, "layout", {windowLayouts[index]});
            for (const auto& objectName : objectLinks.at(windowName))
                setWorldAttribute("link", {objectName, windowName});
        }

        _finished = true;

        // Show the GUI
        setObjectAttribute("gui", "show");
    };

    // Begin calibration
    slaps::Workspace workspace;
    slaps::Structured_Light structuredLight(_structuredLightScale);

    size_t positionIndex = 0;
    while (!_finalizeCalibration)
    {
        _nextPosition = false;
        // Camera capture
        auto captureCam = Image_GPhoto(_root, "");
        auto camStatus = captureCam.getAttribute("ready");
        if (!camStatus || !camStatus.value()[0].as<bool>())
        {
            Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Unable to connect to the camera, please retry" << Log::endl;
            while (!_nextPosition && !_finalizeCalibration)
                std::this_thread::sleep_for(50ms);
            continue;
        }

        // If an error happens while capturing this position, this flag will be set to true
        bool abortCurrentPosition = false;

        // For each position, capture the patterns for all cameras
        std::vector<cv::Mat2i> decodedProjectors;
        for (size_t cameraIndex = 0; cameraIndex < cameraList.size(); ++cameraIndex)
        {
            const auto& cameraName = cameraList[cameraIndex];
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
                    cvtColor(pattern, rgbPattern, CV_GRAY2RGB);
                    pattern = rgbPattern;
                }
            }

            // Find which window displays the camera, and what is its ID in its layout
            std::string targetFilterName;
            int targetLayoutIndex = 0;
            for (size_t index = 0; index < windowList.size(); ++index)
            {
                auto& windowName = windowList[index];
                targetLayoutIndex = 0;
                for (const auto& objectName : objectLinks.at(windowName))
                {
                    if (objectTypes.at(objectName) != "camera")
                        continue;
                    if (objectName == cameraName)
                    {
                        targetFilterName = worldFilterPrefix + std::to_string(index);
                        break;
                    }
                    ++targetLayoutIndex;
                }
                if (!targetFilterName.empty())
                    break;
            }
            if (targetFilterName.empty())
                continue;
            auto& patternTextureName = patternTextureNames[targetFilterName];

            Values layout({0, 0, 0, 0});
            layout[targetLayoutIndex] = 1; // This index will display the second texture, named worldImageName
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
                auto serializedImage = imageObject.serialize();

                // Send the buffer, and make sure it has been received and displayed
                auto currentTimestamp = getObjectAttribute(patternTextureName, "lastDrawnTimestamp");
                sendBuffer(worldImageName, serializedImage);

                for (auto updatedTimestamp = currentTimestamp; updatedTimestamp == currentTimestamp;
                     updatedTimestamp = getObjectAttribute(patternTextureName, "lastDrawnTimestamp"))
                    std::this_thread::sleep_for(15ms);

                if (!captureCam.capture())
                {
                    Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Error while capturing frame, aborting captures for current position" << Log::endl;
                    abortCurrentPosition = true;
                    break;
                }

                captureCam.update();
                imageBuffer = captureCam.get();
                if (imageBuffer.getSize() == 0)
                    return false;

                spec = imageBuffer.getSpec();
                cv::Mat capturedImage(spec.height, spec.width, CV_8UC4, imageBuffer.data());

                cv::cvtColor(capturedImage, capturedImage, CV_RGB2GRAY);
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

            if (abortCurrentPosition)
                break;

            auto decoded = structuredLight.decode(camWidth, camHeight, capturedPatterns);
            auto shadowMask = structuredLight.getShadowMask();
            auto decodedCoords = structuredLight.getDecodedCoordinates(camWidth, camHeight);

            if (!decoded || !shadowMask || !decodedCoords)
                return false;

            slaps::Workspace::ImageList imagesToSave(
                {{"decoded_images/pos_" + std::to_string(positionIndex) + "_proj" + std::to_string(cameraIndex) + "_shadow_mask.jpg", shadowMask.value()},
                    {"decoded_images/pos_" + std::to_string(positionIndex) + "_proj" + std::to_string(cameraIndex) + "_x.jpg", decodedCoords.value().first},
                    {"decoded_images/pos_" + std::to_string(positionIndex) + "_proj" + std::to_string(cameraIndex) + "_y.jpg", decodedCoords.value().second}});
            if (!workspace.saveImagesFromList(imagesToSave))
                return false;
            decodedProjectors.push_back(decoded.value());

            setObjectAttribute(targetFilterName, "texLayout", {0, 0, 0, 0});
        }

        if (abortCurrentPosition)
            continue;

        auto mergedProjectors = workspace.combineDecodedProjectors(decodedProjectors);
        if (!mergedProjectors)
            return false;
        workspace.exportMatrixToYaml(mergedProjectors.value(), "decoded_matrix/pos_" + std::to_string(positionIndex));

        ++positionIndex;
        while (!_nextPosition && !_finalizeCalibration)
            std::this_thread::sleep_for(50ms);
    }
    _finalizeCalibration = false;

    if (positionIndex < 3)
    {
        Log::get() << Log::WARNING << "GeometricCalibrator::" << __FUNCTION__ << " - Not enough positions have been captured to do the calibration" << Log::endl;
        return false;
    }

    // Compute the calibration
    auto slapsCameraModel = _cameraModel == CameraModel::Pinhole ? slaps::Reconstruction::PINHOLE_CAMERA_RADIAL3 : slaps::Reconstruction::PINHOLE_CAMERA_FISHEYE;
    slaps::Reconstruction reconstruction(workspace.getWorkPath(), slapsCameraModel);
    reconstruction.sfmInitImageListing(_cameraFocal);
    reconstruction.computeFeatures();
    reconstruction.computeMatches();
    reconstruction.incrementalSfM();
    reconstruction.computeStructureFromKnownPoses();
    reconstruction.convertSfMStructure();

    // Generate the mesh
    auto points =
        slaps::utils::readPly(std::filesystem::path(workspace.getWorkPath()) / slaps::constants::cOutputDirectory / slaps::constants::cPointCloudStructureFromKnownPoses_ply);
    auto geometry = slaps::Geometry(points, {}, {}, {});
    auto geometryNormals = geometry.computeNormalsPointSet();
    auto geometryMesh = geometryNormals.marchingCubes(600);
    auto geometryMeshUvs = geometryMesh.uvCoordinatesSphere();

    slaps::Obj objFile(geometryMeshUvs);
    objFile.writeMesh(std::filesystem::path(workspace.getWorkPath()) / "final_mesh.obj");

    // Compute projectors calibrations
    for (size_t cameraIndex = 0; cameraIndex < cameraList.size(); ++cameraIndex)
    {
        const auto& cameraName = cameraList[cameraIndex];
        auto cameraSize = getObjectAttribute(cameraName, "size");
        auto camHeight = cameraSize[1].as<int>();

        slaps::MapXYZs pixelMap(workspace.getWorkPath());
        pixelMap.pixelToProj(camHeight, _structuredLightScale);
        auto matchesByProj = pixelMap.sampling(15);
    }

    return true;
}

/*************/
void GeometricCalibrator::registerAttributes()
{
    addAttribute("calibrate", [&](const Values&) {
        _runCalibration = true;
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

    addAttribute("cameraFocal",
        [&](const Values& args) {
            _cameraFocal = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_cameraFocal}; },
        {'n'});
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

    addAttribute("patternScale",
        [&](const Values& args) {
            _structuredLightScale = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_structuredLightScale}; },
        {'n'});
    setAttributeDescription("patternScale", "Scale of the structured light pattern to be projected");
}

} // namespace Splash
