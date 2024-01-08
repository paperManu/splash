#include "./graphics/api/opengl/graphic_shader_gfx_impl.h"

namespace Splash::gfx::opengl
{
/*************/
GraphicShaderGfxImpl::GraphicShaderGfxImpl()
{
    _program = std::make_unique<Program>(ProgramType::Graphic);
    _shaderStages.emplace(gfx::ShaderType::vertex, ShaderType::vertex);
    _shaderStages.emplace(gfx::ShaderType::geometry, ShaderType::geometry);
    _shaderStages.emplace(gfx::ShaderType::fragment, ShaderType::fragment);
}

/*************/
bool GraphicShaderGfxImpl::activate()
{
    if (!ShaderGfxImpl::activate())
        return false;

    {
        OnOpenGLScopeExit(std::string("GraphicShaderGfxImpl::").append(__FUNCTION__));

        switch (_culling)
        {
        default:
            glDisable(GL_CULL_FACE);
            break;
        case Culling::singleSided:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case Culling::inverted:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        }
    }

    return true;
}

/*************/
void GraphicShaderGfxImpl::deactivate()
{
    {
        OnOpenGLScopeExit(std::string("GraphicShaderGfxImpl::").append(__FUNCTION__));
        if (_culling != Culling::doubleSided)
            glDisable(GL_CULL_FACE);
    }

    ShaderGfxImpl::deactivate();
}

/*************/
void GraphicShaderGfxImpl::setModelViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp)
{
    glm::mat4 floatMv = (glm::mat4)mv;
    glm::mat4 floatMp = (glm::mat4)mp;
    glm::mat4 floatMvp = (glm::mat4)(mp * mv);

    _program->setUniform("_modelViewProjectionMatrix", floatMvp);
    _program->setUniform("_modelViewMatrix", floatMv);
    _program->setUniform("_projectionMatrix", floatMp);
    _program->setUniform("_normalMatrix", glm::transpose(glm::inverse(floatMv)));
}

/*************/
bool GraphicShaderGfxImpl::setTexture(Texture* texture, const GLuint textureUnit, std::string_view name)
{
    assert(_program != nullptr);
    const auto& uniforms = _program->getUniforms();
    const auto uniformIt = uniforms.find(std::string(name));

    if (uniformIt == uniforms.end())
        return false;

    auto& uniform = uniformIt->second;
    if (uniform.glIndex == -1)
        return false;

    {
        OnOpenGLScopeExit(std::string("GraphicShaderGfxImpl::").append(__FUNCTION__));
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        texture->bind();
        glUniform1i(uniform.glIndex, static_cast<GLint>(textureUnit));
    }

    _textures.push_back(texture);
    return _program->setUniform("_textureNbr", Values({Value(static_cast<int>(_textures.size()))}));
}

/*************/
void GraphicShaderGfxImpl::unsetTextures()
{
    for (auto texture : _textures)
        texture->unbind();
    _textures.clear();
}

} // namespace Splash::gfx::opengl
