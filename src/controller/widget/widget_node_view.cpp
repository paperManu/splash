#include "./widget_node_view.h"

#include <imgui.h>

#include "./core/factory.h"
#include "./core/scene.h"

using namespace std;

namespace Splash
{

/*************/
GuiNodeView::GuiNodeView(Scene* scene, const string& name)
    : GuiWidget(scene, name)
{
    auto factory = Factory();
    _objectTypes = factory.getObjectTypes();
}

/*************/
void GuiNodeView::render()
{
    _clickedNode = "";

    // if (ImGui::CollapsingHeader(_name.c_str()))
    if (true)
    {
        ImGui::Text("Click: select / Shift + click: link / Ctrl + click: unlink");

        // This defines the default positions for various node types
        static auto defaultPositionByType = map<string, ImVec2>({{"default", {8, 8}},
            {"window sink", {8, 48}},
            {"warp", {32, 88}},
            {"camera", {8, 128}},
            {"object", {32, 168}},
            {"texture filter queue virtual_screen", {8, 208}},
            {"image", {32, 248}},
            {"mesh", {8, 288}},
            {"python", {32, 328}}});
        std::map<std::string, int> shiftByType;

        // Begin a subwindow to enclose nodes
        ImGui::BeginChild("NodeView", ImVec2(0, _viewSize[1]), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Get objects and their relations
        auto objectLinks = getObjectLinks();
        auto objectTypes = getObjectTypes();

        // Objects useful for drawing
        auto drawList = ImGui::GetWindowDrawList();
        auto canvasPos = ImGui::GetCursorScreenPos();
        // Apply view shift
        canvasPos.x += _viewShift[0];
        canvasPos.y += _viewShift[1];

        // Draw objects
        for (auto& object : objectTypes)
        {
            auto& name = object.first;
            auto& type = object.second;
            type = type.substr(0, type.find("_"));

            // We keep count of the number of objects by type, more precisely their shifting
            // Get the default position for the given type
            auto nodePosition = ImVec2(-1, -1);

            string defaultPosName = "default";
            for (auto& position : defaultPositionByType)
            {
                if (position.first.find(type) != string::npos)
                    defaultPosName = position.first;
            }

            auto shiftIt = shiftByType.find(defaultPosName);
            if (shiftIt == shiftByType.end())
                shiftByType[defaultPosName] = 0;
            auto shift = shiftByType[defaultPosName];

            if (defaultPosName == "default")
            {
                type = "default";
                nodePosition = defaultPositionByType["default"];
                nodePosition.x += shift;
            }
            else
            {
                nodePosition = defaultPositionByType[defaultPosName];
                nodePosition.x += shift;
            }

            ImGui::SetCursorPos(nodePosition);
            renderNode(name);

            shiftByType[defaultPosName] += _nodeSize[0] + 8;
        }

        // Draw lines
        for (auto& object : objectLinks)
        {
            auto& name = object.first;
            auto& links = object.second;

            auto& currentNodePos = _nodePositions[name];
            auto firstPoint = ImVec2(currentNodePos[0] + canvasPos.x, currentNodePos[1] + canvasPos.y);

            for (auto& target : links)
            {
                auto targetIt = _nodePositions.find(target);
                if (targetIt == _nodePositions.end())
                    continue;

                auto& targetPos = targetIt->second;
                auto secondPoint = ImVec2(targetPos[0] + canvasPos.x, targetPos[1] + canvasPos.y);

                drawList->AddLine(firstPoint, secondPoint, 0xBB0088FF, 2.f);
            }
        }

        // end of the subwindow
        ImGui::EndChild();

        // Interactions
        if (ImGui::IsItemHovered())
            _isHovered = true;
        else
            _isHovered = false;

        // This handles the mouse capture even when the mouse goes outside the node view widget
        ImGuiIO& io = ImGui::GetIO();
        static bool viewCaptured = false;
        if (io.MouseDownDuration[1] > 0.0)
        {
            if (ImGui::IsItemHovered())
                viewCaptured = true;
        }
        else if (viewCaptured)
        {
            viewCaptured = false;
        }

        if (viewCaptured)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;
            _viewShift[0] += dx;
            _viewShift[1] += dy;
        }
    }

    // Combo box for adding objects
    {
        vector<const char*> items;
        for (auto& typeName : _objectTypes)
            items.push_back(typeName.c_str());
        static int itemIndex = 0;
        ImGui::Text("Select a type to add an object:");
        ImGui::PushID("addObject");
        if (ImGui::Combo("", &itemIndex, items.data(), items.size()))
            setWorldAttribute("addObject", {_objectTypes[itemIndex]});
        ImGui::PopID();
    }
}

/*************/
void GuiNodeView::renderNode(const string& name)
{
    auto& io = ImGui::GetIO();

    auto pos = _nodePositions.find(name);
    if (pos == _nodePositions.end())
    {
        auto cursorPos = ImGui::GetCursorPos();
        _nodePositions[name] = vector<float>({cursorPos.x + _viewShift[0], cursorPos.y + _viewShift[1]});
        pos = _nodePositions.find(name);
    }
    else
    {
        auto& nodePos = (*pos).second;
        ImGui::SetCursorPos(ImVec2(nodePos[0] + _viewShift[0], nodePos[1] + _viewShift[1]));
    }

    // Beginning of node rendering
    bool coloredSelectedNode = false;
    if (name == _sourceNode)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.99f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.99f));
        coloredSelectedNode = true;
    }

    ImGui::BeginChild(string("node_" + name).c_str(), ImVec2(_nodeSize[0], _nodeSize[1]), false);

    ImGui::SetCursorPos(ImVec2(0, 2));

    if (ImGui::IsItemHovered())
    {
        // Object linking
        if (io.MouseClicked[0])
        {
            if (io.KeyShift)
            {
                if (dynamic_cast<Scene*>(_root))
                    setWorldAttribute("link", {_sourceNode, name});
            }
            else if (io.KeyCtrl)
            {
                if (dynamic_cast<Scene*>(_root))
                    setWorldAttribute("unlink", {_sourceNode, name});
            }
        }

        // Dragging
        if (io.MouseDownDuration[0] > 0.0)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;
            auto& nodePos = (*pos).second;
            nodePos[0] += dx;
            nodePos[1] += dy;
        }
    }

    // This tricks allows for detecting clicks on the node, while keeping it closed
    // TODO: investigate whther this is what closes the next panel unexpectedly
    ImGui::SetNextTreeNodeOpen(false);
    if (ImGui::CollapsingHeader(name.c_str()))
    {
        _clickedNode = name;
        _sourceNode = name;
    }

    // End of node rendering
    ImGui::EndChild();

    if (coloredSelectedNode)
        ImGui::PopStyleColor(2);
}

/*************/
int GuiNodeView::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    if (_isHovered)
    {
        flags |= ImGuiWindowFlags_NoMove;
    }
    return flags;
}

} // end of namespace
