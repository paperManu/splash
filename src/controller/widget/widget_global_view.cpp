#include "./controller/widget/widget_global_view.h"

#include <array>
#include <fstream>
#include <imgui.h>

#include "./graphics/virtual_probe.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"

using namespace std;

namespace Splash
{

/*************/
void GuiGlobalView::captureJoystick()
{
    if (_joystickCaptured)
        return;

    UserInput::setCallback(UserInput::State("joystick_0_buttons"), [&](const UserInput::State& state) {
        lock_guard<mutex> lock(_joystickMutex);
        _joyButtons.clear();
        for (auto& b : state.value)
            _joyButtons.push_back(b.as<int>());
    });

    UserInput::setCallback(UserInput::State("joystick_0_axes"), [&](const UserInput::State& state) {
        lock_guard<mutex> lock(_joystickMutex);
        _joyAxes.resize(state.value.size(), 0.f);
        for (uint32_t i = 0; i < _joyAxes.size(); ++i)
            _joyAxes[i] += state.value[i].as<float>();
    });

    _joystickCaptured = true;
}

/*************/
void GuiGlobalView::releaseJoystick()
{
    if (!_joystickCaptured)
        return;

    UserInput::resetCallback(UserInput::State("joystick_0_buttons"));
    UserInput::resetCallback(UserInput::State("joystick_0_axes"));

    _joystickCaptured = false;
}

/*************/
void GuiGlobalView::render()
{
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        captureJoystick();
        if (ImGui::Button("Calibrate camera"))
            doCalibration();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Calibrate the selected camera\n(C while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Button("Revert camera"))
            revertCalibration();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Revert the selected camera to its previous calibration\n(Ctrl + Z while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Button("Reset camera") && _camera)
        {
            auto cameraName = _camera->getName();
            setObjectAttribute(cameraName, "eye", {2.0, 2.0, 2.0});
            setObjectAttribute(cameraName, "target", {0.0, 0.0, 0.0});
            setObjectAttribute(cameraName, "up", {0.0, 0.0, 1.0});
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset the camera to default values, useful when lost in 3D space");

        ImGui::Checkbox("Hide other cameras", &_hideCameras);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Hide all but the selected camera\n(H while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Checkbox("Show targets", &_showCalibrationPoints))
            showAllCalibrationPoints(static_cast<Camera::CalibrationPointsVisibility>(_showCalibrationPoints));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show the target positions for the calibration points\n(A while hovering the view)");
        ImGui::SameLine();

        static bool showAllCamerasPoints = false;
        if (ImGui::Checkbox("Show points everywhere", &showAllCamerasPoints))
            showAllCamerasCalibrationPoints();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show this camera's calibration points in other cameras\n(O while hovering the view)");
        ImGui::SameLine();

        // Colorization of the wireframe rendering. Applied after the GUI camera rendering to keep cameras in gui white
        ImGui::Checkbox("Colorize wireframes", &_camerasColorized);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Activate colorization of the wireframe rendering, green for selected camera and magenta for the other cameras\n(V while hovering the view)");

        double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;

        auto cameras = getCameras();
        drawVirtualProbes();

        ImGui::BeginChild("Cameras", ImVec2(ImGui::GetWindowWidth() * 0.25, ImGui::GetWindowWidth() * 0.67), true);
        ImGui::Text("Select a camera:");
        for (auto& camera : cameras)
        {
            camera->render();

            Values size;
            camera->getAttribute("size", size);

            int w = ImGui::GetWindowWidth() - 4 * leftMargin;
            int h = w * size[1].as<int>() / size[0].as<int>();

            if (ImGui::ImageButton((void*)(intptr_t)camera->getTexture()->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0)))
            {
                // If shift is pressed, we hide / unhide this camera
                if (io.KeyCtrl)
                {
                    setObjectAttribute(camera->getName(), "hide", {-1});
                }
                else
                {
                    // Empty previous camera parameters
                    _previousCameraParameters.clear();

                    // List the number of visible cameras
                    vector<string> visibleCameras{};
                    for (const auto& cam : cameras)
                    {
                        auto visibility = getObjectAttribute(cam->getName(), "hide");
                        if (visibility.size() and !visibility[0].as<bool>())
                            visibleCameras.push_back(cam->getName());
                    }

                    // Keep the same set of cameras visible
                    for (const auto& cam : cameras)
                        setObjectAttribute(cam->getName(), "hide", {1});

                    for (const auto& camName : visibleCameras)
                        setObjectAttribute(camName, "hide", {0});

                    // Ensure the selected camera is visible...
                    setObjectAttribute(camera->getName(), "hide", {0});

                    _camerasColorized = false;
                    setObjectAttribute(_camera->getName(), "frame", {0});
                    setObjectAttribute(_camera->getName(), "displayCalibration", {0});

                    _camera = camera;

                    setObjectAttribute(_camera->getName(), "frame", {1});
                    setObjectAttribute(_camera->getName(), "displayCalibration", {1});
                    showAllCalibrationPoints(static_cast<Camera::CalibrationPointsVisibility>(_showCalibrationPoints));
                }
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", camera->getAlias().c_str());
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Calibration", ImVec2(0, ImGui::GetWindowWidth() * 0.67), true);
        if (_camera != nullptr)
        {
            Values size;
            _camera->getAttribute("size", size);

            int sizeX = size[0].as<int>();
            int sizeY = size[1].as<int>();

            int w = ImGui::GetWindowWidth() - 2 * leftMargin;
            int h = sizeX != 0 ? w * sizeY / sizeX : 1;

            _camWidth = w;
            _camHeight = h;

            Values reprojectionError;
            _camera->getAttribute("getReprojectionError", reprojectionError);
            ImGui::Text("Current camera: %s - Reprojection error: %f", _camera->getAlias().c_str(), reprojectionError[0].as<float>());

            ImGui::Image((void*)(intptr_t)_camera->getTexture()->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));
            if (ImGui::IsItemHoveredRect())
                _noMove = true;
            else
                _noMove = false;

            processKeyEvents();
            processMouseEvents();
        }
        ImGui::EndChild();

        // Applying options which should not be visible inside the GUI
        hideOtherCameras(_hideCameras);
        colorizeCameraWireframes(_camerasColorized);

        // Joystick can be updated independently from the mouse position
        processJoystickState();
    }
    else
    {
        releaseJoystick();
    }
}

/*************/
int GuiGlobalView::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    if (_noMove)
    {
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoScrollWithMouse;
    }
    return flags;
}

