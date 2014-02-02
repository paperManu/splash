#include "gui.h"
#include "scene.h"
#include "timer.h"

using namespace std;
using namespace glv;

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
    glfwMakeContextCurrent(_window->get());
    glGetError();
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    {
        TexturePtr texture(new Texture);
        texture->reset(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        _depthTexture = move(texture);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture->getTexId(), 0);
    }

    {
        TexturePtr texture(new Texture);
        texture->reset(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
        _outTexture = move(texture);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _outTexture->getTexId(), 0);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while initializing framebuffer object: " << status << Log::endl;
    else
        SLog::log << Log::MESSAGE << "Gui::" << __FUNCTION__ << " - Framebuffer object successfully initialized" << Log::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glfwMakeContextCurrent(NULL);

    // Create the default GUI camera
    glfwMakeContextCurrent(scene->_mainWindow->get());
    _guiCamera = CameraPtr(new Camera(scene->_mainWindow));
    _guiCamera->setName("guiCamera");
    _guiCamera->setAttribute("eye", {2.0, 0.0, 0.0});
    _guiCamera->setAttribute("target", {0.0, 0.0, 0.5});
    _guiCamera->setAttribute("size", {640, 480});
    glfwMakeContextCurrent(NULL);

    // Intialize the GUI widgets
    initGLV(_width, _height);

    registerAttributes();
}

/*************/
Gui::~Gui()
{
    SLog::log << Log::DEBUG << "Gui::~Gui - Destructor" << Log::endl;
}

/*************/
void Gui::key(int& key, int& action, int& mods)
{
    switch (key)
    {
    default:
        if (action == GLFW_PRESS)
            _glv.setKeyDown(glfwToGlvKey(key));
        else if (action == GLFW_RELEASE)
            _glv.setKeyUp(glfwToGlvKey(key));
        _glv.setKeyModifiers(mods && GLFW_MOD_SHIFT, mods && GLFW_MOD_ALT, mods && GLFW_MOD_CONTROL, false, false);
        _glv.propagateEvent();
        break;
    case GLFW_KEY_TAB:
        if (action == GLFW_PRESS)
            _isVisible = !_isVisible;
        break;
    }
}

/*************/
void Gui::mousePosition(int xpos, int ypos)
{
    space_t x = (space_t)xpos;
    space_t y = (space_t)ypos;
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
    _glvGlobalView._camera->render();

    glfwMakeContextCurrent(_window->get());

    GLenum error = glGetError();
    ImageSpec spec = _outTexture->getSpec();
    if (spec.width != _width || spec.height != _height)
        setOutputSize(spec.width, spec.height);
    glViewport(0, 0, _width, _height);

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

    error = glGetError();
    if (error)
        SLog::log << Log::WARNING << "Gui::" << __FUNCTION__ << " - Error while rendering the camera: " << error << Log::endl;

    glfwMakeContextCurrent(NULL);

    return error != 0 ? true : false;
}

/*************/
void Gui::setOutputSize(int width, int height)
{
    if (width == 0 || height == 0)
        return;

    glfwMakeContextCurrent(_window->get());
    _depthTexture->resize(width, height);
    _outTexture->resize(width, height);
    glfwMakeContextCurrent(NULL);

    _width = width;
    _height = height;

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
        vector<string> logs = SLog::log.getLogs(Log::DEBUG, Log::MESSAGE, Log::WARNING, Log::ERROR);
        string text;
        int scrollOffset = that._scrollOffset;
        scrollOffset = std::max(0, std::min((int)logs.size() - nbrLines, scrollOffset));
        that._scrollOffset = scrollOffset;
        int offset = std::min((int)logs.size() - 1, std::max(0, ((int)logs.size() - nbrLines - scrollOffset)));
        for (auto t = logs.begin() + offset; t != logs.end(); ++t)
            text += *t + string("\n");

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
        static float upl {0.f};
        static float cam {0.f};
        static float gui {0.f};
        static float win {0.f};
        static float buf {0.f};
        static float evt {0.f};

        fps = fps * 0.95 + 1e6 / std::max(1ull, STimer::timer["worldLoop"]) * 0.05;
        upl = upl * 0.95 + STimer::timer["upload"] * 0.001 * 0.05;
        cam = cam * 0.95 + STimer::timer["cameras"] * 0.001 * 0.05;
        gui = gui * 0.95 + STimer::timer["guis"] * 0.001 * 0.05;
        win = win * 0.95 + STimer::timer["windows"] * 0.001 * 0.05;
        buf = buf * 0.95 + STimer::timer["swap"] * 0.001 * 0.05;
        evt = evt * 0.95 + STimer::timer["events"] * 0.001 * 0.05;

        // Create the text message
        string text;
        text += "Framerate: " + to_string(fps) + " fps\n";
        text += "Buffers upload: " + to_string(upl) + " ms\n";
        text += "Cameras rendering: " + to_string(cam) + " ms\n";
        text += "GUI rendering: " + to_string(gui) + " ms\n";
        text += "Windows rendering: " + to_string(win) + " ms\n";
        text += "Buffer swapping: " + to_string(buf) + " ms\n";
        text += "Events processing: " + to_string(evt) + " ms\n";

        return text;
    });

    _glvProfile.width(SPLASH_GLV_FONTSIZE * 32);
    _glvProfile.height(SPLASH_GLV_FONTSIZE * 2 * 7 + 8);
    _glvProfile.top(8);
    _glvProfile.left(8);
    _glvProfile.style(&_style);
    _glv << _glvProfile;

    // GUI camera view
    _glvGlobalView.set(Rect(8, 8, 640, 480));
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
    _attribFunctions["size"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 2)
            return false;
        setOutputSize(args[0].asInt(), args[1].asInt());
        return true;
    });
}

} // end of namespace
