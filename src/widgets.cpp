#include "widgets.h"

#include "log.h"
#include "scene.h"
#include "timer.h"

using namespace std;
using namespace glv;

namespace Splash
{

/*************/
void GlvTextBox::onDraw(GLV& g)
{
    try
    {
        draw::color(SPLASH_GLV_TEXTCOLOR);
        draw::lineWidth(SPLASH_GLV_FONTWIDTH);

        string text = getText(*this);

        draw::text(text.c_str(), 4, 4, SPLASH_GLV_FONTSIZE);
    }
    catch (bad_function_call)
    {
        SLog::log << Log::ERROR << "GlvTextBox::" << __FUNCTION__ << " - Draw function is undefined" << Log::endl;
    }
}

/*************/
bool GlvTextBox::onEvent(Event::t e, GLV& g)
{
    switch (e)
    {
    default:
        break;
    case Event::KeyDown:
        SLog::log << Log::DEBUGGING << "Key down: " << (char)g.keyboard().key() << Log::endl;
        return false;
    case Event::MouseDrag:
        if (g.mouse().middle())
        {
            move(g.mouse().dx(), g.mouse().dy());
            return false;
        }
        break;
    case Event::MouseWheel:
        int scrollOffset = _scrollOffset.fetch_add((int)g.mouse().dw());
        return false;
    }

    return true;
}

/*************/
GlvControl::GlvControl()
{
    _selectedObjectName.colors().set(Color(1.0));
    _selectedObjectName.top(8);
    _selectedObjectName.left(8);

    _left = View(Rect(16, 16));
    _left.name("left");
    _right = View(Rect(16, 16));
    _right.name("right");
}

/*************/
void GlvControl::onDraw(GLV& g)
{
    if (!_ready)
    {
        string objName = getNameByIndex();
        changeTarget(objName);

        _left.style(&style());
        _right.style(&style());

        _ready = true;
    }

    // Update all the values currently displayed
    if (g.focusedView() == this || g.focusedView()->parent == this)
    {
        for (int i = 0; i < _properties.size(); ++i)
        {
            auto scene = _scene.lock();
            string objName = getNameByIndex();

            if (!_isDistant)
                scene->_objects[objName]->setAttribute(_properties[i]->getValue(), {_numbers[i]->getValue()});
            else
            {
                scene->_ghostObjects[objName]->setAttribute(_properties[i]->getValue(), {_numbers[i]->getValue()});
                scene->sendMessage("sendAll", {objName, _properties[i]->getValue(), _numbers[i]->getValue()});
            }
        }
    }
    else
    {
        for (int i = 0; i < _properties.size(); ++i)
        {
            auto scene = _scene.lock();
            string objName = getNameByIndex();

            Values values;
            if (!_isDistant)
                scene->_objects[objName]->getAttribute(_properties[i]->getValue(), values);
            else
                scene->_ghostObjects[objName]->getAttribute(_properties[i]->getValue(), values);

            if (values.size() != 0)
                _numbers[i]->setValue(values[0].asFloat());
        }
    }
}

/*************/
bool GlvControl::onEvent(Event::t e, GLV& g)
{
    switch (e)
    {
    default:
        break;
    case Event::MouseDown:
    {
        string viewName = g.focusedView()->name();
        if (viewName == "left")
            _objIndex--;
        else if (viewName == "right")
            _objIndex++;
        else
            return true;

        string objName = getNameByIndex();
        changeTarget(objName);
        return false;
    }
    case Event::MouseDrag:
    {
        // Drag the window
        if (g.mouse().middle()) 
        {
            move(g.mouse().dx(), g.mouse().dy());
            return false;
        }
    }
    }

    return true;
}

/*************/
string GlvControl::getNameByIndex()
{
    auto scene = _scene.lock();
    vector<string> objNames;

    for (auto& o : scene->_objects)
        objNames.push_back(o.first);
    for (auto& o : scene->_ghostObjects)
        objNames.push_back(o.first);
    
    if (objNames.size() == 0)
        return "";

    _objIndex = _objIndex < 0 ? 0 : _objIndex >= objNames.size() ? objNames.size() - 1 : _objIndex;
    return objNames[_objIndex];
}

/*************/
void GlvControl::changeTarget(string name)
{
    // Reset the height of the view
    h = 28;

    auto scene = _scene.lock();

    for (auto& p : _properties)
        p->remove();
    _properties.clear();

    for (auto& n : _numbers)
        n->remove();
    _numbers.clear();

    if (scene->_objects.find(name) == scene->_objects.end() && scene->_ghostObjects.find(name) == scene->_ghostObjects.end())
        return;

    _selectedObjectName.setValue(name);

    if (scene->_ghostObjects.find(name) != scene->_ghostObjects.end())
        _isDistant = true;
    else
        _isDistant = false;

    // We don't allow modification of Window parameters yet
    if ((!_isDistant && scene->_objects[name]->getType() == "window") || (_isDistant && scene->_ghostObjects[name]->getType() == "window"))
        return;

    map<string, Values> attribs;
    if (!_isDistant)
        attribs = scene->_objects[name]->getAttributes();
    else
        attribs = scene->_ghostObjects[name]->getAttributes();

    int position = 28;
    for (auto& a : attribs)
    {
        // Only show single values properties
        if (a.second.size() > 1)
            continue;
        glv::Label* label = new glv::Label();
        label->top(position);
        label->left(8);
        label->setValue(a.first);
        _properties.push_back(label);
        *this << *label;

        glv::NumberDialer* number = new glv::NumberDialer();
        number->style(&style());
        number->top(position);
        number->right(w);
        number->resize(2, 2);
        number->setValue(a.second[0].asFloat());
        _numbers.push_back(number);
        *this << *number;

        position += 16;
    }

    h = position;
    _titlePlacer = Placer(*this, Direction::E, Place::TL, 8, 8, 4);
    _titlePlacer << _left << _right << _selectedObjectName;
}

/*************/
GlvGlobalView::GlvGlobalView()
{
    _camLabel.colors().set(Color(1.0));
    _camLabel.top(8);
    _camLabel.left(8);

    *this << _camLabel;
}

/*************/
void GlvGlobalView::onDraw(GLV& g)
{
    if (_camera.get() == nullptr)
    {
        draw::color(0.0, 1.0, 0.0, 1.0);
    }
    else
    {
        // Resize if needed
        Values size;
        _camera->getAttribute("size", size);
        if (size[0].asInt() != 0 && size[1].asInt() != 0 && (size[0].asInt() != w || size[1].asInt() != h))
            h = _baseWidth * size[1].asInt() / size[0].asInt();

        Values fov;
        _camera->getAttribute("fov", fov);
        _camLabel.setValue(_camera->getName() + " - " + fov[0].asString() + "°");

        float vertcoords[] = {0,0, 0,height(), width(),0, width(),height()};
        float texcoords[] = {0,1, 0,0, 1,1, 1,0};

        glGetError();
        glEnable(GL_TEXTURE_2D);

        draw::color(1.0, 1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, _camera->getTextures()[0]->getTexId());
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, vertcoords);
        glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDisable(GL_TEXTURE_2D);
    }
}