/*************/
void GuiGlobalView::setCamera(const shared_ptr<Camera>& cam)
{
    if (cam != nullptr)
    {
        _camera = cam;
        _guiCamera = cam;
        _camera->setAttribute("size", {800, 600});
    }
}

/*************/
void GuiGlobalView::setJoystick(const vector<float>& axes, const vector<uint8_t>& buttons)
{
    _joyAxes = axes;
    _joyButtons = buttons;
}

/*************/
vector<glm::dmat4> GuiGlobalView::getCamerasRTMatrices()
{
    auto rtMatrices = vector<glm::dmat4>();
    auto cameras = getObjectsPtr(getObjectsOfType("camera"));
    for (auto& camera : cameras)
        rtMatrices.push_back(dynamic_pointer_cast<Camera>(camera)->computeViewMatrix());

    return rtMatrices;
}

/*************/
void GuiGlobalView::nextCamera()
{
    vector<shared_ptr<Camera>> cameras;

    // Empty previous camera parameters
    _previousCameraParameters.clear();

    // Ensure that all cameras are shown
    _camerasHidden = false;
    _camerasColorized = false;
    setObjectsOfType("camera", "hide", {0});

    setObjectAttribute(_camera->getName(), "frame", {0});
    setObjectAttribute(_camera->getName(), "displayCalibration", {0});

    if (cameras.size() == 0)
        _camera = _guiCamera;
    else if (_camera == _guiCamera)
        _camera = cameras[0];
    else
    {
        for (uint32_t i = 0; i < cameras.size(); ++i)
        {
            if (cameras[i] == _camera && i == cameras.size() - 1)
            {
                _camera = _guiCamera;
                break;
            }
            else if (cameras[i] == _camera)
            {
                _camera = cameras[i + 1];
                break;
            }
        }
    }

    if (_camera != _guiCamera)
    {
        setObjectAttribute(_camera->getName(), "frame", {1});
        setObjectAttribute(_camera->getName(), "displayCalibration", {1});
    }
}

