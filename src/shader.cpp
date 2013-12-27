#include "shader.h"

using namespace std;

namespace Splash {

/*************/
Shader::Shader()
{
    _shaders[vertex] = glCreateShader(GL_VERTEX_SHADER);
    _shaders[geometry] = glCreateShader(GL_GEOMETRY_SHADER);
    _shaders[fragment] = glCreateShader(GL_FRAGMENT_SHADER);
    _program = glCreateProgram();
}

/*************/
Shader::~Shader()
{
    if (glIsProgram(_program))
        glDeleteProgram(_program);
    for (auto& shader : _shaders)
        if (glIsShader(shader.second))
            glDeleteShader(shader.second);
}

/*************/
void Shader::activate(const GeometryPtr geometry)
{
    if (geometry != _geometry)
    {
        glBindAttribLocation(_program, geometry->getVertexCoords(), "_vertex");
        glBindAttribLocation(_program, geometry->getTextureCoords(), "_texture");
        _geometry = geometry;
        if (!linkProgram())
            return;
    }

    glUseProgram(_program);
}

/*************/
void Shader::setSource(const std::string& src, const ShaderType type)
{
    GLuint shader = _shaders[type];
    const char* shaderSrc = src.c_str();
    glShaderSource(shader, 1, (const GLchar**)&shaderSrc, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status)
        gLog(string(__FUNCTION__) + string(" - Shader compiled successfully"), Log::DEBUG);
    else
    {
        gLog(string(__FUNCTION__) + string(" - Error while compiling a shader:"), Log::WARNING);
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        gLog(string(__FUNCTION__) + string(" - Error log: \n") + string(log), Log::WARNING);
        free(log);
    }

    _isLinked = false;
}

/*************/
void Shader::setTexture(const TexturePtr texture, const GLuint textureUnit, const std::string& name)
{
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture->getTexId());
    GLint uniform = glGetUniformLocation(_program, name.c_str());
    glUniform1i(uniform, textureUnit);
}

/*************/
void Shader::compileProgram()
{
    GLint status;
    if (glIsProgram(_program) == GL_TRUE)
        glDeleteProgram(_program);

    _program = glCreateProgram();
    for (auto& shader : _shaders)
    {
        if (glIsShader(shader.second))
        {
            glGetShaderiv(shader.second, GL_COMPILE_STATUS, &status);
            if (status == GL_TRUE)
            {
                glAttachShader(_program, shader.second);
                gLog(string(__FUNCTION__) + string(" - Shader successfully attacher to the program"), Log::DEBUG);
            }
        }
    }
}

/*************/
bool Shader::linkProgram()
{
    GLint status;
    glLinkProgram(_program);
    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE)
    {
        gLog(string(__FUNCTION__) + string(" - Shader program linked successfully"), Log::DEBUG);
        _isLinked = true;
        return true;
    }
    else
    {
        gLog(string(__FUNCTION__) + string(" - Error while linking the shader program"), Log::WARNING);
        
        GLint length;
        glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*)malloc(length);
        glGetProgramInfoLog(_program, length, &length, log);
        gLog(string(__FUNCTION__) + string(" - Error log: \n") + string(log), Log::WARNING);
        free(log);

        _isLinked = false;
        return false;
    }
}

} // end of namespace
