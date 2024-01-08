#include "./graphics/api/gles/compute_shader_gfx_impl.h"

#include "./graphics/api/gles/gl_utils.h"

namespace Splash::gfx::gles
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
        OnGLESScopeExit(std::string("ComputeShaderGfxImpl::").append(__FUNCTION__));
        glDispatchCompute(numGroupsX, numGroupsY, 1);
    }

    ShaderGfxImpl::deactivate();
    return true;
}

} // namespace Splash::gfx::gles
