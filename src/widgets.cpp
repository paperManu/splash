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
        _camLabel.setValue(_camera->getName());

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
        switch (g.keyboard().key())
        {
        default:
            return false;
        case ' ': // Switch between scene cameras and guiCamera
            auto scene = _scene.lock();
            vector<CameraPtr> cameras;
            for (auto& obj : scene->_objects)
                if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
                    cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));

            _camera->setAttribute("frame", {0});
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
                _camera->setAttribute("frame", {1});

            return true;
        }
        break;
    case Event::MouseDown:
        {
            vector<Value> position = _camera->pickVertex(g.mouse().xRel() / w, g.mouse().yRel() / h);
            if (position.size() == 3)
                _camera->setCalibrationPoint(position, {g.mouse().xRel() / w, g.mouse().yRel() / h});
            return true;
        }
    case Event::MouseDrag:
        if (g.mouse().middle()) // Drag the window
        {
            move(g.mouse().dx(), g.mouse().dy());
            return false;
        }
        else if (g.mouse().left()) // Move the camera
        {
            float dx = g.mouse().dx();
            float dy = g.mouse().dy();
            _camera->setAttribute("rotateAroundTarget", {dx / 10.f, 0, 0});
            _camera->setAttribute("moveEye", {0, 0, dy / 100.f});
            return false;
        }
        else if (g.mouse().right()) // Move the target
        {
            float dx = g.mouse().dx();
            float dy = g.mouse().dy();
            _camera->setAttribute("moveTarget", {0, 0, dy / 100.f});
            return false;
        }
        break;
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
