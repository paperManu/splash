#include "shader.h"
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
        _locationMVP = glGetUniformLocation(_program, "_modelViewProjectionMatrix");
        _locationNormalMatrix = glGetUniformLocation(_program, "_normalMatrix");
        _locationSide = glGetUniformLocation(_program, "_sideness");
        _locationTextureNbr = glGetUniformLocation(_program, "_textureNbr");
        _locationBlendingMap = glGetUniformLocation(_program, "_texBlendingMap");
        _locationBlendWidth = glGetUniformLocation(_program, "_blendWidth");
        _locationBlackLevel = glGetUniformLocation(_program, "_blackLevel");
        _locationBrightness = glGetUniformLocation(_program, "_brightness");
        _locationColor = glGetUniformLocation(_program, "_color");
        _locationScale = glGetUniformLocation(_program, "_scale");
        _locationLayout = glGetUniformLocation(_program, "_layout");
    }

    glUseProgram(_program);

    _textureNbr = 0;

    glUniform1i(_locationSide, _sideness);
    glUniform1i(_locationTextureNbr, _textureNbr);
    glUniform1i(_locationBlendingMap, _useBlendingMap);
    glUniform1f(_locationBlendWidth, _blendWidth);
    glUniform1f(_locationBlackLevel, _blackLevel);
    glUniform1f(_locationBrightness, _brightness);
    glUniform3f(_locationScale, _scale.x, _scale.y, _scale.z);
    glUniform4f(_locationColor, _color.r, _color.g, _color.b, _color.a);
    glUniform4i(_locationLayout, _layout[0], _layout[1], _layout[2], _layout[3]);
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
    _mutex.unlock();
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
    glBindTexture(GL_TEXTURE_2D, texture->getTexId());
    GLint uniform = glGetUniformLocation(_program, name.c_str());
    glUniform1i(uniform, textureUnit);

    _textureNbr++;
    glUniform1i(_locationTextureNbr, _textureNbr);
}

/*************/
void Shader::setModelViewProjectionMatrix(const glm::dmat4& mvp)
{
    glm::mat4 floatMvp = (glm::mat4)mvp;
    glUniformMatrix4fv(_locationMVP, 1, GL_FALSE, glm::value_ptr(floatMvp));
    glUniformMatrix4fv(_locationNormalMatrix, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(floatMvp))));
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

        // Free the previous uniform locations
        _uniforms.clear();
    
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

        if (type == "int")
            _uniforms[name] = pair<Values, GLint>({0}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "float")
            _uniforms[name] = pair<Values, GLint>({0.f}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "vec3")
            _uniforms[name] = pair<Values, GLint>({0.f, 0.f, 0.f}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "ivec4")
            _uniforms[name] = pair<Values, GLint>({0, 0, 0, 0}, glGetUniformLocation(_program, name.c_str()));
        else if (type == "mat4")
            _uniforms[name] = pair<Values, GLint>({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, glGetUniformLocation(_program, name.c_str()));
        else if (type != "sampler2D")
            SLog::log << Log::WARNING << "Shader::" << __FUNCTION__ << "Error while parsing uniforms: " << name << " is of unhandled type " << type << Log::endl;
        cout << "Uniform: " << name << " -> " << _uniforms[name].second << endl;
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
    _attribFunctions["fill"] = AttributeFunctor([&](vector<Value> args) {
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
        return vector<Value>({fill});
    });

    _attribFunctions["color"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 4)
            return false;
        _color = glm::dvec4(args[0].asFloat(), args[1].asFloat(), args[2].asFloat(), args[3].asFloat());
        return true;
    });

    _attribFunctions["scale"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;

        if (args.size() < 3)
            _scale = glm::dvec3(args[0].asFloat(), args[0].asFloat(), args[0].asFloat());
        else
            _scale = glm::dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());

        return true;
    });

    _attribFunctions["blendWidth"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _blendWidth = args[0].asFloat();
        return true;
    });

    _attribFunctions["blackLevel"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _blackLevel = args[0].asFloat();
        return true;
    });

    _attribFunctions["brightness"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _brightness = args[0].asFloat();
        return true;
    });

    // Attribute to configure the placement of the various texture input
    _attribFunctions["layout"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        for (int i = 0; i < args.size() && i < 4; ++i)
            _layout[i] = args[i].asInt();
        return true;
    }, [&]() {
        vector<Value> out;
        for (auto& v : _layout)
            out.push_back(v);
        return out;
    });
}

} // end of namespace
