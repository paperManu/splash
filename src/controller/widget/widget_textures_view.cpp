#include "./widget_textures_view.h"

#include <imgui.h>

#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/texture.h"

using namespace std;

namespace Splash
{

/*************/
void GuiTexturesView::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto cameras = getObjectsOfType("camera");
        auto odd = true;
        double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;

        for (auto& cameraAsObj : cameras)
        {
            cameraAsObj->render();

            Values size;
            cameraAsObj->getAttribute("size", size);

            if (size[0].as<int>() == 0)
                continue;

            int sizeX = size[0].as<int>();
            int sizeY = size[1].as<int>();

            int w = ImGui::GetWindowWidth() / 2;
            int h = sizeX != 0 ? w * sizeY / sizeX : 1;

            auto camera = dynamic_pointer_cast<Camera>(cameraAsObj);

            ImGui::BeginChild(camera->getName().c_str(), ImVec2(w, h), false);
            ImGui::Text("%s", camera->getName().c_str());

            w = ImGui::GetWindowWidth() - 4 * leftMargin;
            h = sizeX != 0 ? w * sizeY / sizeX : 1;
            if (camera)
                ImGui::Image((void*)(intptr_t)camera->getTexture()->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", camera->getName().c_str());
            ImGui::EndChild();

            if (odd)
                ImGui::SameLine();
            odd = !odd;
        }

        auto objects = getObjectsOfBaseType<Texture>();
        for (auto& object : objects)
        {
            Values size;
            object->getAttribute("size", size);

            if (size[0].as<int>() == 0)
                continue;

            int sizeX = size[0].as<int>();
            int sizeY = size[1].as<int>();

            int w = ImGui::GetWindowWidth() / 2;
            int h = sizeX != 0 ? w * sizeY / sizeX : 1;

            ImGui::BeginChild(object->getName().c_str(), ImVec2(w, h), false);
            ImGui::Text("%s", object->getName().c_str());

            w = ImGui::GetWindowWidth() - 4 * leftMargin;
            h = sizeX != 0 ? w * sizeY / sizeX : 1;
            ImGui::Image((void*)(intptr_t)object->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", object->getName().c_str());
            ImGui::EndChild();

            if (odd)
                ImGui::SameLine();
            odd = !odd;
        }
    }
}

} // end of namespace
