#include "./widget_textures_view.h"

#include <imgui.h>

#include "./camera.h"
#include "./scene.h"
#include "./texture.h"

using namespace std;

namespace Splash
{

/*************/
void GuiTexturesView::render()
{
    ImGuiIO& io = ImGui::GetIO();

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

            int w = ImGui::GetWindowWidth() / 2;
            int h = w * size[1].as<int>() / size[0].as<int>();

            auto camera = dynamic_pointer_cast<Camera>(cameraAsObj);

            ImGui::BeginChild(camera->getName().c_str(), ImVec2(w, h), false);
            ImGui::Text(camera->getName().c_str());

            w = ImGui::GetWindowWidth() - 4 * leftMargin;
            h = w * size[1].as<int>() / size[0].as<int>();
            if (camera)
                ImGui::Image((void*)(intptr_t)camera->getTexture()->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(camera->getName().c_str());
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

            int w = ImGui::GetWindowWidth() / 2;
            int h = w * size[1].as<int>() / size[0].as<int>();

            ImGui::BeginChild(object->getName().c_str(), ImVec2(w, h), false);
            ImGui::Text(object->getName().c_str());

            w = ImGui::GetWindowWidth() - 4 * leftMargin;
            h = w * size[1].as<int>() / size[0].as<int>();
            ImGui::Image((void*)(intptr_t)object->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(object->getName().c_str());
            ImGui::EndChild();

            if (odd)
                ImGui::SameLine();
            odd = !odd;
        }
    }
}

} // end of namespace
