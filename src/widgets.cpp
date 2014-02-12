#include "widgets.h"
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
        SLog::log << Log::MESSAGE << "Key down: " << (char)g.keyboard().key() << Log::endl;
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
        vector<Value> size;
        _camera->getAttribute("size", size);
        if (size[0].asInt() != w || size[1].asInt() != h)
            h = _baseWidth * size[1].asInt() / size[0].asInt();

        vector<Value> fov;
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

            _camera->setAttribute("frame", {0});
            _camera->setAttribute("displayCalibration", {0});

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
                _camera->setAttribute("frame", {1});
                _camera->setAttribute("displayCalibration", {1});
            }

            return false;
        }
        // Show all the calibration points for the selected camera
        else if (key == 'A')
        {
           _camera->setAttribute("switchShowAllCalibrationPoints", {});
            return false;
        }
        else if (key == 'C')
        {
             // We keep the current values
            _camera->getAttribute("eye", _eye);
            _camera->getAttribute("target", _target);
            _camera->getAttribute("up", _up);

            // Calibration
            _camera->doCalibration();
            return false;
        }
        // Reset to the previous camera calibration
        else if (key == 'R')
        {
            _camera->setAttribute("eye", _eye);
            _camera->setAttribute("target", _target);
            _camera->setAttribute("up", _up);
        }
        // Switch the rendering to textured
        else if (key == 'T') 
        {
            _camera->setAttribute("wireframe", {0});
            return false;
        }
        // Switch the rendering to wireframe
        else if (key == 'W') 
        {
            _camera->setAttribute("wireframe", {1});
            return false;
        }
        // Arrow keys
        else if (key >= 262 && key <= 265)
        {
            float delta = 1.f;
            if (g.keyboard().shift())
                delta = 0.1f;
                
            if (key == 262)
                _camera->moveCalibrationPoint(delta, 0);
            else if (key == 263)
                _camera->moveCalibrationPoint(-delta, 0);
            else if (key == 264)
                _camera->moveCalibrationPoint(0, -delta);
            else if (key == 265)
                _camera->moveCalibrationPoint(0, delta);
            return false;
        }
        else
        {
            return true;
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
            if (!g.keyboard().shift()) // Add a new calibration point
            {
                vector<Value> position = _camera->pickVertex(g.mouse().xRel() / w, 1.f - g.mouse().yRel() / h);
                if (position.size() == 3)
                    _camera->addCalibrationPoint(position);
                else
                    _camera->deselectCalibrationPoint();
            }
            else // Define the screenpoint corresponding to the selected calibration point
                _camera->setCalibrationPoint({(g.mouse().xRel() / w * 2.f) - 1.f, 1.f - (g.mouse().yRel() / h) * 2.f});
        }
        else if (g.mouse().right()) // Remove a calibration point
        {
            if (g.keyboard().shift())
            {
                vector<Value> position = _camera->pickCalibrationPoint(g.mouse().xRel() / w, 1.f - g.mouse().yRel() / h);
                if (position.size() == 3)
                    _camera->removeCalibrationPoint(position);
            }
            else
            {
                vector<Value> position = _camera->pickVertex(g.mouse().xRel() / w, 1.f - g.mouse().yRel() / h);
                if (position.size() == 3)
                    _camera->removeCalibrationPoint(position);
            }
        }
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
        // Move the camera
        else if (g.mouse().left()) 
        {
            float dx = g.mouse().dx();
            float dy = g.mouse().dy();
            _camera->setAttribute("rotateAroundTarget", {dx / 10.f, 0, 0});
            _camera->setAttribute("moveEye", {0, 0, dy / 100.f});
            return false;
        }
        // Move the target
        else if (g.mouse().right()) 
        {
            float dx = g.mouse().dx();
            float dy = g.mouse().dy();
            _camera->setAttribute("moveTarget", {0, 0, dy / 100.f});
            return false;
        }
        break;
    }
    case Event::MouseWheel:
    {
        vector<Value> fov;
        _camera->getAttribute("fov", fov);
        float camFov = fov[0].asFloat();

        camFov += g.mouse().dw();
        camFov = std::max(2.f, std::min(180.f, camFov));
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
        if ((char)g.keyboard().key() == ' ')
        {
            _target++;
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
