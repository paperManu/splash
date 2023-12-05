#include "./graphics/api/opengl/compute_shader.h"

namespace Splash::gfx::opengl
{

/*************/
ComputeShaderGfxImpl::ComputeShaderGfxImpl()
{
    _program = std::make_unique<Program>(ProgramType::Compute);
    _shaderStages.emplace(gfx::ShaderType::compute, ShaderType::compute);
}

/*************/
bool ComputeShaderGfxImpl::compute(uint32_t numGroupsX, uint32_t numGroupsY)
{
    if (!ShaderGfxImpl::activate())
        return false;

    {
        OnOpenGLScopeExit(std::string("ComputeShaderGfxImpl::").append(__FUNCTION__));
        glDispatchCompute(numGroupsX, numGroupsY, 1);
    }

    ShaderGfxImpl::deactivate();
    return true;
}

} // namespace Splash::gfx::opengl
