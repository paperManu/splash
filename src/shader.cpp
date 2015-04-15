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

    setSource(ShaderSources.VERSION_DIRECTIVE + ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
    setSource(ShaderSources.VERSION_DIRECTIVE + ShaderSources.FRAGMENT_SHADER_TEXTURE, fragment);
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

    for (auto& u : _uniforms)
    {
        if (u.second.type == "buffer")
            glUniformBlockBinding(_program, u.second.glIndex, 1);
    }

    glUseProgram(_program);

    if (_sideness == doubleSided)
    {
        glDisable(GL_CULL_FACE);
    }
    else if (_sideness == singleSided)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else if (_sideness == inverted)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }
}

/*************/
void Shader::deactivate()
{
    glDisable(GL_CULL_FACE);

#ifdef DEBUG
    glUseProgram(0);
#endif
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
    if (_uniforms.find(name) != _uniforms.end())
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        texture->bind();

        glUniform1i(_uniforms[name].glIndex, textureUnit);

        _textures.push_back(texture);
        if (_uniforms.find("_textureNbr") != _uniforms.end())
        {
            _uniforms["_textureNbr"].values = {(int)_textures.size()};
            _uniformsToUpdate.push_back("_textureNbr");
        }
    }
}

/*************/
void Shader::setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp)
{
    glm::mat4 floatMv = (glm::mat4)mv;
    glm::mat4 floatMvp = (glm::mat4)(mp * mv);
    if (_uniforms.find("_modelViewProjectionMatrix") != _uniforms.end())
        glUniformMatrix4fv(_uniforms["_modelViewProjectionMatrix"].glIndex, 1, GL_FALSE, glm::value_ptr(floatMvp));
    if (_uniforms.find("_normalMatrix") != _uniforms.end())
        glUniformMatrix4fv(_uniforms["_normalMatrix"].glIndex, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(floatMv))));
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
        // Remove white spaces
        while (line.substr(0, 1) == " ")
            line = line.substr(1);
        if (line.substr(0, 2) == "//")
            continue;

        string::size_type position;
        if ((position = line.find("layout(std140) uniform")) != string::npos)
        {
            string next = line.substr(position + 23, string::npos);
            string name = next.substr(0, next.find(" "));

            _uniforms[name].type = "buffer";
            _uniforms[name].glIndex = glGetUniformBlockIndex(_program, name.c_str());
            glGenBuffers(1, &_uniforms[name].glBuffer);
            _uniforms[name].glBufferReady = false;
        }
        else
        {
            if ((position = line.find("uniform")) == string::npos)
                continue;

            string next = line.substr(position + 8);
            string type, name;
            type = next.substr(0, next.find(" "));
            next = next.substr(type.size() + 1, next.size());
            name = next.substr(0, next.find(" "));

            if (name.find(";") != string::npos)
                name = name.substr(0, name.size() - 1);
            if (name.find("[") != string::npos)
                name = name.substr(0, name.find("["));

            Values values;
            if (_uniforms.find(name) != _uniforms.end())
                values = _uniforms[name].values;

            _uniforms[name].type = type;

            // Get the location
            _uniforms[name].glIndex = glGetUniformLocation(_program, name.c_str());
            if (type == "int")
                _uniforms[name].values = {0};
            else if (type == "float")
                _uniforms[name].values = {0.f};
            else if (type == "vec2")
                _uniforms[name].values = {0.f, 0.f};
            else if (type == "vec3")
                _uniforms[name].values = {0.f, 0.f, 0.f};
            else if (type == "vec4")
                _uniforms[name].values = {0.f, 0.f, 0.f, 0.f};
            else if (type == "ivec2")
                _uniforms[name].values = {0, 0};
            else if (type == "ivec3")
                _uniforms[name].values = {0, 0, 0};
            else if (type == "ivec4")
                _uniforms[name].values = {0, 0, 0, 0};
            else if (type == "mat3")
                _uniforms[name].values = {0, 0, 0, 0, 0, 0, 0, 0, 0};
            else if (type == "mat4")
                _uniforms[name].values = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            else if (type == "sampler2D" || type == "sampler2DRect")
                _uniforms[name].values = {};
            else
            {
                _uniforms[name].glIndex = -1;
                SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << " - Error while parsing uniforms: " << name << " is of unhandled type " << type << Log::endl;
            }

            if (values.size() != 0)
            {
                _uniforms[name].values = values;
                _uniformsToUpdate.push_back(name);
            }
            else
            {
                // Save the default value
                if (type == "int")
                {
                    int v;
                    glGetUniformiv(_program, _uniforms[name].glIndex, &v);
                    _uniforms[name].values = {v};
                }
                else if (type == "float")
                {
                    float v;
                    glGetUniformfv(_program, _uniforms[name].glIndex, &v);
                    _uniforms[name].values = {v};
                }
                else if (type == "vec2")
                {
                    float v[2];
                    glGetUniformfv(_program, _uniforms[name].glIndex, v);
                    _uniforms[name].values = {v[0], v[1]};
                }
                else if (type == "vec3")
                {
                    float v[3];
                    glGetUniformfv(_program, _uniforms[name].glIndex, v);
                    _uniforms[name].values = {v[0], v[1], v[2]};
                }
                else if (type == "vec4")
                {
                    float v[4];
                    glGetUniformfv(_program, _uniforms[name].glIndex, v);
                    _uniforms[name].values = {v[0], v[1], v[2], v[3]};
                }
                else if (type == "ivec2")
                {
                    int v[2];
                    glGetUniformiv(_program, _uniforms[name].glIndex, v);
                    _uniforms[name].values = {v[0], v[1]};
                }
                else if (type == "ivec3")
                {
                    int v[3];
                    glGetUniformiv(_program, _uniforms[name].glIndex, v);
                    _uniforms[name].values = {v[0], v[1], v[2]};
                }
                else if (type == "ivec4")
                {
                    int v[4];
                    glGetUniformiv(_program, _uniforms[name].glIndex, v);
                    _uniforms[name].values = {v[0], v[1], v[2], v[3]};
                }
            }
        }
    }

    // We parse all uniforms to deactivate of the obsolete ones
    for (auto& u : _uniforms)
    {
        string name = u.first;
        if (u.second.type != "buffer")
        {
            if (glGetUniformLocation(_program, name.c_str()) == -1)
                u.second.glIndex = -1;
        }
        else
        {
            if (glGetUniformBlockIndex(_program, name.c_str()) == -1)
                u.second.glIndex = -1;
        }
    }
}

