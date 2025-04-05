#include "./graphics/api/opengl/shader_stage.h"

#include "./utils/log.h"

namespace Splash::gfx::opengl
{
/*************/
ShaderStage::~ShaderStage()
{
    if (glIsShader(_shader))
        glDeleteShader(_shader);
}

/*************/
bool ShaderStage::isValid() const
{
    OnOpenGLScopeExit(std::string("ShaderStage::").append(__FUNCTION__));

    if (!glIsShader(_shader))
        return false;

    GLint status;
    glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        const auto log = getShaderInfoLog();
        Log::get() << Log::WARNING << "ShaderStage::" << __FUNCTION__ << " - Error while compiling the " << stringFromShaderType() << " shader in program " << Log::endl;
        Log::get() << Log::WARNING << "ShaderStage::" << __FUNCTION__ << " - Error log: \n" << log << Log::endl;
        return false;
    }

    return true;
}

/*************/
bool ShaderStage::setSource(std::string_view source)
{
    OnOpenGLScopeExit(std::string("ShaderStage::").append(__FUNCTION__));

    if (glIsShader(_shader))
        glDeleteShader(_shader);

    _shader = glCreateShader(static_cast<GLenum>(_type));

    const auto sourcePtr = source.data();
    glShaderSource(_shader, 1, &sourcePtr, nullptr);
    glCompileShader(_shader);
    GLint status;
    glGetShaderiv(_shader, GL_COMPILE_STATUS, &status);

    if (status)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "gfx::ShaderStage::" << __FUNCTION__ << " - Shader of type " << stringFromShaderType(_type) << " compiled successfully" << Log::endl;
#endif
    }
    else
    {
        Log::get() << Log::WARNING << "gfx::ShaderStage::" << __FUNCTION__ << " - Error while compiling a shader of type " << stringFromShaderType() << Log::endl;
        Log::get() << Log::WARNING << "gfx::ShaderStage::" << __FUNCTION__ << " - Shader source: " << source << Log::endl;
        const auto log = getShaderInfoLog();
        Log::get() << Log::WARNING << "gfx::ShaderStage::" << __FUNCTION__ << " - Error log: \n" << log << Log::endl;
    }

    _source = source;
    return status;
}

/*************/
std::string ShaderStage::getShaderInfoLog() const
{
    OnOpenGLScopeExit(std::string("ShaderStage::").append(__FUNCTION__));

    GLint length;
    glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &length);
    auto str = std::string(static_cast<size_t>(length), '\0');
    glGetShaderInfoLog(_shader, length, &length, str.data());
    return str;
}

} // namespace Splash::gfx::opengl