/*************/
bool GlvGlobalView::onEvent(Event::t e, GLV& g)
{
    switch (e)
    {
    default:
        break;
    case Event::KeyDown:
    {
        auto key = g.keyboard().key();
        // Switch between scene cameras and guiCamera
        if (key ==  ' ')
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
                scene->sendMessage("sendAll", {cam->getName(), "hide", 0});

            scene->sendMessage("sendAll", {_camera->getName(), "frame", 0});
            scene->sendMessage("sendAll", {_camera->getName(), "displayCalibration", 0});

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
                scene->sendMessage("sendAll", {_camera->getName(), "frame", 1});
                scene->sendMessage("sendAll", {_camera->getName(), "displayCalibration", 1});
            }

            return false;
        }
        // Show all the calibration points for the selected camera
        else if (key == 'A')
        {
            auto scene = _scene.lock();
            scene->sendMessage("sendAll", {_camera->getName(), "switchShowAllCalibrationPoints"});
            return false;
        }
        else if (key == 'C')
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

                    scene->sendMessage("sendAll", sendValues);
                }
            }

            return false;
        }
        else if (key == 'H')
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
                        scene->sendMessage("sendAll", {cam->getName(), "hide", 1});
                _camerasHidden = true;
            }
            else
            {
                for (auto& cam : cameras)
                    if (cam.get() != _camera.get())
                        scene->sendMessage("sendAll", {cam->getName(), "hide", 0});
                _camerasHidden = false;
            }
        }
        // Reset to the previous camera calibration
        else if (key == 'R')
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
                    scene->sendMessage("sendAll", {_camera->getName(), "eye", _eye[0], _eye[1], _eye[2]});
                    scene->sendMessage("sendAll", {_camera->getName(), "target", _target[0], _target[1], _target[2]});
                    scene->sendMessage("sendAll", {_camera->getName(), "up", _up[0], _up[1], _up[2]});
                    scene->sendMessage("sendAll", {_camera->getName(), "fov", _fov[0]});
                    scene->sendMessage("sendAll", {_camera->getName(), "principalPoint", _principalPoint[0], _principalPoint[1]});
                }
        }
        // Arrow keys
        else if (key >= 262 && key <= 265)
        {
            auto scene = _scene.lock();

            float delta = 1.f;
            if (g.keyboard().shift())
                delta = 0.1f;
            else if (g.keyboard().ctrl())
                delta = 10.f;
                
            if (key == 262)
                scene->sendMessage("sendAll", {_camera->getName(), "moveCalibrationPoint", delta, 0});
            else if (key == 263)
                scene->sendMessage("sendAll", {_camera->getName(), "moveCalibrationPoint", -delta, 0});
            else if (key == 264)
                scene->sendMessage("sendAll", {_camera->getName(), "moveCalibrationPoint", 0, -delta});
            else if (key == 265)
                scene->sendMessage("sendAll", {_camera->getName(), "moveCalibrationPoint", 0, delta});
            return false;
        }
        break;
    }
    case Event::MouseDown:
    {
        // If selected camera is guiCamera, do nothing
        if (_camera == _guiCamera)
            return false;

        // Set a calibration point
        if (g.mouse().left()) 
        {
            auto scene = _scene.lock();
            if (g.keyboard().ctrl())
            {
                auto scene = _scene.lock();
                Values position = _camera->pickCalibrationPoint(g.mouse().xRel() / w, 1.f - g.mouse().yRel() / h);
                if (position.size() == 3)
                    scene->sendMessage("sendAll", {_camera->getName(), "removeCalibrationPoint", position[0], position[1], position[2]});
            }
            else if (g.keyboard().shift()) // Define the screenpoint corresponding to the selected calibration point
                scene->sendMessage("sendAll", {_camera->getName(), "setCalibrationPoint", (g.mouse().xRel() / w * 2.f) - 1.f, 1.f - (g.mouse().yRel() / h) * 2.f});
            else // Add a new calibration point
            {
                Values position = _camera->pickVertex(g.mouse().xRel() / w, 1.f - g.mouse().yRel() / h);
                if (position.size() == 3)
                {
                    scene->sendMessage("sendAll", {_camera->getName(), "addCalibrationPoint", position[0], position[1], position[2]});
                    _previousPointAdded = position;
                }
                else
                    scene->sendMessage("sendAll", {_camera->getName(), "deselectCalibrationPoint"});
            }
        }
        return false;
    }
    case Event::MouseUp:
    {
        _beginDrag = true; // We reset the beginDrag flag
        // Pure precautions
        _previousPointAdded.clear();
        return false;
    }
    case Event::MouseDrag:
    {
        // Drag the window
        if (g.mouse().left())
        {
            if (g.keyboard().shift())
            {
                auto scene = _scene.lock();
                scene->sendMessage("sendAll", {_camera->getName(), "setCalibrationPoint", (g.mouse().xRel() / w * 2.f) - 1.f, 1.f - (g.mouse().yRel() / h) * 2.f});
            }
        }
        else if (g.mouse().middle()) 
        {
            move(g.mouse().dx(), g.mouse().dy());
        }
        else if (g.mouse().right()) 
        {
            // Move the camera
            if (!g.keyboard().ctrl() && !g.keyboard().shift())
            {
                float dx = g.mouse().dx();
                float dy = g.mouse().dy();
                auto scene = _scene.lock();

                // If the user clicked on an object, we set a new target on this object
                static Values newTarget;
                if (_beginDrag)
                    newTarget = _camera->pickFragment(g.mouse().xRel() / w, 1.f - g.mouse().yRel() / h);

                if (_camera != _guiCamera)
                {
                    if (newTarget.size() == 3)
                        scene->sendMessage("sendAll", {_camera->getName(), "rotateAroundPoint", dx / 100.f, 0, 0, newTarget[0].asFloat(), newTarget[1].asFloat(), newTarget[2].asFloat()});
                    else
                        scene->sendMessage("sendAll", {_camera->getName(), "rotateAroundTarget", dx / 100.f, 0, 0});
                    scene->sendMessage("sendAll", {_camera->getName(), "moveEye", 0, 0, dy / 100.f});
                }
                else
                {
                    if (newTarget.size() == 3)
                        _camera->setAttribute("rotateAroundPoint", {dx / 100.f, 0, 0, newTarget[0].asFloat(), newTarget[1].asFloat(), newTarget[2].asFloat()});
                    else
                        _camera->setAttribute("rotateAroundTarget", {dx / 100.f, 0, 0});
                    _camera->setAttribute("moveEye", {0, 0, dy / 100.f});
                }
            }
            // Move the target and the camera (in the camera plane)
            else if (g.keyboard().shift() && !g.keyboard().ctrl())
            {
                float dx = g.mouse().dx();
                float dy = g.mouse().dy();
                auto scene = _scene.lock();
                if (_camera != _guiCamera)
                    scene->sendMessage("sendAll", {_camera->getName(), "pan", dx / 100.f, 0, dy / 100.f});
                else
                    _camera->setAttribute("pan", {dx / 100.f, 0, dy / 100.f});
            }
            else if (!g.keyboard().shift() && g.keyboard().ctrl())
            {
                float dy = g.mouse().dy() / 100.f;
                auto scene = _scene.lock();
                if (_camera != _guiCamera)
                    scene->sendMessage("sendAll", {_camera->getName(), "forward", dy});
                else
                    _camera->setAttribute("forward", {dy});
            }
        }
        else
            break;

        _beginDrag = false;
        return false;
    }
    case Event::MouseWheel:
    {
        Values fov;
        _camera->getAttribute("fov", fov);
        float camFov = fov[0].asFloat();

        camFov += g.mouse().dw();
        camFov = std::max(2.f, std::min(180.f, camFov));

        auto scene = _scene.lock();
        if (_camera != _guiCamera)
            scene->sendMessage("sendAll", {_camera->getName(), "fov", camFov});
        else
            _camera->setAttribute("fov", {camFov});

        return false;
    }
    }

    return true;
}

