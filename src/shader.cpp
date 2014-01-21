#include "shader.h"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

namespace Splash {

/*************/
Shader::Shader()
{
    _type = "shader";

    _shaders[vertex] = glCreateShader(GL_VERTEX_SHADER);
    _shaders[geometry] = glCreateShader(GL_GEOMETRY_SHADER);
    _shaders[fragment] = glCreateShader(GL_FRAGMENT_SHADER);
    _program = glCreateProgram();

    setSource(DEFAULT_VERTEX_SHADER, vertex);
    setSource(DEFAULT_FRAGMENT_SHADER, fragment);
    compileProgram();
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
void Shader::activate()
{
    if (!_isLinked)
    {
        if (!linkProgram())
            return;
        _locationMVP = glGetUniformLocation(_program, "_viewProjectionMatrix");
        _locationNormalMatrix = glGetUniformLocation(_program, "_normalMatrix");
        _locationSide = glGetUniformLocation(_program, "_sideness");
        _locationTextureNbr = glGetUniformLocation(_program, "_textureNbr");
    }

    glUseProgram(_program);

    glUniform1i(_locationSide, _sideness);
    _textureNbr = 0;
    glUniform1i(_locationTextureNbr, _textureNbr);
}

/*************/
void Shader::deactivate()
{
    glUseProgram(0);
}

/*************/
void Shader::setSideness(const Sideness side)
{
    _sideness = side;
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
        SLog::log << Log::DEBUG << "Shader::" << __FUNCTION__ << " - Shader compiled successfully" << Log::endl;
    else
    {
        SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error while compiling a shader:" << Log::endl;
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        SLog::log(Log::WARNING, __FUNCTION__, " - Error log: \n", (const char*)log);
        free(log);
    }

    _isLinked = false;
}

/*************/
void Shader::setSourceFromFile(const std::string filename, const ShaderType type)
{
    ifstream in(filename, ios::in | ios::binary);
    if (in)
    {
        string contents;
        in.seekg(0, ios::end);
        contents.resize(in.tellg());
        in.seekg(0, ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        setSource(contents, type);
    }

    SLog::log << Log::WARNING << __FUNCTION__ << " - Unable to load file " << filename << Log::endl;
}

/*************/
void Shader::setTexture(const TexturePtr texture, const GLuint textureUnit, const std::string& name)
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture->getTexId());
    GLint uniform = glGetUniformLocation(_program, name.c_str());
    glUniform1i(uniform, textureUnit);

    _textureNbr++;
    glUniform1i(_locationTextureNbr, _textureNbr);
}

/*************/
void Shader::setViewProjectionMatrix(const glm::mat4& mvp)
{
    glUniformMatrix4fv(_locationMVP, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(_locationNormalMatrix, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(mvp))));
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
                SLog::log(Log::DEBUG, __FUNCTION__, " - Shader successfully attached to the program");
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
        SLog::log << Log::DEBUG << "Shader::" << __FUNCTION__ << " - Shader program linked successfully" << Log::endl;
        _isLinked = true;
        return true;
    }
    else
    {
        SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error while linking the shader program" << Log::endl;

        GLint length;
        glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*)malloc(length);
        glGetProgramInfoLog(_program, length, &length, log);
        SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error log: \n" << (const char*)log << Log::endl;
        free(log);

        _isLinked = false;
        return false;
    }
}

} // end of namespace
