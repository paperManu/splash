#include "./graphics/shader.h"

#include <array>
#include <fstream>
#include <regex>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "./graphics/api/compute_shader_gfx_impl.h"
#include "./graphics/api/feedback_shader_gfx_impl.h"
#include "./graphics/api/graphic_shader_gfx_impl.h"
#include "./graphics/programSources.h"
#include "./graphics/shaderSources.h"
#include "./utils/files.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Shader::Shader(gfx::Renderer* renderer, ProgramType type)
    : _renderer(renderer)
{
    if (type == prgGraphic)
    {
        _gfxImpl = _renderer->createGraphicShader();
        selectFillMode("texture");
    }
    else if (type == prgCompute)
    {
        _gfxImpl = _renderer->createComputeShader();
        selectComputePhase(ComputePhase::ResetVisibility);
    }
    else if (type == prgFeedback)
    {
        _gfxImpl = _renderer->createFeedbackShader();
        selectFeedbackPhase(FeedbackPhase::TessellateFromCamera);
    }
}

/*************/
void Shader::activate()
{
    _gfxImpl->activate();
}

/*************/
void Shader::deactivate()
{
    _gfxImpl->deactivate();
}

/*************/
bool Shader::doCompute(GLuint numGroupsX, GLuint numGroupsY)
{
    auto computeShader = dynamic_cast<gfx::ComputeShaderGfxImpl*>(_gfxImpl.get());
    assert(computeShader != nullptr);
    return computeShader->compute(numGroupsX, numGroupsY);
}

/*************/
std::map<std::string, Values> Shader::getUniforms() const
{
    const auto uniformValues = _gfxImpl->getUniformValues();
    std::map<std::string, Values> uniforms;
    for (const auto& [name, value] : uniformValues)
        uniforms[std::string(name)] = value;
    return uniforms;
}

/*************/
std::map<std::string, std::string> Shader::getUniformsDocumentation() const
{
    return _gfxImpl->getUniformsDocumentation();
}

/*************/
void Shader::selectComputePhase(ComputePhase phase)
{
    std::string options = ShaderSources::VERSION_DIRECTIVE_GL32_ES;

    switch (phase)
    {
    default:
        assert(false);
        break;
    case ComputePhase::ResetVisibility:
        setSource(gfx::ShaderType::compute, options + ShaderSources::COMPUTE_SHADER_RESET_VISIBILITY);
        break;
    case ComputePhase::ResetBlending:
        setSource(gfx::ShaderType::compute, options + ShaderSources::COMPUTE_SHADER_RESET_BLENDING);
        break;
    case ComputePhase::ComputeCameraContribution:
        setSource(gfx::ShaderType::compute, options + ShaderSources::COMPUTE_SHADER_COMPUTE_CAMERA_CONTRIBUTION);
        break;
    case ComputePhase::TransferVisibilityToAttribute:
        setSource(gfx::ShaderType::compute, options + ShaderSources::COMPUTE_SHADER_TRANSFER_VISIBILITY_TO_ATTR);
        break;
    }
}

