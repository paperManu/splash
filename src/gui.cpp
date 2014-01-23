#include "gui.h"
#include "timer.h"

using namespace std;
using namespace glv;

namespace Splash
{

/*************/
Gui::Gui(GlWindowPtr w)
{
    _type = "gui";

    if (w.get() == nullptr)
        return;

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
        texture->reset(GL_TEXTURE_2D, 0, GL_RGB8, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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

    if (_glv.mouse().left())
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
    SLog::log << xoffset << " " << yoffset << Log::endl;
}

/*************/
bool Gui::render()
{
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

    glClearColor(0.02, 0.02, 0.02, 1.0); //< This is the transparent color
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
    _depthTexture->reset(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    _outTexture->reset(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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

    _glvLog.setTextFunc([](GlvTextBox& that)
    {
        // Compute the number of lines which would fit
        int nbrLines = that.height() / (int)(that.fontSize + that.lineSpacing * that.fontSize);
    
        // Convert the last lines of the text log
        vector<string> logs = SLog::log.getLogs(Log::MESSAGE);
        string text;
        for (auto t = logs.begin() + std::max(0, ((int)logs.size() - nbrLines)); t != logs.end(); ++t)
            text += *t + string("\n");

        return text;
    });
    _glvLog.width(width / 2 - 16);
    _glvLog.height(height / 4 - 16);
    _glvLog.bottom(height - 8);
    _glvLog.left(8);
    _glvLog.style(&_style);
    _glv << _glvLog;

    _glvProfile.setTextFunc([](GlvTextBox& that)
    {
        // Smooth the values
        static float fps {0.f};
        static float cam {0.f};
        static float win {0.f};
        static float buf {0.f};
        static float evt {0.f};

        fps = fps * 0.95 + 1e6 / std::max(1ull, STimer::timer["worldLoop"]) * 0.05;
        cam = cam * 0.95 + STimer::timer["cameras"] * 0.001 * 0.05;
        win = win * 0.95 + STimer::timer["windows"] * 0.001 * 0.05;
        buf = buf * 0.95 + STimer::timer["swap"] * 0.001 * 0.05;
        evt = evt * 0.95 + STimer::timer["events"] * 0.001 * 0.05;

        // Create the text message
        string text;
        text += "Framerate: " + to_string(fps) + " fps\n";
        text += "Cameras rendering: " + to_string(cam) + " ms\n";
        text += "Windows rendering: " + to_string(win) + " ms\n";
        text += "Buffer swapping: " + to_string(buf) + " ms\n";
        text += "Events processing: " + to_string(evt) + " ms\n";

        return text;
    });

    _glvProfile.width(width / 2 - 16);
    _glvProfile.height(height / 4 - 16);
    _glvProfile.bottom(height - 8);
    _glvProfile.right(width - 8);
    _glvProfile.style(&_style);
    _glv << _glvProfile;
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

/*************/
void GlvTextBox::onDraw(GLV& g)
{
    try
    {
        draw::color(SPLASH_GLV_TEXTCOLOR);
        draw::lineWidth(SPLASH_GLV_FONTSIZE);

        string text = getText(*this);

        draw::text(text.c_str(), 4, 4);
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
        if (g.mouse().left())
        {
            move(g.mouse().dx(), g.mouse().dy());
            return false;
        }
    }

    return true;
}

} // end of namespace