/*************/
void GuiGlobalView::revertCalibration()
{
    if (_previousCameraParameters.size() == 0)
        return;

    Log::get() << Log::MESSAGE << "GuiGlobalView::" << __FUNCTION__ << " - Reverting camera to previous parameters" << Log::endl;

    auto params = _previousCameraParameters.back();
    // We keep the very first calibration, it has proven useful
    if (_previousCameraParameters.size() > 1)
        _previousCameraParameters.pop_back();

    setObjectAttribute(_camera->getName(), "eye", {params.eye[0], params.eye[1], params.eye[2]});
    setObjectAttribute(_camera->getName(), "target", {params.target[0], params.target[1], params.target[2]});
    setObjectAttribute(_camera->getName(), "up", {params.up[0], params.up[1], params.up[2]});
    setObjectAttribute(_camera->getName(), "fov", {params.fov[0]});
    setObjectAttribute(_camera->getName(), "principalPoint", {params.principalPoint[0], params.principalPoint[1]});
}

/*************/
void GuiGlobalView::showAllCalibrationPoints(Camera::CalibrationPointsVisibility showPoints)
{
    if (showPoints == Camera::CalibrationPointsVisibility::switchVisibility)
        _showCalibrationPoints = !_showCalibrationPoints;
    setObjectAttribute(_camera->getName(), "showAllCalibrationPoints", {static_cast<int>(showPoints)});
}

/*************/
void GuiGlobalView::showAllCamerasCalibrationPoints()
{
    if (_camera == _guiCamera)
        _guiCamera->setAttribute("switchDisplayAllCalibration", {});
    else
        setObjectAttribute(_camera->getName(), "switchDisplayAllCalibration", {});
}

/*************/
void GuiGlobalView::colorizeCameraWireframes(bool colorize)
{
    if ((_camera == _guiCamera && colorize) || colorize == _camerasColorizedPreviousValue)
        return;

    _camerasColorizedPreviousValue = colorize;

    if (colorize)
    {
        setObjectsOfType("camera", "colorWireframe", {1.0, 0.0, 1.0, 1.0});
        setObjectAttribute(_camera->getName(), "colorWireframe", {0.0, 1.0, 0.0, 1.0});
    }
    else
    {
        setObjectsOfType("camera", "colorWireframe", {1.0, 1.0, 1.0, 1.0});
    }
}

/*************/
void GuiGlobalView::doCalibration()
{
    CameraParameters params;
    // We keep the current values
    _camera->getAttribute("eye", params.eye);
    _camera->getAttribute("target", params.target);
    _camera->getAttribute("up", params.up);
    _camera->getAttribute("fov", params.fov);
    _camera->getAttribute("principalPoint", params.principalPoint);
    _previousCameraParameters.push_back(params);

    // Calibration
    _camera->setAttribute("calibrate");

    return;
}

/*************/
void GuiGlobalView::hideOtherCameras(bool hide)
{
    if (hide == _camerasHidden)
        return;

    auto cameras = getObjectsPtr(getObjectsOfType("camera"));
    for (auto& cam : cameras)
        if (cam != _camera)
            setObjectAttribute(cam->getName(), "hide", {(int)hide});
    _camerasHidden = hide;
}

/*************/
void GuiGlobalView::processJoystickState()
{
    lock_guard<mutex> lock(_joystickMutex);
    float speed = 1.f;

    // Buttons
    if (_joyButtons.size() >= 4)
    {
        if (_joyButtons[0] == 1 && _joyButtons[0] != _joyButtonsPrevious[0])
        {
            setObjectAttribute(_camera->getName(), "selectPreviousCalibrationPoint", {});
        }
        else if (_joyButtons[1] == 1 && _joyButtons[1] != _joyButtonsPrevious[1])
        {
            setObjectAttribute(_camera->getName(), "selectNextCalibrationPoint", {});
        }
        else if (_joyButtons[2] == 1 && _joyButtons[2] != _joyButtonsPrevious[2])
        {
            _hideCameras = !_camerasHidden;
        }
        else if (_joyButtons[3] == 1 && _joyButtons[3] != _joyButtonsPrevious[3])
        {
            doCalibration();
        }
    }
    if (_joyButtons.size() >= 7)
    {
        if (_joyButtons[4] == 1)
        {
            speed = 0.1f;
        }
        else if (_joyButtons[5] == 1)
        {
            speed = 10.f;
        }
        else if (_joyButtons[6] == 1 && _joyButtons[6] != _joyButtonsPrevious[6])
        {
            setObjectsOfType("camera", "flashBG", {});
        }
    }

    _joyButtonsPrevious = _joyButtons;

    // Axes
    if (_joyAxes.size() >= 2)
    {
        float xValue = _joyAxes[0];
        float yValue = -_joyAxes[1]; // Y axis goes downward for joysticks...

        if (xValue != 0.f || yValue != 0.f)
            setObjectAttribute(_camera->getName(), "moveCalibrationPoint", {xValue * speed, yValue * speed});
    }

    // This prevents issues when disconnecting the joystick
    _joyAxes.clear();
    _joyButtons.clear();
}

