#include "./graphics/api/opengl/gl_utils.h"

#include "./utils/log.h"

namespace Splash::gfx::opengl
{

#ifdef DEBUG
/*************/
OpenGLErrorGuard::OpenGLErrorGuard(std::string_view scope)
    : _scope(scope)
{
    // Get previous errors, if any
    const auto previousError = glGetError();
    if (previousError != GL_NO_ERROR)
    {
        Log::get() << Log::WARNING << _scope << " - An error flag was set in a previous OpenGL call" << Log::endl;
        switch (previousError)
        {
        case GL_INVALID_ENUM:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_INVALID_ENUM" << Log::endl;
            return;
        case GL_INVALID_VALUE:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_INVALID_VALUE" << Log::endl;
            return;
        case GL_INVALID_OPERATION:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_INVALID_OPERATION" << Log::endl;
            return;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION" << Log::endl;
            return;
        case GL_OUT_OF_MEMORY:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_OUT_OF_MEMORY" << Log::endl;
            return;
        case GL_STACK_UNDERFLOW:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_STACK_UNDERFLOW" << Log::endl;
            return;
        case GL_STACK_OVERFLOW:
            Log::get() << Log::WARNING << _scope << " - Previous OpenGL error: GL_STACK_OVERFLOW" << Log::endl;
            return;
        }
    }
}

/*************/
OpenGLErrorGuard::~OpenGLErrorGuard()
{
    const auto error = glGetError();
    switch (error)
    {
    case GL_NO_ERROR:
        return;
    case GL_INVALID_ENUM:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_INVALID_ENUM" << Log::endl;
        return;
    case GL_INVALID_VALUE:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_INVALID_VALUE" << Log::endl;
        return;
    case GL_INVALID_OPERATION:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_INVALID_OPERATION" << Log::endl;
        return;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION" << Log::endl;
        return;
    case GL_OUT_OF_MEMORY:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_OUT_OF_MEMORY" << Log::endl;
        return;
    case GL_STACK_UNDERFLOW:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_STACK_UNDERFLOW" << Log::endl;
        return;
    case GL_STACK_OVERFLOW:
        Log::get() << Log::WARNING << _scope << " - OpenGL error: GL_STACK_OVERFLOW" << Log::endl;
        return;
    }
}
#endif

} // namespace Splash::gfx::opengl
