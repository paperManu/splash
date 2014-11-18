#include "shader.h"

#include "log.h"
#include "shaderSources.h"

#include <fstream>
#include <sstream>
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

#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Shader::~Shader - Destructor" << Log::endl;
#endif
}

/*************/
void Shader::activate()
{
    _mutex.lock();
    if (!_isLinked)
    {
        if (!linkProgram())
            return;
    }

    _activated = true;
    glUseProgram(_program);
}

/*************/
void Shader::deactivate()
{
    glUseProgram(0);
    _activated = false;
    for (int i = 0; i < _textures.size(); ++i)
        _textures[i]->unbind();
    _textures.clear();
    _mutex.unlock();
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
    {
#ifdef DEBUG
        SLog::log << Log::DEBUGGING << "Shader::" << __FUNCTION__ << " - Shader of type " << stringFromShaderType(type) << " compiled successfully" << Log::endl;
#endif
    }
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

    _shadersSource[type] = src;
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
    texture->bind();
    GLint uniform = glGetUniformLocation(_program, name.c_str());
    glUniform1i(uniform, textureUnit);

    _textures.push_back(texture);
    if (_uniforms.find("_textureNbr") != _uniforms.end())
    {
        _uniforms["_textureNbr"].first = Values({(int)_textures.size()});
        _uniformsToUpdate.push_back("_textureNbr");
    }
}

/*************/
void Shader::setModelViewProjectionMatrix(const glm::dmat4& mvp)
{
    glm::mat4 floatMvp = (glm::mat4)mvp;
    if (_uniforms.find("_modelViewProjectionMatrix") != _uniforms.end())
        glUniformMatrix4fv(_uniforms["_modelViewProjectionMatrix"].second, 1, GL_FALSE, glm::value_ptr(floatMvp));
    if (_uniforms.find("_normalMatrix") != _uniforms.end())
        glUniformMatrix4fv(_uniforms["_normalMatrix"].second, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(floatMvp))));
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
#ifdef DEBUG
                SLog::log << Log::DEBUGGING << "Shader::" << __FUNCTION__ << " - Shader of type " << stringFromShaderType(shader.first) << " successfully attached to the program" << Log::endl;
#endif
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
#ifdef DEBUG
        SLog::log << Log::DEBUGGING << "Shader::" << __FUNCTION__ << " - Shader program linked successfully" << Log::endl;
