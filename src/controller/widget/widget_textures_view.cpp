#include "./controller/widget/widget_textures_view.h"

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
    auto cameras = getObjectsPtr(getObjectsOfType("camera"));
    auto odd = false;
    auto leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;

    for (const auto& cameraAsObj : cameras)
    {
        if (odd)
            ImGui::SameLine();
        odd = !odd;

        cameraAsObj->render();

        Values size;
        cameraAsObj->getAttribute("size", size);

        if (size[0].as<int>() == 0)
            continue;

        auto sizeX = size[0].as<int>();
        auto sizeY = size[1].as<int>();

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
    }

    auto objects = getObjectsOfBaseType<Texture>();
    for (const auto& object : objects)
    {
        Values size;
        object->getAttribute("size", size);
        assert(size.size() == 2);

        if (size[0].as<int>() == 0)
            continue;

        if (odd)
            ImGui::SameLine();
        odd = !odd;

        auto sizeX = size[0].as<int>();
        auto sizeY = size[1].as<int>();

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
    }
}

} // namespace Splash