/*************/
string Shader::stringFromShaderType(int type)
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

            auto uniformIt = _uniforms.find(u);
            if (uniformIt == _uniforms.end())
                continue;

            auto& uniform = uniformIt->second;
            if (uniform.glIndex == -1)
            {
                uniform.values.clear(); // To make sure it is sent next time if the index is correctly set
                continue;
            }

            int size = uniform.values.size();
            int type = uniform.values[0].getType();

            if (type == Value::Type::i)
            {
                if (size == 1)
                    glUniform1i(uniform.glIndex, uniform.values[0].asInt());
                else if (size == 2)
                    glUniform2i(uniform.glIndex, uniform.values[0].asInt(), uniform.values[1].asInt());
                else if (size == 3)
                    glUniform3i(uniform.glIndex, uniform.values[0].asInt(), uniform.values[1].asInt(), uniform.values[2].asInt());
                else if (size == 4)
                    glUniform4i(uniform.glIndex, uniform.values[0].asInt(), uniform.values[1].asInt(), uniform.values[2].asInt(), uniform.values[3].asInt());
            }
            else if (type == Value::Type::f)
            {
                if (size == 1)
                    glUniform1f(uniform.glIndex, uniform.values[0].asFloat());
                else if (size == 2)
                    glUniform2f(uniform.glIndex, uniform.values[0].asFloat(), uniform.values[1].asFloat());
                else if (size == 3)
                    glUniform3f(uniform.glIndex, uniform.values[0].asFloat(), uniform.values[1].asFloat(), uniform.values[2].asFloat());
                else if (size == 4)
                    glUniform4f(uniform.glIndex, uniform.values[0].asFloat(), uniform.values[1].asFloat(), uniform.values[2].asFloat(), uniform.values[3].asFloat());
                else if (size == 9)
                {
                    vector<float> m(9);
                    for (unsigned int i = 0; i < 9; ++i)
                        m[i] = uniform.values[i].asFloat();
                    glUniformMatrix3fv(uniform.glIndex, 1, GL_FALSE, m.data());
                }
            }
            else if (type == Value::Type::v && uniform.values[0].asValues().size() > 0)
            {
                type = uniform.values[0].asValues()[0].getType();
                if (type == Value::Type::i)
                {
                    vector<int> data;
                    if (uniform.type == "buffer")
                    {
                        for (auto& v : uniform.values[0].asValues())
                            data.push_back(v.asInt());

                        glBindBuffer(GL_UNIFORM_BUFFER, uniform.glBuffer);
                        if (!uniform.glBufferReady)
                        {
                            glBufferData(GL_UNIFORM_BUFFER, data.size() * sizeof(int), NULL, GL_STATIC_DRAW);
                            uniform.glBufferReady = true;
                        }
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, data.size() * sizeof(int), data.data());
                        glBindBuffer(GL_UNIFORM_BUFFER, 0);
                        glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniform.glBuffer, 0, data.size() * sizeof(int));
                    }
                    else
                    {
                        for (auto& v : uniform.values[0].asValues())
                            data.push_back(v.asInt());

                        if (uniform.type == "int")
                            glUniform1iv(uniform.glIndex, data.size(), data.data());
                        else if (uniform.type == "ivec2")
                            glUniform2iv(uniform.glIndex, data.size() / 2, data.data());
                        else if (uniform.type == "ivec3")
                            glUniform3iv(uniform.glIndex, data.size() / 3, data.data());
                        else if (uniform.type == "ivec4")
                            glUniform4iv(uniform.glIndex, data.size() / 4, data.data());
                    }
                }
                else if (type == Value::Type::f)
                {
                    vector<float> data;
                    if (uniform.type == "buffer")
                    {
                        for (auto& v : uniform.values[0].asValues())
                            data.push_back(v.asFloat());

                        glBindBuffer(GL_UNIFORM_BUFFER, uniform.glBuffer);
                        if (!uniform.glBufferReady)
                        {
                            glBufferData(GL_UNIFORM_BUFFER, data.size() * sizeof(float), NULL, GL_STATIC_DRAW);
                            uniform.glBufferReady = true;
                        }
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, data.size() * sizeof(float), data.data());
                        glBindBuffer(GL_UNIFORM_BUFFER, 0);
                        glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniform.glBuffer, 0, data.size() * sizeof(float));
                    }
                    else
                    {
                        for (auto& v : uniform.values[0].asValues())
                            data.push_back(v.asFloat());

                        if (uniform.type == "float")
                            glUniform1fv(uniform.glIndex, data.size(), data.data());
                        else if (uniform.type == "vec2")
                            glUniform2fv(uniform.glIndex, data.size() / 2, data.data());
                        else if (uniform.type == "vec3")
                            glUniform3fv(uniform.glIndex, data.size() / 3, data.data());
                        else if (uniform.type == "vec4")
                            glUniform4fv(uniform.glIndex, data.size() / 4, data.data());
                    }
                }
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
    _attribFunctions["blending"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;
        
        _uniforms["_texBlendingMap"].values = args;
        _uniformsToUpdate.push_back("_texBlendingMap");

        return true;
    });

    _attribFunctions["fill"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;

        // Get additionnal shading options
        string options = ShaderSources.VERSION_DIRECTIVE;
        for (int i = 1; i < args.size(); ++i)
            options += "#define " + args[i].asString() + "\n";

        if (args[0].asString() == "texture" && (_fill != texture || _shaderOptions != options))
        {
            _fill = texture;
            _shaderOptions = options;
            setSource(options + ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
            resetShader(geometry);
            setSource(options + ShaderSources.FRAGMENT_SHADER_TEXTURE, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "color" && (_fill != color || _shaderOptions != options))
        {
            _fill = color;
            _shaderOptions = options;
            setSource(options + ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
            resetShader(geometry);
            setSource(options + ShaderSources.FRAGMENT_SHADER_COLOR, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "uv" && (_fill != uv || _shaderOptions != options))
        {
            _fill = uv;
            _shaderOptions = options;
            setSource(options + ShaderSources.VERTEX_SHADER_DEFAULT, vertex);
            resetShader(geometry);
            setSource(options + ShaderSources.FRAGMENT_SHADER_UV, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "wireframe" && (_fill != wireframe || _shaderOptions != options))
        {
            _fill = wireframe;
            _shaderOptions = options;
            setSource(options + ShaderSources.VERTEX_SHADER_WIREFRAME, vertex);
            setSource(options + ShaderSources.GEOMETRY_SHADER_WIREFRAME, geometry);
            setSource(options + ShaderSources.FRAGMENT_SHADER_WIREFRAME, fragment);
            compileProgram();
        }
        else if (args[0].asString() == "window" && (_fill != window || _shaderOptions != options))
        {
            _fill = window;
            _shaderOptions = options;
            setSource(options + ShaderSources.VERTEX_SHADER_WINDOW, vertex);
            resetShader(geometry);
            setSource(options + ShaderSources.FRAGMENT_SHADER_WINDOW, fragment);
            compileProgram();
        }
        return true;
    }, [&]() -> Values {
        string fill;
        if (_fill == texture)
            fill = "texture";
        else if (_fill == texture_rect)
            fill = "texture_rect";
        else if (_fill == color)
            fill = "color";
        else if (_fill == uv)
            fill = "uv";
        else if (_fill == wireframe)
            fill = "wireframe";
        else if (_fill == window)
            fill = "window";
        return {fill};
    });

    _attribFunctions["color"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 4)
            return false;
        _uniforms["_color"].values = args;
        _uniformsToUpdate.push_back("_color");
        return true;
    });

    _attribFunctions["scale"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        else if (args.size() < 3)
        {
            _uniforms["_scale"].values.resize(3);
            _uniforms["_scale"].values[0] = args[0];
            _uniforms["_scale"].values[1] = args[0];
            _uniforms["_scale"].values[2] = args[0];
        }
        else if (args.size() == 3)
            _uniforms["_scale"].values = args;
        else
            return false;

        _uniformsToUpdate.push_back("_scale");

        return true;
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;

        _sideness = (Shader::Sideness)args[0].asInt();
        return true;
    }, [&]() -> Values {
        return {_sideness};
    });

    // Attribute to configure the placement of the various texture input
    _attribFunctions["layout"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1 || args.size() > 4)
            return false;

        _uniforms["_layout"].values = {0, 0, 0, 0};
        for (int i = 0; i < args.size() && i < 4; ++i)
        {
            _layout[i] = args[i].asInt();
            _uniforms["_layout"].values[i] = args[i];
        }
        _uniformsToUpdate.push_back("_layout");

        return true;
    }, [&]() {
        Values out;
        for (auto& v : _layout)
            out.push_back(v);
        return out;
    });

    _attribFunctions["uniform"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 2)
            return false;

        string uniformName = args[0].asString();
        Values uniformArgs;
        for (int i = 1; i < args.size(); ++i)
            uniformArgs.push_back(args[i]);

        // Check if the values changed from previous use
        auto uniformIt = _uniforms.find(uniformName);
        if (uniformIt != _uniforms.end() && Value(uniformArgs) == Value(uniformIt->second.values))
            return true;
        else if (uniformIt == _uniforms.end())
            uniformIt = (_uniforms.emplace(make_pair(uniformName, Uniform()))).first;

        uniformIt->second.values = uniformArgs;
        _uniformsToUpdate.push_back(uniformName);

        return true;
    });
}

} // end of namespace
