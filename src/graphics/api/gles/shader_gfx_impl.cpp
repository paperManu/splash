#include "./graphics/api/gles/shader_gfx_impl.h"

namespace Splash::gfx::gles
{

/*************/
bool ShaderGfxImpl::activate()
{
    assert(_program != nullptr);

    if (!_program->isLinked() && !_program->link())
        return false;

    _program->activate();
    _isActive = true;
    return true;
}

/*************/
void ShaderGfxImpl::deactivate()
{
    assert(_program != nullptr);
    _isActive = false;
    _program->deactivate();
}

/*************/
std::map<std::string, Values> ShaderGfxImpl::getUniformValues() const
{
    assert(_program != nullptr);
    return _program->getUniformValues();
}

/*************/
std::map<std::string, std::string> ShaderGfxImpl::getUniformsDocumentation() const
{
    assert(_program != nullptr);
    return _program->getUniformsDocumentation();
}

/*************/
bool ShaderGfxImpl::setSource(gfx::ShaderType type, std::string_view source)
{
    auto shaderStageIt = _shaderStages.find(type);
    if (shaderStageIt == _shaderStages.end())
    {
        auto [it, inserted] = _shaderStages.emplace(type, type);
        if (!inserted)
            return false;
        shaderStageIt = it;
    }
    else
    {
        _program->detachShaderStage(type);
    }

    if (!shaderStageIt->second.setSource(source))
        return false;

    assert(_program != nullptr);
    _program->attachShaderStage(shaderStageIt->second);

    return true;
}

/*************/
void ShaderGfxImpl::removeShaderType(gfx::ShaderType type)
{
    _shaderStages.erase(type);
    _program->detachShaderStage(type);
}
} // namespace Splash::gfx::gles
