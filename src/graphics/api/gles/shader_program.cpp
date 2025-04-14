#include "./graphics/api/gles/shader_program.h"

#include <regex>

#include "./graphics/api/gles/gl_utils.h"
#include "./utils/log.h"

namespace Splash::gfx::gles
{
/*************/
Program::Program(ProgramType type)
    : _type(type)
{
    _program = glCreateProgram();
}

/*************/
Program::~Program()
{
    if (glIsProgram(_program))
        glDeleteProgram(_program);
}

/*************/
bool Program::activate()
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    if (_isActive)
        return true;

    if (!_isLinked && !link())
        return false;

    _isActive = true;

    if (_type == ProgramType::Graphic)
        for (const auto& [name, uniform] : _uniforms)
            if (uniform.type == "buffer")
                glUniformBlockBinding(_program, uniform.glIndex, 1);

    glUseProgram(_program);
    updateUniforms();

    return true;
}

/*************/
void Program::deactivate()
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    glUseProgram(0);
    _isActive = false;
}

/*************/
void Program::attachShaderStage(const ShaderStage& stage)
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    assert(stage.isValid());
    const auto type = stage.getType();
    if (const auto shaderIt = _shaderStages.find(type); shaderIt != _shaderStages.end())
        glDetachShader(_program, shaderIt->second->getId());

    glAttachShader(_program, stage.getId());
    _shaderStages[type] = &stage;
    _isLinked = false;
}

/*************/
void Program::detachShaderStage(gfx::ShaderType type)
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    auto shaderIt = _shaderStages.find(type);
    if (shaderIt != _shaderStages.end())
        glDetachShader(_program, shaderIt->second->getId());
    _shaderStages.erase(type);
    _isLinked = false;
}

/*************/
bool Program::isValid() const
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    return glIsProgram(_program);
}

/*************/
std::string Program::getProgramInfoLog() const
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    GLint length;
    glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &length);
    auto str = std::string(static_cast<size_t>(length), '\0'); // To avoid printing a bunch of 0s
    glGetProgramInfoLog(_program, length, &length, str.data());
    return str;
}

/*************/
const std::map<std::string, Values> Program::getUniformValues() const
{
    std::map<std::string, Values> uniforms;
    for (const auto& [name, uniform] : _uniforms)
        uniforms[name] = uniform.values;
    return uniforms;
}

/*************/
bool Program::link()
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    GLint status;
    glLinkProgram(_program);
    glGetProgramiv(_program, GL_LINK_STATUS, &status);

    if (status == GL_TRUE)
    {
#ifdef DEBUG
        Log::get() << Log::DEBUGGING << "Program::" << __FUNCTION__ << " - Shader program " << _programName << " linked successfully" << Log::endl;
#endif
        for (auto stage : _shaderStages)
            parseUniforms(stage.second->getSource());

        _isLinked = true;
        return true;
    }
    else
    {
        Log::get() << Log::WARNING << "Program::" << __FUNCTION__ << " - Error while linking the shader program " << _programName << Log::endl;
        const auto log = getProgramInfoLog();
        Log::get() << Log::WARNING << "Program::" << __FUNCTION__ << " - Error log: \n" << log << Log::endl;
        _isLinked = false;

        return false;
    }
}

