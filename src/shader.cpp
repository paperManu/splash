#include "shader.h"
#include "shaderSources.h"

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

    setSource(ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
    setSource(ShaderSources.FRAGMENT_SHADER_TEXTURE, fragment);
    compileProgram();

    registerAttributes();
}

/*************/
Shader::~Shader()
{
    if (glIsProgram(_program))
        glDeleteProgram(_program);
    for (auto& shader : _shaders)
        if (glIsShader(shader.second))
            glDeleteShader(shader.second);

    SLog::log << Log::DEBUG << "Shader::~Shader - Destructor" << Log::endl;
}

/*************/
void Shader::activate()
{
    if (!_isLinked)
    {
        if (!linkProgram())
            return;
        _locationMVP = glGetUniformLocation(_program, "_modelViewProjectionMatrix");
        _locationNormalMatrix = glGetUniformLocation(_program, "_normalMatrix");
        _locationSide = glGetUniformLocation(_program, "_sideness");
        _locationTextureNbr = glGetUniformLocation(_program, "_textureNbr");
        _locationColor = glGetUniformLocation(_program, "_color");
        _locationScale = glGetUniformLocation(_program, "_scale");
    }

    glUseProgram(_program);

    glUniform1i(_locationSide, _sideness);
    _textureNbr = 0;
    glUniform1i(_locationTextureNbr, _textureNbr);
    glUniform3f(_locationScale, _scale.x, _scale.y, _scale.z);

    if (_texture == color)
        glUniform4f(_locationColor, _color.r, _color.g, _color.b, _color.a);
}

/*************/
void Shader::deactivate()
{
    glUseProgram(0);
    for (int i = 0; i < _textureNbr; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
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
        SLog::log << Log::DEBUG << "Shader::" << __FUNCTION__ << " - Shader of type " << stringFromShaderType(type) << " compiled successfully" << Log::endl;
    else
    {
        SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error while compiling a shader of type " << stringFromShaderType(type) << Log::endl;
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error log: \n" << (const char*)log << Log::endl;
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
void Shader::setModelViewProjectionMatrix(const glm::mat4& mvp)
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
                SLog::log << Log::DEBUG << "Shader::" << __FUNCTION__ << " - Shader of type " << stringFromShaderType(shader.first) << " successfully attached to the program" << Log::endl;
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

/*************/
string Shader::stringFromShaderType(ShaderType type)
{
    switch (type)
    {
    default:
        return string();
    case 0:
        return string("vertex");
    case 1:
        return string("geometry");
    case 2:
        return string("fragment");
    }
}

/*************/
void Shader::registerAttributes()
{
    _attribFunctions["texture"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        if (args[0].asString() == "uv")
        {
            _texture = uv;
            setSource(ShaderSources.FRAGMENT_SHADER_TEXTURE, fragment);
        }
        else if (args[0].asString() == "color")
        {
            _texture = color;
            setSource(ShaderSources.FRAGMENT_SHADER_COLOR, fragment);
        }
        return true;
    });

    _attribFunctions["color"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 4)
            return false;
        _color = glm::vec4(args[0].asFloat(), args[1].asFloat(), args[2].asFloat(), args[3].asFloat());
        return true;
    });

    _attribFunctions["scale"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;

        if (args.size() < 3)
            _scale = glm::vec3(args[0].asFloat(), args[0].asFloat(), args[0].asFloat());
        else
            _scale = glm::vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());

        return true;
    });
}

} // end of namespace