/*************/
void GuiGlobalView::processKeyEvents()
{
    if (!ImGui::IsItemHoveredRect())
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeysDown[' '] && io.KeysDownDuration[' '] == 0.0)
    {
        nextCamera();
        return;
    }
    else if (io.KeysDown['A'] && io.KeysDownDuration['A'] == 0.0)
    {
        showAllCalibrationPoints();
        return;
    }
    else if (io.KeysDown['C'] && io.KeysDownDuration['C'] == 0.0)
    {
        doCalibration();
        return;
    }
    else if (io.KeysDown['H'] && io.KeysDownDuration['H'] == 0.0)
    {
        _hideCameras = !_camerasHidden;
        return;
    }
    else if (io.KeysDown['O'] && io.KeysDownDuration['O'] == 0.0)
    {
        showAllCamerasCalibrationPoints();
        return;
    }
    else if (io.KeysDown['V'] && io.KeysDownDuration['V'] == 0.0)
    {
        _camerasColorized = !_camerasColorized;
        return;
    }
    // Reset to the previous camera calibration
    else if (io.KeysDown['Z'] && io.KeysDownDuration['Z'] == 0.0)
    {
        if (io.KeyCtrl)
            revertCalibration();
        return;
    }
    else if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_Tab)] && io.KeysDownDuration[ImGui::GetKeyIndex(ImGuiKey_Tab)] == 0.0)
    {
        if (io.KeyShift)
            setObjectAttribute(_camera->getName(), "selectPreviousCalibrationPoint", {});
        else
            setObjectAttribute(_camera->getName(), "selectNextCalibrationPoint", {});
    }
    // Arrow keys
    else
    {
        float delta = 1.f;
        if (io.KeyShift)
            delta = 0.1f;
        else if (io.KeyCtrl)
            delta = 10.f;

        // Setting the camera locally is needed due to async nature of setObjectAttribute
        if (io.KeysDownDuration[ImGui::GetKeyIndex(ImGuiKey_RightArrow)] > 0.0)
            setObjectAttribute(_camera->getName(), "moveCalibrationPoint", {delta, 0});
        if (io.KeysDownDuration[ImGui::GetKeyIndex(ImGuiKey_LeftArrow)] > 0.0)
            setObjectAttribute(_camera->getName(), "moveCalibrationPoint", {-delta, 0});
        if (io.KeysDownDuration[ImGui::GetKeyIndex(ImGuiKey_DownArrow)] > 0.0)
            setObjectAttribute(_camera->getName(), "moveCalibrationPoint", {0, -delta});
        if (io.KeysDownDuration[ImGui::GetKeyIndex(ImGuiKey_UpArrow)] > 0.0)
            setObjectAttribute(_camera->getName(), "moveCalibrationPoint", {0, delta});

        return;
    }
}

