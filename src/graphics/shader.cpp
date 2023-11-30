#include "./graphics/shader.h"

#include <array>
#include <fstream>
#include <regex>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "./graphics/api/compute_shader.h"
#include "./graphics/api/feedback_shader.h"
#include "./graphics/api/graphic_shader.h"
#include "./graphics/shaderSources.h"
#include "./utils/log.h"
#include "./utils/timer.h"

namespace Splash
{

/*************/
Shader::Shader(RootObject* root, ProgramType type)
    : GraphObject(root)
{
    _type = "shader";

    if (type == prgGraphic)
    {
        _gfxImpl = _renderer->createGraphicShader();
        registerGraphicAttributes();
        setAttribute("fill", {"texture"});
    }
    else if (type == prgCompute)
    {
        _gfxImpl = _renderer->createComputeShader();
        registerComputeAttributes();
        setAttribute("computePhase", {"resetVisibility"});
    }
    else if (type == prgFeedback)
    {
        _gfxImpl = _renderer->createFeedbackShader();
        registerFeedbackAttributes();
        setAttribute("feedbackPhase", {"tessellateFromCamera"});
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
        status &= setSource(gfx::ShaderType::vertex, ShaderSources.VERSION_DIRECTIVE_GL32_ES + ShaderSources.VERTEX_SHADER_DEFAULT);
    for (const auto& [shaderType, source] : sources)
        status &= setSource(shaderType, source);

    return status;
}

/*************/
bool Shader::setSourceFromFile(const gfx::ShaderType type, const std::string& filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return setSource(type, contents);
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
            auto includeIt = ShaderSources.INCLUDES.find(includeName);
            if (includeIt == ShaderSources.INCLUDES.end())
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
void Shader::registerGraphicAttributes()
{
    addAttribute(
        "fill",
        [&](const Values& args) {
            assert(dynamic_cast<gfx::GraphicShaderGfxImpl*>(_gfxImpl.get()) != nullptr);
            // Get additionnal shading options
            std::string options = ShaderSources.VERSION_DIRECTIVE_GL32_ES;
            for (uint32_t i = 1; i < args.size(); ++i)
                options += "#define " + args[i].as<std::string>() + "\n";

            if (args[0].as<std::string>() == "texture" && (_fill != texture || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = texture;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_TEXTURE);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_TEXTURE);
            }
            else if (args[0].as<std::string>() == "object_cubemap" && (_fill != object_cubemap || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = object_cubemap;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_OBJECT_CUBEMAP);
                setSource(gfx::ShaderType::geometry, options + ShaderSources.GEOMETRY_SHADER_OBJECT_CUBEMAP);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_OBJECT_CUBEMAP);
            }
            else if (args[0].as<std::string>() == "cubemap_projection" && (_fill != cubemap_projection || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = object_cubemap;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_CUBEMAP_PROJECTION);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_CUBEMAP_PROJECTION);
            }
            else if (args[0].as<std::string>() == "image_filter" && (_fill != image_filter || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = image_filter;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_FILTER);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_IMAGE_FILTER);
            }
            else if (args[0].as<std::string>() == "blacklevel_filter" && (_fill != blacklevel_filter || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = blacklevel_filter;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_FILTER);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_BLACKLEVEL_FILTER);
            }
            else if (args[0].as<std::string>() == "color_curves_filter" && (_fill != color_curves_filter || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = color_curves_filter;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_FILTER);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_COLOR_CURVES_FILTER);
            }
            else if (args[0].as<std::string>() == "color" && (_fill != color || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = color;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_MODELVIEW);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_COLOR);
            }
            else if (args[0].as<std::string>() == "primitiveId" && (_fill != primitiveId || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = primitiveId;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_MODELVIEW);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_PRIMITIVEID);
            }
            else if (args[0].as<std::string>() == "userDefined" && (_fill != userDefined || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = userDefined;
                _shaderOptions = options;
                if (_shadersSource.find(static_cast<int>(gfx::ShaderType::vertex)) == _shadersSource.end())
                    setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_DEFAULT);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                if (_shadersSource.find(static_cast<int>(gfx::ShaderType::fragment)) == _shadersSource.end())
                    setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_DEFAULT_FILTER);
            }
            else if (args[0].as<std::string>() == "uv" && (_fill != uv || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = uv;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_MODELVIEW);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_UV);
            }
            else if (args[0].as<std::string>() == "warp" && (_fill != warp || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = warp;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_WARP);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_WARP);
            }
            else if (args[0].as<std::string>() == "warpControl" && (_fill != warpControl || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = warpControl;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_WARP_WIREFRAME);
                setSource(gfx::ShaderType::geometry, options + ShaderSources.GEOMETRY_SHADER_WARP_WIREFRAME);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_WARP_WIREFRAME);
            }
            else if (args[0].as<std::string>() == "wireframe" && (_fill != wireframe || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = wireframe;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_WIREFRAME);
                setSource(gfx::ShaderType::geometry, options + ShaderSources.GEOMETRY_SHADER_WIREFRAME);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_WIREFRAME);
            }
            else if (args[0].as<std::string>() == "window" && (_fill != window || _shaderOptions != options))
            {
                _currentProgramName = args[0].as<std::string>();
                _fill = window;
                _shaderOptions = options;
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_WINDOW);
                _gfxImpl->removeShaderType(gfx::ShaderType::geometry);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_WINDOW);
            }
            return true;
        },
        [&]() -> Values {
            std::string fill;
            if (_fill == texture)
                fill = "texture";
            else if (_fill == object_cubemap)
                fill = "object_cubemap";
            else if (_fill == color)
                fill = "color";
            else if (_fill == uv)
                fill = "uv";
            else if (_fill == wireframe)
                fill = "wireframe";
            else if (_fill == window)
                fill = "window";
            return {fill};
        },
        {'s'});
    setAttributeDescription("fill", "Set the filling mode");

    addAttribute(
        "sideness",
        [&](const Values& args) {
            _sideness = (Shader::Sideness)args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {(int)_sideness}; },
        {'i'});
    setAttributeDescription("sideness", "If set to 0 or 1, the object is single-sided (back or front-sided). If set to 2, it is double-sided");
}