/*************/
void Program::parseUniforms(std::string_view source)
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    const auto sourceAsStr = std::string(source);
    std::istringstream input(sourceAsStr);
    for (std::string line; getline(input, line);)
    {
        // Remove white spaces
        while (line.substr(0, 1) == " ")
            line = line.substr(1);
        if (line.substr(0, 2) == "//")
            continue;

        std::string::size_type position;
        if ((position = line.find("layout(std140) uniform")) != std::string::npos)
        {
            std::string next = line.substr(position + 23, std::string::npos);
            std::string name = next.substr(0, next.find(' '));

            _uniforms[name].type = "buffer";
            _uniforms[name].glIndex = glGetUniformBlockIndex(_program, name.c_str());
            glGenBuffers(1, &_uniforms[name].glBuffer);
            _uniforms[name].glBufferReady = false;
        }
        else
        {
            if (line.find("uniform") == std::string::npos)
                continue;

            std::string type, name;
            std::string documentation = "";
            uint32_t elementSize = 1;
            uint32_t arraySize = 1;

            static const auto regType = std::regex("[ ]*uniform ([[:alpha:]]*([[:digit:]]?[D]?)) ([_[:alnum:]]*)[\\[]?([^] ;]*)[]]?[^/]*(.*)", std::regex_constants::extended);
            std::smatch regMatch;
            if (regex_match(line, regMatch, regType))
            {
                type = regMatch[1].str();
                name = regMatch[3].str();
                auto elementSizeStr = regMatch[2].str();
                auto arraySizeStr = regMatch[4].str();
                documentation = regMatch[5].str();

                if (documentation.find("//") != std::string::npos)
                    documentation = documentation.substr(documentation.find("//") + 2);
                else
                    documentation.clear();

                try
                {
                    elementSize = std::stoi(elementSizeStr);
                }
                catch (...)
                {
                    elementSize = 1;
                }

                try
                {
                    arraySize = std::stoi(arraySizeStr);
                }
                catch (...)
                {
                    arraySize = 0;
                }
            }
            else
            {
                continue;
            }

            const auto uniformIndex = glGetUniformLocation(_program, name.c_str());

            if (uniformIndex == -1)
            {
                // If a uniform is unused (perhaps due to its code path being removed by an ifdef), the compiler removes it from the shader altogether, as if it's never been there.
                // I think renderdoc can be helpful with debugging this kind of stuff. Anyway, it might not be strictly an error depending on how you structure the code.
                // In Splash, the ifdef trick is used a lot to enable/disable features, so some uniforms are removed between different shader versions.
                Log::get() << Log::DEBUGGING << "Program::" << __FUNCTION__ << " - Uniform \"" << name << "\" with type \"" << type << "\" "
                           << "was not found in the shader. You might have forgotten it or it might have been optimized out" << Log::endl;

                continue;
            }

            _uniforms[name].type = type;
            _uniforms[name].glIndex = uniformIndex;
            _uniforms[name].elementSize = type.find("mat") != std::string::npos ? elementSize * elementSize : elementSize;
            _uniforms[name].arraySize = arraySize;
            _uniformsDocumentation[name] = documentation;

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
            else if (type == "mat3")
            {
                _uniforms[name].values = {0, 0, 0, 0, 0, 0, 0, 0, 0};
            }
            else if (type == "mat4")
            {
                _uniforms[name].values = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            }
            else if (type.find("sampler") != std::string::npos)
            {
                _uniforms[name].values = {};
            }
            else
            {
                _uniforms[name].glIndex = -1;
                Log::get() << Log::WARNING << "Program::" << __FUNCTION__ << " - Error while parsing uniforms: " << name << " is of unhandled type " << type << Log::endl;
            }
        }
    }

    // We parse all uniforms to deactivate the obsolete ones
    for (auto& [name, uniform] : _uniforms)
    {
        if (uniform.type != "buffer")
        {
            if (glGetUniformLocation(_program, name.data()) == -1)
                uniform.glIndex = -1;
        }
        else
        {
            if (glGetUniformBlockIndex(_program, name.data()) == GL_INVALID_INDEX)
                uniform.glIndex = -1;
        }
    }
}

/*************/
void Program::selectVaryings(const std::vector<std::string>& varyingNames)
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    assert(_type == ProgramType::Feedback);
    std::vector<const GLchar*> varyingNamesAsChar(varyingNames.size(), nullptr);
    for (uint32_t i = 0; i < varyingNames.size(); ++i)
        varyingNamesAsChar[i] = static_cast<const GLchar*>(varyingNames[i].data());

    glTransformFeedbackVaryings(_program, static_cast<GLsizei>(varyingNames.size()), varyingNamesAsChar.data(), GL_SEPARATE_ATTRIBS);
}

