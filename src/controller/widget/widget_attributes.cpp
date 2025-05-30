#include <imgui.h>

#include "./core/scene.h"
#include "./widget_attributes.h"

namespace Splash
{

/*************/
void GuiAttributes::render()
{
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##objectParameters", availableSize, ImGuiChildFlags_Borders);

    if (checkObjectExists(_targetObjectName))
    {
        ImGui::Text("Object name: %s", _targetObjectName.c_str());
        ImGui::Text("Type: %s", getObjectTypes()[_targetObjectName].c_str());

        ImGui::BeginChild("##parameters", ImVec2(0, -26), ImGuiChildFlags_Borders);
        auto attributes = getObjectAttributes(_targetObjectName);
        drawAttributes(_targetObjectName, attributes);
        ImGui::EndChild();
    }

    ImGui::EndChild();
}

}
