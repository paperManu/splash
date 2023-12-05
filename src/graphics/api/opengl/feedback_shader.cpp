#include "./graphics/api/opengl/feedback_shader.h"

namespace Splash::gfx::opengl
{

/*************/
FeedbackShaderGfxImpl::FeedbackShaderGfxImpl()
{
    _program = std::make_unique<Program>(ProgramType::Feedback);
    _shaderStages.emplace(gfx::ShaderType::vertex, ShaderType::vertex);
    _shaderStages.emplace(gfx::ShaderType::tess_ctrl, ShaderType::tess_ctrl);
    _shaderStages.emplace(gfx::ShaderType::tess_eval, ShaderType::tess_eval);
    _shaderStages.emplace(gfx::ShaderType::geometry, ShaderType::geometry);
    _shaderStages.emplace(gfx::ShaderType::fragment, ShaderType::fragment);
}

/*************/
bool FeedbackShaderGfxImpl::activate()
{
    if (!ShaderGfxImpl::activate())
        return false;

    {
        OnOpenGLScopeExit(std::string("FeedbackShaderGfxImpl::").append(__FUNCTION__));
        glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(GL_TRIANGLES);
    }

    return true;
}

/*************/
void FeedbackShaderGfxImpl::deactivate()
{
    {
        OnOpenGLScopeExit(std::string("FeedbackShaderGfxImpl::").append(__FUNCTION__));
        glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);
    }
    ShaderGfxImpl::deactivate();
}

/*************/
void FeedbackShaderGfxImpl::selectVaryings(const std::vector<std::string>& varyingNames)
{
    _program->selectVaryings(varyingNames);
}

} // namespace Splash::gfx::opengl