#endif

        for (auto src : _shadersSource)
            parseUniforms(src.second);

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
void Shader::parseUniforms(const std::string& src)
{
    istringstream input(src);
    for (string line; getline(input, line);)
    {
        string::size_type position;
        if ((position = line.find("uniform")) == string::npos)
            continue;
        string next = line.substr(position + 8);
        string type, name;
        istringstream lineStream(next);
        getline(lineStream, type, ' ');
        getline(lineStream, name, ' ');

        if (name.find(";") != string::npos)
            name = name.substr(0, name.size() - 1);

        Values values;
        if (_uniforms.find(name) != _uniforms.end())
            values = _uniforms[name].first;

        // Get the location
        if (type == "int")
            _uniforms[name] = pair<Values, GLint>({0}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "float")
            _uniforms[name] = pair<Values, GLint>({0.f}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "vec3")
            _uniforms[name] = pair<Values, GLint>({0.f, 0.f, 0.f}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "vec4")
            _uniforms[name] = pair<Values, GLint>({0.f, 0.f, 0.f, 0.f}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "ivec2")
            _uniforms[name] = pair<Values, GLint>({0, 0}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "ivec3")
            _uniforms[name] = pair<Values, GLint>({0, 0, 0}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "ivec4")
            _uniforms[name] = pair<Values, GLint>({0, 0, 0, 0}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "mat4")
            _uniforms[name] = pair<Values, GLint>({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, glGetUniformLocation(_program, name.c_str()));
        else if (type != "sampler2D")
            SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error while parsing uniforms: " << name << " is of unhandled type " << type << Log::endl;

        if (values.size() != 0)
        {
            _uniforms[name].first = values;
            _uniformsToUpdate.push_back(name);
        }
        else
        {
            // Save the default value
            if (type == "int")
            {
                int v;
                glGetUniformiv(_program, _uniforms[name].second, &v);
                _uniforms[name].first = Values({v});
            }
            else if (type == "float")
            {
                float v;
                glGetUniformfv(_program, _uniforms[name].second, &v);
                _uniforms[name].first = Values({v});
            }
            else if (type == "vec3")
            {
                float v[3];
                glGetUniformfv(_program, _uniforms[name].second, v);
                _uniforms[name].first = Values({v[0], v[1], v[2]});
            }
            else if (type == "vec4")
            {
                float v[4];
                glGetUniformfv(_program, _uniforms[name].second, v);
                _uniforms[name].first = Values({v[0], v[1], v[2], v[3]});
            }
            else if (type == "ivec2")
            {
                int v[2];
                glGetUniformiv(_program, _uniforms[name].second, v);
                _uniforms[name].first = Values({v[0], v[1]});
            }
            else if (type == "ivec3")
            {
                int v[3];
                glGetUniformiv(_program, _uniforms[name].second, v);
                _uniforms[name].first = Values({v[0], v[1], v[2]});
            }
            else if (type == "ivec4")
            {
                int v[4];
                glGetUniformiv(_program, _uniforms[name].second, v);
                _uniforms[name].first = Values({v[0], v[1], v[2], v[3]});
            }
        }
    }

    // We parse all uniforms to deactivate of the obsolete ones
    for (auto& u : _uniforms)
    {
        string name = u.first;
        if (glGetUniformLocation(_program, name.c_str()) == -1)
            u.second.second = -1;
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
void Shader::updateUniforms()
{
    if (_activated)
    {
        for (int i = 0; i < _uniformsToUpdate.size(); ++i)
        {
            string u = _uniformsToUpdate[i];
            if (_uniforms.find(u) == _uniforms.end())
                continue;

            if (_uniforms[u].second == -1)
                continue;

            int size = _uniforms[u].first.size();
            int type = _uniforms[u].first[0].getType();

            if (size > 4)
                continue;

            if (type == Value::Type::i)
            {
                if (size == 1)
                    glUniform1i(_uniforms[u].second, _uniforms[u].first[0].asInt());
                else if (size == 2)
                    glUniform2i(_uniforms[u].second, _uniforms[u].first[0].asInt(), _uniforms[u].first[1].asInt());
                else if (size == 3)
                    glUniform3i(_uniforms[u].second, _uniforms[u].first[0].asInt(), _uniforms[u].first[1].asInt(), _uniforms[u].first[2].asInt());
                else if (size == 4)
                    glUniform4i(_uniforms[u].second, _uniforms[u].first[0].asInt(), _uniforms[u].first[1].asInt(), _uniforms[u].first[2].asInt(), _uniforms[u].first[3].asInt());
            }
            else if (type == Value::Type::f)
            {
                if (size == 1)
                    glUniform1f(_uniforms[u].second, _uniforms[u].first[0].asFloat());
                else if (size == 2)
                    glUniform2f(_uniforms[u].second, _uniforms[u].first[0].asFloat(), _uniforms[u].first[1].asFloat());
                else if (size == 3)
                    glUniform3f(_uniforms[u].second, _uniforms[u].first[0].asFloat(), _uniforms[u].first[1].asFloat(), _uniforms[u].first[2].asFloat());
                else if (size == 4)
                    glUniform4f(_uniforms[u].second, _uniforms[u].first[0].asFloat(), _uniforms[u].first[1].asFloat(), _uniforms[u].first[2].asFloat(), _uniforms[u].first[3].asFloat());
            }
        }

        _uniformsToUpdate.clear();
    }
}

/*************/
void Shader::resetShader(ShaderType type)
{
    glDeleteShader(_shaders[type]);
    GLenum glShaderType;
    if (type == vertex)
        glShaderType = GL_VERTEX_SHADER;
    if (type == geometry)
        glShaderType = GL_GEOMETRY_SHADER;
    if (type == fragment)
        glShaderType = GL_FRAGMENT_SHADER;
    _shaders[type] = glCreateShader(glShaderType);
}

/*************/
void Shader::registerAttributes()
{
    _attribFunctions["blending"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;
        
        _uniforms["_texBlendingMap"].first = args;
        _uniformsToUpdate.push_back("_texBlendingMap");

        return true;
    });

    _attribFunctions["blendWidth"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;

        _uniforms["_blendWidth"].first = args;
        _uniformsToUpdate.push_back("_blendWidth");

        return true;
    });

    _attribFunctions["blackLevel"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;

        _uniforms["_blackLevel"].first = args;
        _uniformsToUpdate.push_back("_blackLevel");

        return true;
    });

    _attribFunctions["brightness"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;

        _uniforms["_brightness"].first = args;
        _uniformsToUpdate.push_back("_brightness");

        return true;
    });

    _attribFunctions["colorTemperature"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;

        _uniforms["_colorTemperature"].first = args;
        _uniformsToUpdate.push_back("_colorTemperature");

        return true;
    });

    _attribFunctions["fill"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        if (args[0].asString() == "texture" && _fill != texture)
        {
            _fill = texture;
            setSource(ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
            resetShader(geometry);
            setSource(ShaderSources.FRAGMENT_SHADER_TEXTURE, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "color" && _fill != color)
        {
            _fill = color;
            setSource(ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
            resetShader(geometry);
            setSource(ShaderSources.FRAGMENT_SHADER_COLOR, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "uv" && _fill != uv)
        {
            _fill = uv;
            setSource(ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
            resetShader(geometry);
            setSource(ShaderSources.FRAGMENT_SHADER_UV, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "wireframe" && _fill != wireframe)
        {
            _fill = wireframe;
            setSource(ShaderSources.VERTEX_SHADER_WIREFRAME, vertex);
            setSource(ShaderSources.GEOMETRY_SHADER_WIREFRAME, geometry);
            setSource(ShaderSources.FRAGMENT_SHADER_WIREFRAME, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "window" && _fill != window)
        {
            _fill = window;
            setSource(ShaderSources.VERTEX_SHADER_WINDOW, vertex);
            resetShader(geometry);
            setSource(ShaderSources.FRAGMENT_SHADER_WINDOW, fragment);
            compileProgram();
        }
        return true;
    }, [&]() {
        string fill;
        if (_fill == texture)
            fill = "texture";
        else if (_fill == color)
            fill = "color";
        else if (_fill == uv)
            fill = "uv";
        else if (_fill == wireframe)
            fill = "wireframe";
        else if (_fill == window)
            fill = "window";
        return Values({fill});
    });

    _attribFunctions["color"] = AttributeFunctor([&](Values args) {
        if (args.size() != 4)
            return false;
        _uniforms["_color"].first = args;
        _uniformsToUpdate.push_back("_color");
        return true;
    });

    _attribFunctions["scale"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        else if (args.size() < 3)
        {
            _uniforms["_scale"].first.resize(3);
            _uniforms["_scale"].first[0] = args[0];
            _uniforms["_scale"].first[1] = args[0];
            _uniforms["_scale"].first[2] = args[0];
        }
        else if (args.size() == 3)
            _uniforms["_scale"].first = args;
        else
            return false;

        _uniformsToUpdate.push_back("_scale");

        return true;
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](Values args) {
        if (args.size() != 1)
            return false;

        _sideness = (Shader::Sideness)args[0].asInt();
        _uniforms["_sideness"].first = args;
        _uniformsToUpdate.push_back("_sideness");

        return true;
    }, [&]() {
        return Values({_sideness});
    });

    // Attribute to configure the placement of the various texture input
    _attribFunctions["layout"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1 || args.size() > 4)
            return false;

        _uniforms["_layout"].first = Values({0, 0, 0, 0});
        for (int i = 0; i < args.size() && i < 4; ++i)
        {
            _layout[i] = args[i].asInt();
            _uniforms["_layout"].first[i] = args[i];
        }
        _uniformsToUpdate.push_back("_layout");

        return true;
    }, [&]() {
        Values out;
        for (auto& v : _layout)
            out.push_back(v);
        return out;
    });

    _attribFunctions["uniform"] = AttributeFunctor([&](Values args) {
        if (args.size() < 2)
            return false;

        string uniformName = args[0].asString();
        Values uniformArgs;
        for (int i = 1; i < args.size(); ++i)
            uniformArgs.push_back(args[i]);

        _uniforms[uniformName].first = uniformArgs;
        _uniformsToUpdate.push_back(uniformName);

        return true;
    });
}

} // end of namespace