/*************/
void GlvGlobalView::setCamera(CameraPtr cam)
{
    if (cam.get() != nullptr)
    {
        _camera = cam;
        _guiCamera = cam;
        _camera->setAttribute("size", {width(), height()});
    }
}

/*************/
GlvGraph::GlvGraph()
{
    _plotFunction = PlotFunction1D(Color(1), 2, draw::LineStrip);
    _plot.add(_plotFunction);
    _plot.data().resize(1, _maxHistoryLength);
    _plot.range(0.0, _plot.data().size(1), 0).range(0.0, 20.0, 1);
    _plot.major(20, 0).minor(4, 0).major(5, 1).minor(0, 1);
    _plot.disable(Controllable);
    *this << _plot;

    _graphLabel.setValue("Graph").colors().set(Color(1.0));
    _graphLabel.top(8);
    _graphLabel.left(8);

    _scaleLabel.top(8);
    _scaleLabel.right(w - 24);
    _plot << _graphLabel;
    _plot << _scaleLabel;
}

/*************/
void GlvGraph::onDraw(GLV& g)
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

    unsigned int target = _target;
    if (target >= _durationGraph.size())
        _target = target = 0;

    auto durationIt = _durationGraph.begin();
    for (int i = 0; i < target; ++i)
        durationIt++;

    float maxValue {0.f};
    int index = 0;
    _graphLabel.setValue((*durationIt).first);
    for (auto v : (*durationIt).second)
    {
        maxValue = std::max((float)v * 0.001f, maxValue);
        _plot.data().assign((float)v * 0.001f, index);
        index++;
    }

    maxValue = ceil(maxValue * 0.1f) * 10.f;
    _plot.range(0.0, maxValue, 1);

    _scaleLabel.setValue(to_string((int)maxValue) + " ms");
    _scaleLabel.right(w - 8);
}

/*************/
bool GlvGraph::onEvent(Event::t e, GLV& g)
{
    switch (e)
    {
    default:
        break;
    case Event::KeyDown:
        if ((char)g.keyboard().key() == 'N')
        {
            _target++;
            return false;
        }
        else if ((char)g.keyboard().key() == 'P')
        {
            _target--;
            return false;
        }
        break;
    case Event::MouseDrag:
        if (g.mouse().middle())
        {
            move(g.mouse().dx(), g.mouse().dy());
            return false;
        }
        break;
    }

    return true;
}

/*************/
void GlvGraph::onResize(glv::space_t dx, glv::space_t dy)
{
    _style = style();
    _plot.style(&_style);
    _plot.width(w);
    _plot.height(h);
    _plot.range(0.0, _plot.data().size(1), 0).range(0.0, 20.0, 1);
}

} // end of namespace