/*************/
void GuiGlobalView::processMouseEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    // Get mouse pos
    ImVec2 mousePos = ImVec2((io.MousePos.x - ImGui::GetCursorScreenPos().x) / _camWidth, -(io.MousePos.y - ImGui::GetCursorScreenPos().y) / _camHeight);

    if (ImGui::IsItemHoveredRect())
    {
        // Vertex selection
        if (io.MouseDown[0])
        {
            // If selected camera is guiCamera, do nothing
            if (_camera == _guiCamera)
                return;

            // Set a calibration point
            if (io.KeyCtrl && io.MouseClicked[0])
            {
                Values position = _camera->pickCalibrationPoint(mousePos.x, mousePos.y);
                if (position.size() == 3)
                    setObjectAttribute(_camera->getName(), "removeCalibrationPoint", {position[0], position[1], position[2]});
            }
            else if (io.KeyShift) // Define the screenpoint corresponding to the selected calibration point
                setObjectAttribute(_camera->getName(), "setCalibrationPoint", {mousePos.x * 2.f - 1.f, mousePos.y * 2.f - 1.f});
            else if (io.MouseClicked[0]) // Add a new calibration point
            {
                Values position = _camera->pickVertexOrCalibrationPoint(mousePos.x, mousePos.y);
                if (position.size() == 3)
                {
                    setObjectAttribute(_camera->getName(), "addCalibrationPoint", {position[0], position[1], position[2]});
                    _previousPointAdded = position;
                }
                else
                    setObjectAttribute(_camera->getName(), "deselectCalibrationPoint", {});
            }
            return;
        }

        // Calibration point set
        if (io.MouseClicked[2])
        {
            float fragDepth = 0.f;
            _newTarget = _camera->pickFragment(mousePos.x, mousePos.y, fragDepth);

            if (fragDepth == 0.f)
                _newTargetDistance = 1.f;
            else
                _newTargetDistance = -fragDepth * 0.1f;
        }

        if (io.MouseWheel != 0)
        {
            Values fov;
            _camera->getAttribute("fov", fov);
            auto camFov = fov[0].as<float>();

            camFov += io.MouseWheel;
            camFov = std::max(2.f, std::min(180.f, camFov));

            if (_camera != _guiCamera)
                setObjectAttribute(_camera->getName(), "fov", {camFov});
            else
                _camera->setAttribute("fov", {camFov});
        }
    }

    // This handles the mouse capture even when the mouse goes outside the view widget, which controls are defined next
    static bool viewCaptured = false;
    if (io.MouseDownDuration[2] > 0.0)
    {
        if (ImGui::IsItemHovered())
            viewCaptured = true;
    }
    else if (viewCaptured)
    {
        viewCaptured = false;
    }

    // View widget controls
    if (viewCaptured)
    {
        // Move the camera
        if (!io.KeyCtrl && !io.KeyShift)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;

            // We reset the up vector. Not ideal, but prevent the camera from being unusable.
            setObjectAttribute(_camera->getName(), "up", {0.0, 0.0, 1.0});
            if (_camera != _guiCamera)
            {
                if (_newTarget.size() == 3)
                    setObjectAttribute(
                        _camera->getName(), "rotateAroundPoint", {dx / 100.f, dy / 100.f, 0, _newTarget[0].as<float>(), _newTarget[1].as<float>(), _newTarget[2].as<float>()});
                else
                    setObjectAttribute(_camera->getName(), "rotateAroundTarget", {dx / 100.f, dy / 100.f, 0});
            }
            else
            {
                if (_newTarget.size() == 3)
                    _camera->setAttribute("rotateAroundPoint", {dx / 100.f, dy / 100.f, 0, _newTarget[0].as<float>(), _newTarget[1].as<float>(), _newTarget[2].as<float>()});
                else
                    _camera->setAttribute("rotateAroundTarget", {dx / 100.f, dy / 100.f, 0});
            }
        }
        // Move the target and the camera (in the camera plane)
        else if (io.KeyShift && !io.KeyCtrl)
        {
            float dx = io.MouseDelta.x * _newTargetDistance;
            float dy = io.MouseDelta.y * _newTargetDistance;
            if (_camera != _guiCamera)
                setObjectAttribute(_camera->getName(), "pan", {-dx / 100.f, dy / 100.f, 0.f});
            else
                _camera->setAttribute("pan", {-dx / 100.f, dy / 100.f, 0});
        }
        else if (!io.KeyShift && io.KeyCtrl)
        {
            float dy = io.MouseDelta.y * _newTargetDistance / 100.f;
            if (_camera != _guiCamera)
                setObjectAttribute(_camera->getName(), "forward", {dy});
            else
                _camera->setAttribute("forward", {dy});
        }
    }
}

/*************/
vector<shared_ptr<Camera>> GuiGlobalView::getCameras()
{
    auto cameras = vector<shared_ptr<Camera>>();

    _guiCamera->setAttribute("size", {ImGui::GetWindowWidth(), ImGui::GetWindowWidth() * 3 / 4});

    auto rtMatrices = getCamerasRTMatrices();
    for (auto& matrix : rtMatrices)
        _guiCamera->drawModelOnce("camera", matrix);
    cameras.push_back(_guiCamera);

    auto listOfCameras = getObjectsPtr(getObjectsOfType("camera"));
    for (auto& camera : listOfCameras)
        cameras.push_back(dynamic_pointer_cast<Camera>(camera));

    return cameras;
}

/*************/
void GuiGlobalView::drawVirtualProbes()
{
    auto rtMatrices = vector<glm::dmat4>();
    auto probes = getObjectsPtr(getObjectsOfType("virtual_probe"));
    for (auto& probe : probes)
        _guiCamera->drawModelOnce("probe", dynamic_pointer_cast<VirtualProbe>(probe)->computeViewMatrix());
}

#pragma clang diagnostic pop

} // end of namespace
