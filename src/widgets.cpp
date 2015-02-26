#include "widgets.h"

#include "log.h"
#include "scene.h"
#include "timer.h"

#include <imgui.h>

using namespace std;

namespace Splash
{

/*************/
GuiWidget::GuiWidget(string name)
{
    _name = name;
}

/*************/
GuiTextBox::GuiTextBox(string name)
    : GuiWidget(name)
{
}

/*************/
void GuiTextBox::render()
{
    if (getText)
    {
        if (ImGui::CollapsingHeader(_name.c_str()))
            ImGui::Text(getText().c_str());
    }
}

/*************/
void GuiControl::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        // Select the object the control
        {
            vector<string> objectNames = getObjectNames();
            vector<const char*> items;
            for (auto& name : objectNames)
                items.push_back(name.c_str());
            ImGui::Combo("Selected object", &_targetIndex, items.data(), items.size());
        }

        // Initialize the target
        if (_targetIndex >= 0)
        {
            vector<string> objectNames = getObjectNames();
            if (objectNames.size() <= _targetIndex)
                return;
            _targetObjectName = objectNames[_targetIndex];
        }

        if (_targetObjectName == "")
            return;

        auto scene = _scene.lock();

        bool isDistant = false;
        if (scene->_ghostObjects.find(_targetObjectName) != scene->_ghostObjects.end())
            isDistant = true;

        map<string, Values> attributes;
        if (!isDistant)
            attributes = scene->_objects[_targetObjectName]->getAttributes();
        else
            attributes = scene->_ghostObjects[_targetObjectName]->getAttributes();

        for (auto& attr : attributes)
        {
            if (attr.second.size() > 4 || attr.second.size() == 0)
                continue;

            if (attr.second[0].getType() == Value::Type::i
                || attr.second[0].getType() == Value::Type::f)
            {
                int precision = 0;
                if (attr.second[0].getType() == Value::Type::f)
                    precision = 2;

                if (attr.second.size() == 1)
                {
                    float tmp = attr.second[0].asFloat();
                    float step = attr.second[0].getType() == Value::Type::f ? 0.01 * tmp : 1.f;
                    if (ImGui::InputFloat(attr.first.c_str(), &tmp, step, step, precision))
                    {
                        if (!isDistant)
                            scene->_objects[_targetObjectName]->setAttribute(attr.first, {tmp});
                        else
                        {
                            scene->_ghostObjects[_targetObjectName]->setAttribute(attr.first, {tmp});
                            scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp});
                        }
                    }
                }
                else if (attr.second.size() == 2)
                {
                    vector<float> tmp;
                    tmp.push_back(attr.second[0].asFloat());
                    tmp.push_back(attr.second[1].asFloat());
                    if (ImGui::InputFloat2(attr.first.c_str(), tmp.data(), precision))
                    {
                        if (!isDistant)
                            scene->_objects[_targetObjectName]->setAttribute(attr.first, {tmp[0], tmp[1]});
                        else
                        {
                            scene->_ghostObjects[_targetObjectName]->setAttribute(attr.first, {tmp[0], tmp[1]});
                            scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp[0], tmp[1]});
                        }
                    }
                }
                else if (attr.second.size() == 3)
                {
                    vector<float> tmp;
                    tmp.push_back(attr.second[0].asFloat());
                    tmp.push_back(attr.second[1].asFloat());
                    tmp.push_back(attr.second[2].asFloat());
                    if (ImGui::InputFloat3(attr.first.c_str(), tmp.data(), precision))
                    {
                        if (!isDistant)
                            scene->_objects[_targetObjectName]->setAttribute(attr.first, {tmp[0], tmp[1], tmp[2]});
                        else
                        {
                            scene->_ghostObjects[_targetObjectName]->setAttribute(attr.first, {tmp[0], tmp[1], tmp[2]});
                            scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp[0], tmp[1], tmp[2]});
                        }
                    }
                }
                else if (attr.second.size() == 4)
                {
                    vector<float> tmp;
                    tmp.push_back(attr.second[0].asFloat());
                    tmp.push_back(attr.second[1].asFloat());
                    tmp.push_back(attr.second[2].asFloat());
                    tmp.push_back(attr.second[3].asFloat());
                    if (ImGui::InputFloat3(attr.first.c_str(), tmp.data(), precision))
                    {
                        if (!isDistant)
                            scene->_objects[_targetObjectName]->setAttribute(attr.first, {tmp[0], tmp[1], tmp[2], tmp[3]});
                        else
                        {
                            scene->_ghostObjects[_targetObjectName]->setAttribute(attr.first, {tmp[0], tmp[1], tmp[2], tmp[3]});
                            scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp[0], tmp[1], tmp[2], tmp[3]});
                        }
                    }
                }
            }
            else if (attr.second.size() == 1 && attr.second[0].getType() == Value::Type::v)
            {
                // We skip anything that looks like a vector / matrix
                // (for usefulness reasons...)
                Values values = attr.second[0].asValues();
                if (values.size() > 16)
                {
                    if (values[0].getType() == Value::Type::i || values[0].getType() == Value::Type::f)
                    {
                        float minValue = numeric_limits<float>::max();
                        float maxValue = numeric_limits<float>::min();
                        vector<float> samples;
                        for (auto& v : values)
                        {
                            float value = v.asFloat();
                            maxValue = std::max(value, maxValue);
                            minValue = std::min(value, minValue);
                            samples.push_back(value);
                        }
                        
                        ImGui::PlotLines(attr.first.c_str(), samples.data(), samples.size(), samples.size(), ("[" + to_string(minValue) + ", " + to_string(maxValue) + "]").c_str(), minValue, maxValue, ImVec2(0, 100));
                    }
                }
            }
            else if (attr.second[0].getType() == Value::Type::s)
            {
                for (auto& v : attr.second)
                {
                    string tmp = v.asString();
                    ImGui::Text(tmp.c_str());
                }
            }
        }
    }
}

