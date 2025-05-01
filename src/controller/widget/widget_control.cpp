#include "./controller/widget/widget_control.h"

#include <imgui.h>

#include "./core/scene.h"
#include "./widget_node_view.h"

namespace Splash
{

/*************/
void GuiControl::render()
{
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##nodeView", ImVec2(availableSize.x, availableSize.y), ImGuiChildFlags_Borders);

    // Node view
    if (!_nodeView)
    {
        auto scene = dynamic_cast<Scene*>(_root);
        auto nodeView = std::make_shared<GuiNodeView>(scene, "Nodes");
        _nodeView = std::dynamic_pointer_cast<GuiWidget>(nodeView);
    }
    _nodeView->render();

    // Node configuration
    // Select the object the control
    int index = 0;
    std::string clickedNode = std::dynamic_pointer_cast<GuiNodeView>(_nodeView)->getClickedNode(); // Used to set the object
    ImGui::EndChild();

    // Get the object name
    auto objectNames = getObjectList();
    for (const auto& name : objectNames)
    {
        if (name == clickedNode)
            _targetIndex = index;
        index++;
    }

    if (_targetIndex >= 0)
    {
        if (objectNames.size() <= static_cast<uint32_t>(_targetIndex))
            return;
        _targetObjectName = objectNames[_targetIndex];
    }
}

/*************/
int GuiControl::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    if (_nodeView)
        flags |= _nodeView->updateWindowFlags();
    return flags;
}

} // namespace Splash
