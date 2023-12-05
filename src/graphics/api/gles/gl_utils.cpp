#include "./graphics/api/gles/gl_utils.h"

#include "./utils/log.h"

namespace Splash::gfx::gles
{

#ifdef DEBUG
/*************/
GLESErrorGuard::GLESErrorGuard(std::string_view scope)
    : _scope(scope)
{
    // Get previous errors, if any
    const auto previousError = glGetError();
    if (previousError != GL_NO_ERROR)
        Log::get() << Log::WARNING << _scope << " - An error flag was set in a previous OpenGL ES call" << Log::endl;
}

/*************/
GLESErrorGuard::~GLESErrorGuard()
{
    const auto error = glGetError();
    switch (error)
    {
    case GL_NO_ERROR:
        return;
    case GL_INVALID_ENUM:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_INVALID_ENUM" << Log::endl;
        return;
    case GL_INVALID_VALUE:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_INVALID_VALUE" << Log::endl;
        return;
    case GL_INVALID_OPERATION:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_INVALID_OPERATION" << Log::endl;
        return;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_INVALID_FRAMEBUFFER_OPERATION" << Log::endl;
        return;
    case GL_OUT_OF_MEMORY:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_OUT_OF_MEMORY" << Log::endl;
        return;
    case GL_STACK_UNDERFLOW:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_STACK_UNDERFLOW" << Log::endl;
        return;
    case GL_STACK_OVERFLOW:
        Log::get() << Log::WARNING << _scope << " - OpenGL ES error: GL_STACK_OVERFLOW" << Log::endl;
        return;
    }
}
#endif

} // namespace Splash::gfx::gles
