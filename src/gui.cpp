#include "gui.h"

#include "camera.h"
#include "log.h"
#include "object.h"
#include "scene.h"
#include "texture.h"
#include "timer.h"
#include "threadpool.h"

using namespace std;
using namespace glv;
using namespace OIIO_NAMESPACE;

namespace Splash
{

/*************/
Gui::Gui(GlWindowPtr w, SceneWeakPtr s)
{
    _type = "gui";

    auto scene = s.lock();
    if (w.get() == nullptr || scene.get() == nullptr)
        return;

    _scene = s;
    _window = w;
    if (!_window->setAsCurrentContext()) 
    	 SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    glGetError();
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    {
        TexturePtr texture = make_shared<Texture>();
        texture->reset(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        _depthTexture = move(texture);
        _depthTexture->setAttribute("resizable", {1});
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    }

    {
        TexturePtr texture = make_shared<Texture>();
        texture->reset(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
        _outTexture = move(texture);
        _outTexture->setAttribute("resizable", {1});
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outTexture->getTexId(), 0);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while initializing framebuffer object: " << status << Log::endl;
    else
        SLog::log << Log::MESSAGE << "Gui::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    _window->releaseContext();

    // Create the default GUI camera
    scene->_mainWindow->setAsCurrentContext();
    _guiCamera = make_shared<Camera>(s);
    _guiCamera->setName("guiCamera");
    _guiCamera->setAttribute("eye", {2.0, 2.0, 0.0});
    _guiCamera->setAttribute("target", {0.0, 0.0, 0.5});
    _guiCamera->setAttribute("size", {640, 480});
    scene->_mainWindow->releaseContext();

    // Intialize the GUI widgets
    initGLV(_width, _height);

    registerAttributes();
}

/*************/
Gui::~Gui()
{
#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Gui::~Gui - Destructor" << Log::endl;
#endif
}

/*************/
void Gui::key(int& key, int& action, int& mods)
{
    switch (key)
    {
    default:
    {
        if (action == GLFW_PRESS)
            _glv.setKeyDown(glfwToGlvKey(key));
        else if (action == GLFW_RELEASE)
            _glv.setKeyUp(glfwToGlvKey(key));
        else if (action == GLFW_REPEAT)
            _glv.setKeyDown(glfwToGlvKey(key));
        _glv.setKeyModifiers(mods & GLFW_MOD_SHIFT, mods & GLFW_MOD_ALT, mods & GLFW_MOD_CONTROL, false, false);
        _glv.propagateEvent();
        break;
    }
    case GLFW_KEY_TAB:
    {
        if (action == GLFW_PRESS)
        {
            _isVisible = !_isVisible;
            //STimer::timer.setStatus(_isVisible);
        }
        break;
    }
    case GLFW_KEY_ESCAPE:
    {
        auto scene = _scene.lock();
        scene->sendMessage("quit");
        break;
    }
    case GLFW_KEY_B:
    {
        if (action == GLFW_PRESS)
        {
            auto scene = _scene.lock();
            scene->sendMessage("computeBlending");
        }
        break;
    }
    case GLFW_KEY_S:
    {
        if (mods == GLFW_MOD_CONTROL && action == GLFW_PRESS)
        {
            auto scene = _scene.lock();
            scene->sendMessage("save");
        }
        break;
    }
    case GLFW_KEY_F:
    {
        if (action == GLFW_PRESS)
        {
            auto scene = _scene.lock();

            if (_flashBG)
                scene->sendMessage("flashBG", {0});
            else
                scene->sendMessage("flashBG", {1});

            _flashBG = !_flashBG;
        }
        break;
    }
    // Switch the rendering to textured
    case GLFW_KEY_T: 
    {
        auto scene = _scene.lock();
        scene->sendMessage("wireframe", {0});
        break;
    }
    // Switch the rendering to wireframe
    case GLFW_KEY_W:
    {
        auto scene = _scene.lock();
        scene->sendMessage("wireframe", {1});
        break;
    }
    }
}

/*************/
void Gui::mousePosition(int xpos, int ypos)
{
    space_t x = (space_t)xpos;
    space_t y = (space_t)ypos;

    // If no movement, no message
    if (_prevMouseX == x && _prevMouseY == y)
        return;

    _prevMouseX = x;
    _prevMouseY = y;

    space_t relx = x;
    space_t rely = y;

    if (_glv.mouse().left() || _glv.mouse().right() || _glv.mouse().middle())
        _glv.setMouseMotion(relx, rely, Event::MouseDrag);
    else
        _glv.setMouseMotion(relx, rely, Event::MouseMove);

    _glv.setMousePos((int)x, (int)y, relx, rely);
    _glv.propagateEvent();
}

/*************/
void Gui::mouseButton(int btn, int action, int mods)
{
    int button {0};
    switch (btn)
    {
    default:
        break;
    case GLFW_MOUSE_BUTTON_LEFT:
        button = Mouse::Left;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        button = Mouse::Right;
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        button = Mouse::Middle;
        break;
    }
    _glv.setKeyModifiers(mods & GLFW_MOD_SHIFT, mods & GLFW_MOD_ALT, mods & GLFW_MOD_CONTROL, false, false);

    space_t x = _glv.mouse().x();
    space_t y = _glv.mouse().y();

    if (action == GLFW_PRESS)
        _glv.setMouseDown(x, y, button, 0);
    if (action == GLFW_RELEASE)
        _glv.setMouseUp(x, y, button, 0);
    _glv.propagateEvent();
}

/*************/
void Gui::mouseScroll(double xoffset, double yoffset)
{
    _glv.setMouseWheel(yoffset);
    _glv.propagateEvent();
}

/*************/
bool Gui::linkTo(BaseObjectPtr obj)
{
    // Mandatory before trying to link
    BaseObject::linkTo(obj);

    if (dynamic_pointer_cast<Camera>(obj).get() != nullptr)
    {
        CameraPtr cam = dynamic_pointer_cast<Camera>(obj);
        _glvGlobalView.setCamera(cam);
        return true;
    }
    else if (dynamic_pointer_cast<Object>(obj).get() != nullptr)
    {
        ObjectPtr object = dynamic_pointer_cast<Object>(obj);
        _guiCamera->linkTo(object);
        return true;
    }

    return false;
}

/*************/
bool Gui::render()
{
    ImageSpec spec = _outTexture->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);

    if (_isVisible)
    {
        _doNotRender = false;
        // Render the visible camera
        _glvGlobalView._camera->render();
    }

#ifdef DEBUG
    GLenum error = 0;
#endif
    if (_doNotRender == false)
    {
        if (!_window->setAsCurrentContext()) 
        	 SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
        glViewport(0, 0, _width, _height);

#ifdef DEBUG
        error = glGetError();
#endif
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        GLenum fboBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, fboBuffers);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        
        if (_isVisible) 
            _glv.drawWidgets(_width, _height, 0.016);

        glActiveTexture(GL_TEXTURE0);
        _outTexture->generateMipmap();

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

#ifdef DEBUG
        error = glGetError();
        if (error)
            SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while rendering the camera: " << error << Log::endl;
#endif

        _window->releaseContext();

        if (!_isVisible)
            _doNotRender = true;
    }

#ifdef DEBUG
    return error != 0 ? true : false;
#else
    return false;
#endif
}

/*************/
void Gui::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    if (!_window->setAsCurrentContext()) 
    	 SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - A previous context has not been released." << Log::endl;;
    _depthTexture->resize(width, height);
    _outTexture->resize(width, height);
    _window->releaseContext();