/*************/
void Shader::registerComputeAttributes()
{
    addAttribute("computePhase",
        [&](const Values& args) {
            assert(dynamic_cast<gfx::ComputeShaderGfxImpl*>(_gfxImpl.get()) != nullptr);
            if (args.size() < 1)
                return false;

            // Get additionnal shading options
            std::string options = ShaderSources.VERSION_DIRECTIVE_GL32_ES;
            for (uint32_t i = 1; i < args.size(); ++i)
                options += "#define " + args[i].as<std::string>() + "\n";

            if ("resetVisibility" == args[0].as<std::string>())
            {
                _currentProgramName = args[0].as<std::string>();
                setSource(gfx::ShaderType::compute, options + ShaderSources.COMPUTE_SHADER_RESET_VISIBILITY);
            }
            else if ("resetBlending" == args[0].as<std::string>())
            {
                _currentProgramName = args[0].as<std::string>();
                setSource(gfx::ShaderType::compute, options + ShaderSources.COMPUTE_SHADER_RESET_BLENDING);
            }
            else if ("computeCameraContribution" == args[0].as<std::string>())
            {
                _currentProgramName = args[0].as<std::string>();
                setSource(gfx::ShaderType::compute, options + ShaderSources.COMPUTE_SHADER_COMPUTE_CAMERA_CONTRIBUTION);
            }
            else if ("transferVisibilityToAttr" == args[0].as<std::string>())
            {
                _currentProgramName = args[0].as<std::string>();
                setSource(gfx::ShaderType::compute, options + ShaderSources.COMPUTE_SHADER_TRANSFER_VISIBILITY_TO_ATTR);
            }

            return true;
        },
        {});
}

/*************/
void Shader::registerFeedbackAttributes()
{
    GraphObject::registerAttributes();

    addAttribute("feedbackPhase",
        [&](const Values& args) {
            assert(dynamic_cast<gfx::FeedbackShaderGfxImpl*>(_gfxImpl.get()) != nullptr);
            if (args.size() < 1)
                return false;

            // Get additionnal shader options
            std::string options = ShaderSources.VERSION_DIRECTIVE_GL32_ES;
            for (uint32_t i = 1; i < args.size(); ++i)
                options += "#define " + args[i].as<std::string>() + "\n";

            if ("tessellateFromCamera" == args[0].as<std::string>())
            {
                _currentProgramName = args[0].as<std::string>();
                setSource(gfx::ShaderType::vertex, options + ShaderSources.VERTEX_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
                setSource(gfx::ShaderType::tess_ctrl, options + ShaderSources.TESS_CTRL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
                setSource(gfx::ShaderType::tess_eval, options + ShaderSources.TESS_EVAL_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
                setSource(gfx::ShaderType::geometry, options + ShaderSources.GEOMETRY_SHADER_FEEDBACK_TESSELLATE_FROM_CAMERA);
                setSource(gfx::ShaderType::fragment, options + ShaderSources.FRAGMENT_SHADER_EMPTY);
            }

            return true;
        },
        {});

    addAttribute("feedbackVaryings",
        [&](const Values& args) {
            if (args.size() < 1)
                return false;

            auto feedbackShader = dynamic_cast<gfx::FeedbackShaderGfxImpl*>(_gfxImpl.get());
            assert(feedbackShader != nullptr);

            std::vector<std::string> varyingNames;
            for (uint32_t i = 0; i < args.size(); ++i)
                varyingNames.push_back(args[i].as<std::string>());

            feedbackShader->selectVaryings(varyingNames);

            return true;
        },
        {});
}

} // namespace Splash
