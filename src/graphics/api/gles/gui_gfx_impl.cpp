#include "./graphics/api/gles/gui_gfx_impl.h"

#include "./utils/log.h"

namespace Splash::gfx::gles
{

/*************/
GuiGfxImpl::~GuiGfxImpl()
{
    glDeleteTextures(1, &_imFontTextureId);
    glDeleteProgram(_imGuiShaderHandle);
    glDeleteBuffers(1, &_imGuiVboHandle);
    glDeleteBuffers(1, &_imGuiElementsHandle);
    glDeleteVertexArrays(1, &_imGuiVaoHandle);
}

/*************/
void GuiGfxImpl::initRendering()
{
    // Initialize GL stuff for ImGui
    static const std::string_view vertexShader{R"(
            #version 320 es
            precision mediump float;

            uniform mat4 ProjMtx;
            in vec2 Position;
            in vec2 UV;
            in vec4 Color;
            out vec2 Frag_UV;
            out vec4 Frag_Color;

            void main()
            {
                Frag_UV = UV;
                Frag_Color = Color;
                gl_Position = ProjMtx * vec4(Position.xy, 0.f, 1.f);
            }
        )"};

    static const std::string_view fragmentShader{R"(
            #version 320 es
            precision mediump float;

            uniform sampler2D Texture;
            in vec2 Frag_UV;
            in vec4 Frag_Color;
            out vec4 Out_Color;

            void main()
            {
                Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
            }
        )"};

    _imGuiShaderHandle = glCreateProgram();
    _imGuiVertHandle = glCreateShader(GL_VERTEX_SHADER);
    _imGuiFragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    {
        const GLchar* shaderSrc = vertexShader.data();
        glShaderSource(_imGuiVertHandle, 1, &shaderSrc, 0);
    }
    {
        const GLchar* shaderSrc = fragmentShader.data();
        glShaderSource(_imGuiFragHandle, 1, &shaderSrc, 0);
    }
    glCompileShader(_imGuiVertHandle);
    glCompileShader(_imGuiFragHandle);
    glAttachShader(_imGuiShaderHandle, _imGuiVertHandle);
    glAttachShader(_imGuiShaderHandle, _imGuiFragHandle);
    glLinkProgram(_imGuiShaderHandle);

    GLint status;
    glGetProgramiv(_imGuiShaderHandle, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        Log::get() << Log::WARNING << "GuiGfxImpl::" << __FUNCTION__ << " - Error while linking the shader program" << Log::endl;
        return;
    }

    _imGuiTextureLocation = glGetUniformLocation(_imGuiShaderHandle, "Texture");
    _imGuiProjMatrixLocation = glGetUniformLocation(_imGuiShaderHandle, "ProjMtx");
    _imGuiPositionLocation = glGetAttribLocation(_imGuiShaderHandle, "Position");
    _imGuiUVLocation = glGetAttribLocation(_imGuiShaderHandle, "UV");
    _imGuiColorLocation = glGetAttribLocation(_imGuiShaderHandle, "Color");

    glGenBuffers(1, &_imGuiVboHandle);
    glGenBuffers(1, &_imGuiElementsHandle);

    glGenVertexArrays(1, &_imGuiVaoHandle);
    glBindVertexArray(_imGuiVaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
    glEnableVertexAttribArray(_imGuiPositionLocation);
    glEnableVertexAttribArray(_imGuiUVLocation);
    glEnableVertexAttribArray(_imGuiColorLocation);

    glVertexAttribPointer(_imGuiPositionLocation, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)(size_t) & (((ImDrawVert*)0)->pos));
    glVertexAttribPointer(_imGuiUVLocation, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)(size_t) & (((ImDrawVert*)0)->uv));
    glVertexAttribPointer(_imGuiColorLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)(size_t) & (((ImDrawVert*)0)->col));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*************/
ImTextureID GuiGfxImpl::initFontTexture(uint32_t width, uint32_t height, uint8_t* pixels)
{
    glDeleteTextures(1, &_imFontTextureId);
    glGenTextures(1, &_imFontTextureId);
    glBindTexture(GL_TEXTURE_2D, _imFontTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return (ImTextureID)(intptr_t)(_imFontTextureId);
}

/*************/
void GuiGfxImpl::setupViewport(uint32_t width, uint32_t height)
{
    glViewport(0, 0, width, height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

/*************/
void GuiGfxImpl::drawGui(float width, float height, ImDrawData* drawData)
{
    if (!drawData->CmdListsCount)
        return;

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    const float orthoProjection[4][4] = {
        {2.0f / width, 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / -height, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };

    glUseProgram(_imGuiShaderHandle);
    glUniform1i(_imGuiTextureLocation, 0);
    glUniformMatrix4fv(_imGuiProjMatrixLocation, 1, GL_FALSE, &orthoProjection[0][0]);
    glBindVertexArray(_imGuiVaoHandle);

    for (int n = 0; n < drawData->CmdListsCount; ++n)
    {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        const ImDrawIdx* idxBufferOffset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, _imGuiVboHandle);
        glBufferData(
            GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmdList->VtxBuffer.size()) * sizeof(ImDrawVert), static_cast<const GLvoid*>(&cmdList->VtxBuffer.front()), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _imGuiElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(cmdList->IdxBuffer.size()) * sizeof(ImDrawIdx),
            static_cast<const GLvoid*>(&cmdList->IdxBuffer.front()),
            GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmdList->CmdBuffer.begin(); pcmd != cmdList->CmdBuffer.end(); ++pcmd)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmdList, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x,
                    static_cast<int>(height - pcmd->ClipRect.w),
                    static_cast<int>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                    static_cast<int>(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount), sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idxBufferOffset);
            }
            idxBufferOffset += pcmd->ElemCount;
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

} // namespace Splash::gfx::gles