/*************/
void Program::updateUniforms()
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__));

    if (_isActive)
    {
        for (uint32_t i = 0; i < _uniformsToUpdate.size(); ++i)
        {
            const auto& uniformName = _uniformsToUpdate[i];
            const auto uniformIt = _uniforms.find(uniformName);
            if (uniformIt == _uniforms.end())
                continue;

            auto& uniform = uniformIt->second;
            if (uniform.glIndex == -1)
            {
                uniform.values.clear(); // To make sure it is sent next time if the index is correctly set
                continue;
            }

            const auto& type = uniform.type;
            assert(!type.empty());

            if (uniform.arraySize == 0)
            {
                if (uniform.elementSize != uniform.values.size())
                    continue;

                if (type == "int")
                    glUniform1i(uniform.glIndex, uniform.values[0].as<int>());
                else if (type == "ivec2")
                    glUniform2i(uniform.glIndex, uniform.values[0].as<int>(), uniform.values[1].as<int>());
                else if (type == "ivec3")
                    glUniform3i(uniform.glIndex, uniform.values[0].as<int>(), uniform.values[1].as<int>(), uniform.values[2].as<int>());
                else if (type == "ivec4")
                    glUniform4i(uniform.glIndex, uniform.values[0].as<int>(), uniform.values[1].as<int>(), uniform.values[2].as<int>(), uniform.values[3].as<int>());
                else if (type == "float")
                    glUniform1f(uniform.glIndex, uniform.values[0].as<float>());
                else if (type == "vec2")
                    glUniform2f(uniform.glIndex, uniform.values[0].as<float>(), uniform.values[1].as<float>());
                else if (type == "vec3")
                    glUniform3f(uniform.glIndex, uniform.values[0].as<float>(), uniform.values[1].as<float>(), uniform.values[2].as<float>());
                else if (type == "vec4")
                    glUniform4f(uniform.glIndex, uniform.values[0].as<float>(), uniform.values[1].as<float>(), uniform.values[2].as<float>(), uniform.values[3].as<float>());
                else if (type == "mat3")
                {
                    std::vector<float> m(9);
                    for (unsigned int i = 0; i < 9; ++i)
                        m[i] = uniform.values[i].as<float>();
                    glUniformMatrix3fv(uniform.glIndex, 1, GL_FALSE, m.data());
                }
                else if (type == "mat4")
                {
                    std::vector<float> m(16);
                    for (unsigned int i = 0; i < 16; ++i)
                        m[i] = uniform.values[i].as<float>();
                    glUniformMatrix4fv(uniform.glIndex, 1, GL_FALSE, m.data());
                }
            }
            else
            {
                if (type == "int" || type.find("ivec") != std::string::npos)
                {
                    std::vector<int> data;
                    for (auto& v : uniform.values)
                        data.push_back(v.as<int>());

                    if (uniform.type == "buffer")
                    {
                        glBindBuffer(GL_UNIFORM_BUFFER, uniform.glBuffer);
                        if (!uniform.glBufferReady)
                        {
                            glBufferData(GL_UNIFORM_BUFFER, data.size() * sizeof(int), nullptr, GL_STATIC_DRAW);
                            uniform.glBufferReady = true;
                        }
                        glBufferSubData(GL_UNIFORM_BUFFER, 0, data.size() * sizeof(int), data.data());
                        glBindBuffer(GL_UNIFORM_BUFFER, 0);
                        glBindBufferRange(GL_UNIFORM_BUFFER, 1, uniform.glBuffer, 0, data.size() * sizeof(int));
                    }
                    else
                    {
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
                else if (type == "float" || type.find("vec") != std::string::npos)
                {
                    std::vector<float> data;
                    for (auto& v : uniform.values)
                        data.push_back(v.as<float>());

                    if (uniform.type == "buffer")
                    {
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
bool Program::setUniform(const std::string& name, const Value& value)
{
    auto uniformIt = _uniforms.find(name);
    if (uniformIt == _uniforms.end())
        return false;

    if (uniformIt->second.glIndex == -1)
        return false;

    if (value.getType() == Value::Type::values)
        uniformIt->second.values = value.as<Values>();
    else
        uniformIt->second.values = {value};
    _uniformsToUpdate.emplace_back(name);

    updateUniforms();

    return true;
}

/*************/
bool Program::setUniform(const std::string& name, const glm::mat4 mat)
{
    OnGLESScopeExit(std::string("Program::").append(__FUNCTION__).append("::").append(name));

    const auto uniformIt = _uniforms.find(name);
    if (uniformIt == _uniforms.end())
        return false;

    if (uniformIt->second.glIndex == -1)
        return false;

    glUniformMatrix4fv(uniformIt->second.glIndex, 1, GL_FALSE, glm::value_ptr(mat));
    return true;
}

} // namespace Splash::gfx::gles