/*************/
vector<string> GuiControl::getObjectNames()
{
    auto scene = _scene.lock();
    vector<string> objNames;

    for (auto& o : scene->_objects)
        objNames.push_back(o.first);
    for (auto& o : scene->_ghostObjects)
        objNames.push_back(o.first);

    return objNames;
}

/*************/
GuiGlobalView::GuiGlobalView(string name)
    : GuiWidget(name)
{
}

/*************/
void GuiGlobalView::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        if (_camera != nullptr)
        {
            _camera->render();

            Values size;
            _camera->getAttribute("size", size);

            double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
            ImVec2 winSize = ImGui::GetWindowSize();
            int w = std::max(400.0, winSize.x - 4 * leftMargin);
            int h = w * size[1].asInt() / size[0].asInt();

            _camWidth = w;
            _camHeight = h;

            if (ImGui::Button("Next camera"))
                nextCamera();
            ImGui::SameLine();
            if (ImGui::Button("Hide other cameras"))
                switchHideOtherCameras();
            ImGui::SameLine();
            if (ImGui::Button("Show all points"))
                showAllCalibrationPoints();
            ImGui::SameLine();
            if (ImGui::Button("Calibrate camera"))
                doCalibration();

            ImGui::Text(("Current camera: " + _camera->getName()).c_str());

            ImGui::Image((void*)(intptr_t)_camera->getTextures()[0]->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));
            if (ImGui::IsItemHovered())
            {
                _noMove = true;
                processKeyEvents();
                processMouseEvents();
            }
            else
                _noMove = false;
        }
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
void GuiGlobalView::setCamera(CameraPtr cam)
{
    if (cam != nullptr)
    {
        _camera = cam;
        _guiCamera = cam;
        _camera->setAttribute("size", {800, 600});
    }
}

/*************/
void GuiGlobalView::nextCamera()
{
    auto scene = _scene.lock();
    vector<CameraPtr> cameras;
    for (auto& obj : scene->_objects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& obj : scene->_ghostObjects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));

    // Ensure that all cameras are shown
    _camerasHidden = false;
    for (auto& cam : cameras)
        scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 0});

    scene->sendMessageToWorld("sendAll", {_camera->getName(), "frame", 0});
    scene->sendMessageToWorld("sendAll", {_camera->getName(), "displayCalibration", 0});

    if (cameras.size() == 0)
        _camera = _guiCamera;
    else if (_camera == _guiCamera)
        _camera = cameras[0];
    else
    {
        for (int i = 0; i < cameras.size(); ++i)
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
        scene->sendMessageToWorld("sendAll", {_camera->getName(), "frame", 1});
        scene->sendMessageToWorld("sendAll", {_camera->getName(), "displayCalibration", 1});
    }

    return;
}

/*************/
void GuiGlobalView::showAllCalibrationPoints()
{
    auto scene = _scene.lock();
    scene->sendMessageToWorld("sendAll", {_camera->getName(), "switchShowAllCalibrationPoints"});
}