    _width = width;
    _height = height;
    _doNotRender = false;

    initGLV(width, height);
}

/*************/
int Gui::glfwToGlvKey(int key)
{
    // Nothing special noted yet...
    return key;
}

/*************/
void Gui::initGLV(int width, int height)
{
    _glv.width(width);
    _glv.height(height);
    _glv.disable(DrawBack);

    _style.color.set(Color(1.0, 0.5, 0.2, 0.7), 0.7);

    // Log display
    _glvLog.setTextFunc([](GlvTextBox& that)
    {
        // Compute the number of lines which would fit
        int nbrLines = that.height() / (int)(that.fontSize + that.lineSpacing * that.fontSize);
    
        // Convert the last lines of the text log
        vector<string> logs = SLog::log.getLogs(Log::MESSAGE, Log::WARNING, Log::ERROR, Log::DEBUGGING);
        string text;
        int scrollOffset = that._scrollOffset;
        scrollOffset = std::max(0, std::min((int)logs.size() - nbrLines, scrollOffset));
        that._scrollOffset = scrollOffset;
        int offset = std::min((int)logs.size() - 1, std::max(0, ((int)logs.size() - nbrLines - scrollOffset)));
        for (int i = offset; i < logs.size(); ++i)
            text += logs[i] + string("\n");

        return text;
    });
    _glvLog.width(width / 2 - 16);
    _glvLog.height(height / 4 - 16);
    _glvLog.bottom(height - 8);
    _glvLog.left(8);
    _glvLog.style(&_style);
    _glv << _glvLog;

    // FPS and timings
    _glvProfile.setTextFunc([](GlvTextBox& that)
    {
        // Smooth the values
        static float fps {0.f};
        static float worldFps {0.f};
        static float upl {0.f};
        static float tex {0.f};
        static float cam {0.f};
        static float gui {0.f};
        static float win {0.f};
        static float buf {0.f};
        static float evt {0.f};

        fps = fps * 0.95 + 1e6 / std::max(1ull, STimer::timer["sceneLoop"]) * 0.05;
        worldFps = worldFps * 0.9 + 1e6 / std::max(1ull, STimer::timer["worldLoop"]) * 0.1;
        upl = upl * 0.9 + STimer::timer["upload"] * 0.001 * 0.1;
        tex = tex * 0.9 + STimer::timer["textureUpload"] * 0.001 * 0.1;
        cam = cam * 0.9 + STimer::timer["cameras"] * 0.001 * 0.1;
        gui = gui * 0.9 + STimer::timer["guis"] * 0.001 * 0.1;
        win = win * 0.9 + STimer::timer["windows"] * 0.001 * 0.1;
        buf = buf * 0.9 + STimer::timer["swap"] * 0.001 * 0.1;
        evt = evt * 0.9 + STimer::timer["events"] * 0.001 * 0.1;

        // Create the text message
        ostringstream stream;
        stream << "Framerate: " << setprecision(4) << fps << " fps\n";
        stream << "World framerate: " << setprecision(4) << worldFps << " fps\n";
        stream << "Sending buffers to Scenes: " << setprecision(4) << upl << " ms\n";
        stream << "Texture upload: " << setprecision(4) << tex << " ms\n";
        stream << "Cameras rendering: " << setprecision(4) << cam << " ms\n";
        stream << "GUI rendering: " << setprecision(4) << gui << " ms\n";
        stream << "Windows rendering: " << setprecision(4) << win << " ms\n";
        stream << "Swapping and events: " << setprecision(4) << buf << " ms\n";

        return stream.str();
    });
    _glvProfile.width(SPLASH_GLV_FONTSIZE * 36);
    _glvProfile.height(SPLASH_GLV_FONTSIZE * 2 * 8 + 8);
    _glvProfile.style(&_style);

    // Some help regarding keyboard shortcuts
    _glvHelp.setTextFunc([](GlvTextBox& that)
    {
        string text;
        text += "Tab: show / hide this GUI\n";
        text += "Shortcuts for the calibration view:\n";
        text += " Space: switcher between cameras\n";
        text += " A: show / hide the target calibration point\n";
        text += " F: white background instead of black\n";
        text += " C: calibrate the selected camera\n";
        text += " R: revert camera to previous calibration\n";
        text += " B: compute the blending between all cameras\n";
        text += " H: hide all but the selected camera\n";
        text += " T: textured draw mode\n";
        text += " W: wireframe draw mode\n";

        return text;
    });
    _glvHelp.width(SPLASH_GLV_FONTSIZE * 48);
    _glvHelp.height(SPLASH_GLV_FONTSIZE * 2 * 11 + 8);
    _glvHelp.style(&_style);

    // Controls
    _glvControl.width(200);
    _glvControl.height(128);
    _glvControl.top(8);
    _glvControl.right(_width / 2 - 64);
    _glvControl.style(&_style);
    _glvControl.setScene(_scene);

    Placer placer(_glv, Direction::S, Place::TL, 8, 8);
    placer << _glvHelp << _glvProfile << _glvControl;

    // GUI camera view
    _glvGlobalView.set(Rect(8, 8, 800, 600));
    _glvGlobalView.right(width - 8);
    _glvGlobalView.style(&_style);
    _glvGlobalView.setCamera(_guiCamera);
    _glvGlobalView.setScene(_scene);
    _glv << _glvGlobalView;

    // Performance graphs
    _glvGraph.width(_width / 2 - 16);
    _glvGraph.height(_height / 4 - 16);
    _glvGraph.bottom(height - 8);
    _glvGraph.right(_width - 8);
    _glvGraph.style(&_style);
    _glv << _glvGraph;
}

/*************/
void Gui::registerAttributes()
{
    _attribFunctions["size"] = AttributeFunctor([&](Values args) {
        if (args.size() < 2)
            return false;
        setOutputSize(args[0].asInt(), args[1].asInt());
        return true;
    });
}

} // end of namespace