/*************/
void Shader::selectFeedbackPhase(FeedbackPhase phase, const std::vector<std::string>& varyings)
{
    auto feedbackShader = dynamic_cast<gfx::FeedbackShaderGfxImpl*>(_gfxImpl.get());
    assert(feedbackShader != nullptr);

    std::string options = ShaderSources::VERSION_DIRECTIVE_GL32_ES;

    switch (phase)
    {
    default:
        assert(false);
        break;
    case FeedbackPhase::TessellateFromCamera:
        setSource(gfx::ShaderType::vertex, options + ShaderSources::VERTEX_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
        setSource(gfx::ShaderType::tess_ctrl, options + ShaderSources::TESS_CTRL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
        setSource(gfx::ShaderType::tess_eval, options + ShaderSources::TESS_EVAL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
        setSource(gfx::ShaderType::geometry, options + ShaderSources::GEOMETRY_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
        setSource(gfx::ShaderType::fragment, options + ShaderSources::FRAGMENT_SHADER_EMPTY);
        break;
    }

    feedbackShader->selectVaryings(varyings);
}

/*************/
void Shader::setCulling(const Culling culling)
{
    _gfxImpl->setCulling(static_cast<gfx::Culling>(culling));
}

/*************/
Shader::Culling Shader::getCulling() const
{
    return static_cast<Shader::Culling>(_gfxImpl->getCulling());
}

/*************/
void Shader::setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp)
{
    auto graphicShader = dynamic_cast<gfx::GraphicShaderGfxImpl*>(_gfxImpl.get());
    assert(graphicShader != nullptr);
    graphicShader->setModelViewProjectionMatrix(mv, mp);
}

/*************/
bool Shader::setSource(const gfx::ShaderType type, const std::string& src)
{
    const auto parsedSources = parseIncludes(src);
    _currentSources[type] = parsedSources;
    return _gfxImpl->setSource(type, parsedSources);
}

/*************/
bool Shader::setSource(const std::map<gfx::ShaderType, std::string>& sources)
{
    _gfxImpl->removeShaderType(gfx::ShaderType::vertex);
    _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
    _gfxImpl->removeShaderType(gfx::ShaderType::fragment);

    bool status = true;
    if (sources.find(gfx::ShaderType::vertex) == sources.end())
        status &= setSource(gfx::ShaderType::vertex, ShaderSources::VERSION_DIRECTIVE_GL32_ES + ShaderSources::VERTEX_SHADER_TEXTURE);
    for (const auto& [shaderType, source] : sources)
        status &= setSource(shaderType, source);

    return status;
}

/*************/
bool Shader::setSourceFromFile(const gfx::ShaderType type, const std::string& filename)
{
    const auto content = Utils::getTextFileContent(filename);
    if (!content.empty())
    {
        return setSource(type, content);
    }
    else
    {
        Log::get() << Log::WARNING << "Shader::" << __FUNCTION__ << " - Unable to load file " << filename << Log::endl;
        return false;
    }
}

/*************/
void Shader::setTexture(const std::shared_ptr<Texture>& texture, const GLuint textureUnit, const std::string& name)
{
    auto graphicShader = dynamic_cast<gfx::GraphicShaderGfxImpl*>(_gfxImpl.get());
    assert(graphicShader != nullptr);
    graphicShader->setTexture(texture.get(), textureUnit, name);
}

/*************/
void Shader::setUniform(const std::string& name, const Value& value)
{
    _gfxImpl->setUniform(name, value);
}

/*************/
std::string Shader::parseIncludes(const std::string& src)
{
    std::string finalSources = "";

    std::istringstream input(src);
    for (std::string line; getline(input, line);)
    {
        // Remove white spaces
        while (line.substr(0, 1) == " ")
            line = line.substr(1);
        if (line.substr(0, 2) == "//")
            continue;

        std::string::size_type position;
        if ((position = line.find("#include")) != std::string::npos)
        {
            std::string includeName = line.substr(position + 9, std::string::npos);
            auto includeIt = ShaderSources::INCLUDES.find(includeName);
            if (includeIt == ShaderSources::INCLUDES.end())
            {
                Log::get() << Log::WARNING << "Shader::" << __FUNCTION__ << " - Could not find included shader named " << includeName << Log::endl;
            }
            else
            {
                finalSources += (*includeIt).second + "\n";
            }
        }
        else
        {
            finalSources += line + "\n";
        }
    }

    return finalSources;
}

/*************/
void Shader::selectFillMode(std::string_view mode, const std::vector<std::string>& parameters)
{
    std::string options = ShaderSources::VERSION_DIRECTIVE_GL32_ES;
    for (const auto& param : parameters)
        options.append("#define ").append(param).append("\n");

    const auto& programSources = Splash::ProgramSources::Sources;

    if (_currentProgramName == mode && _shaderOptions == options)
        return;

    _currentProgramName = mode;
    _shaderOptions = options;

    if (_currentProgramName == "userDefined")
    {
        _gfxImpl->removeShaderType(gfx::ShaderType::tess_eval);
        _gfxImpl->removeShaderType(gfx::ShaderType::tess_ctrl);
        _gfxImpl->removeShaderType(gfx::ShaderType::compute);

        if (_currentSources.find(gfx::ShaderType::vertex) == _currentSources.end())
            setSource(gfx::ShaderType::vertex, options + ShaderSources::VERTEX_SHADER_TEXTURE);
        if (_currentSources.find(gfx::ShaderType::fragment) == _currentSources.end())
            setSource(gfx::ShaderType::fragment, options + ShaderSources::FRAGMENT_SHADER_TEXTURE);

        return;
    }

    if (const auto& programSourceIt = programSources.find(_currentProgramName); programSourceIt != programSources.end())
    {
        const auto& programSource = programSourceIt->second;

        const auto applyShader = [&](gfx::ShaderType type) {
            const auto source = programSource[type];
            if (source.empty())
                _gfxImpl->removeShaderType(type);
            else
                setSource(type, options + std::string(source));
        };

        applyShader(gfx::ShaderType::vertex);
        applyShader(gfx::ShaderType::tess_ctrl);
        applyShader(gfx::ShaderType::tess_eval);
        applyShader(gfx::ShaderType::geometry);
        applyShader(gfx::ShaderType::fragment);
        applyShader(gfx::ShaderType::compute);

        return;
    }

    Log::get() << Log::WARNING << "Shader::" << __FUNCTION__ << " - Unknown fill mode " << mode << Log::endl;
}

} // namespace Splash