/*************/
void GuiGlobalView::doCalibration()
{
     // We keep the current values
    _camera->getAttribute("eye", _eye);
    _camera->getAttribute("target", _target);
    _camera->getAttribute("up", _up);
    _camera->getAttribute("fov", _fov);
    _camera->getAttribute("principalPoint", _principalPoint);

    // Calibration
    _camera->doCalibration();

    bool isDistant {false};
    auto scene = _scene.lock();
    for (auto& obj : scene->_ghostObjects)
        if (_camera->getName() == obj.second->getName())
            isDistant = true;
    
    if (isDistant)
    {
        vector<string> properties {"eye", "target", "up", "fov", "principalPoint"};
        auto scene = _scene.lock();
        for (auto& p : properties)
        {
            Values values;
            _camera->getAttribute(p, values);

            Values sendValues {_camera->getName(), p};
            for (auto& v : values)
                sendValues.push_back(v);

            scene->sendMessageToWorld("sendAll", sendValues);
        }
    }

    return;
}

/*************/
void GuiGlobalView::switchHideOtherCameras()
{
    auto scene = _scene.lock();
    vector<CameraPtr> cameras;
    for (auto& obj : scene->_objects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& obj : scene->_ghostObjects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));

    if (!_camerasHidden)
    {
        for (auto& cam : cameras)
            if (cam.get() != _camera.get())
                scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 1});
        _camerasHidden = true;
    }
    else
    {
        for (auto& cam : cameras)
            if (cam.get() != _camera.get())
                scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 0});
        _camerasHidden = false;
    }
}

/*************/
void GuiGlobalView::processKeyEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeysDown[' '] && io.KeysDownTime[' '] == 0.0)
    {
        nextCamera();
        return;
    }
    else if (io.KeysDown['A'] && io.KeysDownTime['A'] == 0.0)
    {
        showAllCalibrationPoints();
        return;
    }
    else if (io.KeysDown['C'] && io.KeysDownTime['C'] == 0.0)
    {
        doCalibration();
        return;
    }
    else if (io.KeysDown['H'] && io.KeysDownTime['H'] == 0.0)
    {
        switchHideOtherCameras();
        return;
    }
    // Reset to the previous camera calibration
    else if (io.KeysDown['R'] && io.KeysDownTime['R'] == 0.0)
    {
        _camera->setAttribute("eye", _eye);
        _camera->setAttribute("target", _target);
        _camera->setAttribute("up", _up);
        _camera->setAttribute("fov", _fov);
        _camera->setAttribute("principalPoint", _principalPoint);

        auto scene = _scene.lock();
        for (auto& obj : scene->_ghostObjects)
            if (_camera->getName() == obj.second->getName())
            {
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "eye", _eye[0], _eye[1], _eye[2]});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "target", _target[0], _target[1], _target[2]});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "up", _up[0], _up[1], _up[2]});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "fov", _fov[0]});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "principalPoint", _principalPoint[0], _principalPoint[1]});
            }
    }
    // Arrow keys
    else
    {
        auto scene = _scene.lock();

        float delta = 1.f;
        if (io.KeyShift)
            delta = 0.1f;
        else if (io.KeyCtrl)
            delta = 10.f;
            
        if (io.KeysDown[262])
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", delta, 0});
        if (io.KeysDown[263])
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", -delta, 0});
        if (io.KeysDown[264])
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", 0, -delta});
        if (io.KeysDown[265])
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", 0, delta});
        return;
    }
}

