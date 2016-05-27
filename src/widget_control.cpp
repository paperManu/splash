#include "./widget_text_box.h"

#include <imgui.h>

#include "./scene.h"

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
        static auto worldFramerate = 60;
        if (ImGui::InputInt("World framerate", &worldFramerate, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            worldFramerate = std::max(worldFramerate, 0);
            auto scene = _scene.lock();
            scene->sendMessageToWorld("framerate", {worldFramerate});
        }
        static auto syncTestFrameDelay = 0;
        if (ImGui::InputInt("Frames between color swap", &syncTestFrameDelay, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            syncTestFrameDelay = std::max(syncTestFrameDelay, 0);
            auto scene = _scene.lock();
            scene->sendMessageToWorld("swapTest", {syncTestFrameDelay});
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Node view
        if (!_nodeView)
        {
            auto nodeView = make_shared<GuiNodeView>("Nodes");
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
            sendValuesToObjectsOfType("camera", "blendWidth", {blendWidth});

        static auto blendPrecision = 0.1f;
        if (ImGui::InputFloat("Blending precision", &blendPrecision, 0.01f, 0.04f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            sendValuesToObjectsOfType("camera", "blendPrecision", {blendPrecision});

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Node configuration
        ImGui::Text("Objects configuration");
        // Select the object the control
        {
            vector<string> objectNames = getObjectNames();
            vector<const char*> items;

            int index = 0;
            string clickedNode = dynamic_pointer_cast<GuiNodeView>(_nodeView)->getClickedNode(); // Used to set the object selected for configuration
            for (auto& name : objectNames)
            {
                items.push_back(name.c_str());
                // If the object name is the same as the item selected in the node view, we change the targetIndex
                if (name == clickedNode)
                    _targetIndex = index;
                index++;
            }
            ImGui::Combo("Selected object", &_targetIndex, items.data(), items.size());
        }

        // Initialize the target
        if (_targetIndex >= 0)
        {
            vector<string> objectNames = getObjectNames();
            if (objectNames.size() <= _targetIndex)
                return;
            _targetObjectName = objectNames[_targetIndex];
        }

        if (_targetObjectName == "")
            return;

        auto scene = _scene.lock();

        bool isDistant = false;
        if (scene->_ghostObjects.find(_targetObjectName) != scene->_ghostObjects.end())
            isDistant = true;

        unordered_map<string, Values> attributes;
        if (!isDistant)
            attributes = scene->_objects[_targetObjectName]->getAttributes(true);
        else
            attributes = scene->_ghostObjects[_targetObjectName]->getAttributes(true);

        drawAttributes(_targetObjectName, attributes);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Delete selected object"))
            scene->sendMessageToWorld("deleteObject", {_targetObjectName});
    }
}

/*************/
vector<string> GuiControl::getObjectNames()
{
    auto scene = _scene.lock();
    vector<string> objNames;

    for (auto& o : scene->_objects)
    {
        if (!o.second->getSavable())
            continue;
        objNames.push_back(o.first);
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        objNames.push_back(o.first);
    }

    return objNames;
}

/*************/
void GuiControl::sendValuesToObjectsOfType(string type, string attr, Values values)
{
    auto scene = _scene.lock();
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == type)
            obj.second->setAttribute(attr, values);
    
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == type)
        {
            obj.second->setAttribute(attr, values);
            scene->sendMessageToWorld("sendAll", {obj.first, attr, values});
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
