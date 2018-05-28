#include "./controller/widget/widget_control.h"

#include <imgui.h>

#include "./core/scene.h"
#include "./widget_node_view.h"

using namespace std;

namespace Splash
{

/*************/
void GuiControl::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        // World control
        ImGui::Text("World configuration");
        static auto looseClock = false;
        auto looseClockValue = getWorldAttribute("looseClock");
        if (!looseClockValue.empty())
            looseClock = looseClockValue[0].as<bool>();
        if (ImGui::Checkbox("Loose master clock", &looseClock))
            setWorldAttribute("looseClock", {static_cast<int>(looseClock)});
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Loose clock: if activated, the master clock is only used as an indication, not a hard constraint");

        static auto worldFramerate = 60;
        if (ImGui::InputInt("World framerate", &worldFramerate, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            worldFramerate = std::max(worldFramerate, 0);
            setWorldAttribute("framerate", {worldFramerate});
        }

        static auto syncTestFrameDelay = 0;
        if (ImGui::InputInt("Frames between color swap", &syncTestFrameDelay, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            syncTestFrameDelay = std::max(syncTestFrameDelay, 0);
            setWorldAttribute("swapTest", {syncTestFrameDelay});
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Node view
        if (!_nodeView)
        {
            auto scene = dynamic_cast<Scene*>(_root);
            auto nodeView = make_shared<GuiNodeView>(scene, "Nodes");
            nodeView->setScene(_scene);
            _nodeView = dynamic_pointer_cast<GuiWidget>(nodeView);
        }
        ImGui::Text("Configuration global view");
        _nodeView->render();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Configuration applied to multiple objects
        ImGui::Text("Global configuration");
        static auto blendWidth = 0.05f;
        if (ImGui::InputFloat("Blending width", &blendWidth, 0.01f, 0.04f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            setObjectsOfType("camera", "blendWidth", {blendWidth});

        static auto blendPrecision = 0.1f;
        if (ImGui::InputFloat("Blending precision", &blendPrecision, 0.01f, 0.04f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            setObjectsOfType("camera", "blendPrecision", {blendPrecision});

        static auto showCameraCount = false;
        if (ImGui::Checkbox("Show camera count (w/ blending)", &showCameraCount))
        {
            setWorldAttribute("computeBlending", {"continuous"});
            setObjectsOfType("camera", "showCameraCount", {showCameraCount});
        }

        static auto cameras16Bits = true;
        if (ImGui::Checkbox("Render cameras in 16bits per channel", &cameras16Bits))
            setObjectsOfType("camera", "16bits", {cameras16Bits});

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Node configuration
        ImGui::Text("Objects configuration");
        auto objectNames = getObjectNames();
        auto objectAliases = getObjectAliases();
        // Select the object the control
        vector<const char*> items;
        int index = 0;
        string clickedNode = dynamic_pointer_cast<GuiNodeView>(_nodeView)->getClickedNode(); // Used to set the object selected for configuration
        for (auto& name : objectNames)
        {
            items.push_back(objectAliases[name].c_str());
            // If the object name is the same as the item selected in the node view, we change the targetIndex
            if (name == clickedNode)
                _targetIndex = index;
            index++;
        }
        ImGui::Combo("Selected object", &_targetIndex, items.data(), items.size());

        // Initialize the target
        if (_targetIndex >= 0)
        {
            if (objectNames.size() <= static_cast<uint32_t>(_targetIndex))
                return;
            _targetObjectName = objectNames[_targetIndex];
        }

        if (_targetObjectName != "")
        {
            auto attributes = getObjectAttributes(_targetObjectName);
            drawAttributes(_targetObjectName, attributes);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            string newName = objectAliases[_targetObjectName];
            newName.resize(256);
            if (ImGui::InputText("Rename", const_cast<char*>(newName.c_str()), newName.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                setWorldAttribute("setAlias", {_targetObjectName, newName});

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Delete selected object"))
            {
                setWorldAttribute("deleteObject", {_targetObjectName});
                _targetObjectName = "";
                _targetIndex = -1;
            }
        }
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

} // end of namespace