/*************/
void GuiGlobalView::processMouseEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    // Get mouse pos
    ImVec2 mousePos = ImVec2((io.MousePos.x - ImGui::GetCursorScreenPos().x) / _camWidth,
                             -(io.MousePos.y - ImGui::GetCursorScreenPos().y) / _camHeight);

    if (io.MouseDown[0])
    {
        // If selected camera is guiCamera, do nothing
        if (_camera == _guiCamera)
            return;

        // Set a calibration point
        auto scene = _scene.lock();
        if (io.KeyCtrl && io.MouseClicked[0])
        {
            auto scene = _scene.lock();
            Values position = _camera->pickCalibrationPoint(mousePos.x, mousePos.y);
            if (position.size() == 3)
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "removeCalibrationPoint", position[0], position[1], position[2]});
        }
        else if (io.KeyShift) // Define the screenpoint corresponding to the selected calibration point
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "setCalibrationPoint", mousePos.x * 2.f - 1.f, mousePos.y * 2.f - 1.f});
        else if (io.MouseClicked[0]) // Add a new calibration point
        {
            Values position = _camera->pickVertex(mousePos.x, mousePos.y);
            if (position.size() == 3)
            {
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "addCalibrationPoint", position[0], position[1], position[2]});
                _previousPointAdded = position;
            }
            else
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "deselectCalibrationPoint"});
        }
        return;
    }
    if (io.MouseClicked[1])
    {
        _newTarget = _camera->pickFragment(mousePos.x, mousePos.y);
    }
    if (io.MouseDownTime[1] > 0.0) 
    {
        // Move the camera
        if (!io.KeyCtrl && !io.KeyShift)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;
            auto scene = _scene.lock();

            if (_camera != _guiCamera)
            {
                if (_newTarget.size() == 3)
                    scene->sendMessageToWorld("sendAll", {_camera->getName(), "rotateAroundPoint", dx / 100.f, 0, 0, _newTarget[0].asFloat(), _newTarget[1].asFloat(), _newTarget[2].asFloat()});
                else
                    scene->sendMessageToWorld("sendAll", {_camera->getName(), "rotateAroundTarget", dx / 100.f, 0, 0});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveEye", 0, 0, dy / 100.f});
            }
            else
            {
                if (_newTarget.size() == 3)
                    _camera->setAttribute("rotateAroundPoint", {dx / 100.f, 0, 0, _newTarget[0].asFloat(), _newTarget[1].asFloat(), _newTarget[2].asFloat()});
                else
                    _camera->setAttribute("rotateAroundTarget", {dx / 100.f, 0, 0});
                _camera->setAttribute("moveEye", {0, 0, dy / 100.f});
            }
        }
        // Move the target and the camera (in the camera plane)
        else if (io.KeyShift && !io.KeyCtrl)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;
            auto scene = _scene.lock();
            if (_camera != _guiCamera)
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "pan", dx / 100.f, 0, dy / 100.f});
            else
                _camera->setAttribute("pan", {dx / 100.f, 0, dy / 100.f});
        }
        else if (!io.KeyShift && io.KeyCtrl)
        {
            float dy = io.MouseDelta.y / 100.f;
            auto scene = _scene.lock();
            if (_camera != _guiCamera)
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "forward", dy});
            else
                _camera->setAttribute("forward", {dy});
        }
    }
    if (io.MouseWheel != 0)
    {
        Values fov;
        _camera->getAttribute("fov", fov);
        float camFov = fov[0].asFloat();

        camFov += io.MouseWheel;
        camFov = std::max(2.f, std::min(180.f, camFov));

        auto scene = _scene.lock();
        if (_camera != _guiCamera)
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "fov", camFov});
        else
            _camera->setAttribute("fov", {camFov});
    }
}

/*************/
void GuiGraph::render()
{
    map<string, unsigned long long> durationMap = STimer::timer.getDurationMap();

    for (auto& t : durationMap)
    {
        if (_durationGraph.find(t.first) == _durationGraph.end())
            _durationGraph[t.first] = deque<unsigned long long>({t.second});
        else
        {
            if (_durationGraph[t.first].size() == _maxHistoryLength)
                _durationGraph[t.first].pop_front();
            _durationGraph[t.first].push_back(t.second);
        }
    }

    if (_durationGraph.size() == 0)
        return;

    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        for (auto& duration : _durationGraph)
        {
            float maxValue {0.f};
            vector<float> values;
            for (auto& v : duration.second)
            {
                maxValue = std::max((float)v * 0.001f, maxValue);
                values.push_back((float)v * 0.001f);
            }

            maxValue = ceil(maxValue * 0.1f) * 10.f;

            ImGui::PlotLines(duration.first.c_str(), values.data(), values.size(), values.size(), (to_string((int)maxValue) + "ms").c_str(), 0.f, maxValue, ImVec2(0, 80));
        }
    }
}

/*************/
void GuiColorCalibration::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto scene = _scene.lock();
        
        float step = 1.f;
        if (ImGui::InputFloat("Color samples", &_colorCurveSamples, step, step, 0))
        {
            _colorCurveSamples = std::max(3.f, _colorCurveSamples);
            scene->sendMessageToWorld("calibrateColorSetParameter", {"colorSamples", round(_colorCurveSamples)});
        }
        step = 0.1f;
        if (ImGui::InputFloat("Detection threshold", &_displayDetectionThreshold, step, step, 1))
        {
            scene->sendMessageToWorld("calibrateColorSetParameter", {"detectionThresholdFactor", _displayDetectionThreshold});
        }
        step = 1.f;
        if (ImGui::InputFloat("Image per HDR", &_imagePerHDR, step, step, 0))
        {
            _imagePerHDR = std::max(1.f, _imagePerHDR);
            scene->sendMessageToWorld("calibrateColorSetParameter", {"imagePerHDR", round(_imagePerHDR)});
        }
        step = 0.1f;
        if (ImGui::InputFloat("HDR step", &_hdrStep, step, step, 1))
        {
            _hdrStep = std::max(0.3f, _hdrStep);
            scene->sendMessageToWorld("calibrateColorSetParameter", {"hdrStep", _hdrStep});
        }
    }
}

} // end of namespace
