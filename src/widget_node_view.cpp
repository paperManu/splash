#include "./widget_node_view.h"

#include <imgui.h>

#include "./factory.h"
#include "./scene.h"

using namespace std;

namespace Splash
{

/*************/
GuiNodeView::GuiNodeView(string name)
    : GuiWidget(name)
{
    auto factory = Factory();
    _objectTypes = factory.getObjectTypes();
}

/*************/
map<string, vector<string>> GuiNodeView::getObjectLinks()
{
    auto scene = _scene.lock();

    auto links = map<string, vector<string>>();

    for (auto& o : scene->_objects)
    {
        if (!o.second->getSavable())
            continue;
        links[o.first] = vector<string>();
        auto linkedObjects = o.second->getLinkedObjects();
        for (auto& link : linkedObjects)
            links[o.first].push_back(link->getName());
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        links[o.first] = vector<string>();
        auto linkedObjects = o.second->getLinkedObjects();
        for (auto& link : linkedObjects)
            links[o.first].push_back(link->getName());
    }

    return links;
}

/*************/
map<string, string> GuiNodeView::getObjectTypes()
{
    auto scene = _scene.lock();

    auto types = map<string, string>();

    for (auto& o : scene->_objects)
    {
        if (!o.second->getSavable())
            continue;
        types[o.first] = o.second->getType();
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->getSavable())
            continue;
        types[o.first] = o.second->getType();
    }

    return types;
}

/*************/
void GuiNodeView::render()
{
    _clickedNode = "";

    //if (ImGui::CollapsingHeader(_name.c_str()))
    if (true)
    {
        ImGui::Text("Click: select / Shift + click: link / Ctrl + click: unlink");

        // This defines the default positions for various node types
        static auto defaultPositionByType = map<string, ImVec2>({{"default", {8, 8}},
                                                                 {"window", {8, 32}},
                                                                 {"warp", {32, 64}},
                                                                 {"camera", {8, 96}},
                                                                 {"object", {32, 128}},
                                                                 {"texture filter queue", {8, 160}},
                                                                 {"image", {32, 192}},
                                                                 {"mesh", {8, 224}}
                                                                });
        std::map<std::string, int> shiftByType;

        // Begin a subwindow to enclose nodes
        ImGui::BeginChild("NodeView", ImVec2(_viewSize[0], _viewSize[1]), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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
            auto shiftIt = shiftByType.find(type);
            if (shiftIt == shiftByType.end())
                shiftByType[type] = 0;
            auto shift = shiftByType[type];

            // Get the default position for the given type
            auto nodePosition = ImVec2(-1, -1);

            string defaultPosName = "default";
            for (auto& position : defaultPositionByType)
            {
                if (position.first.find(type) != string::npos)
                    defaultPosName = position.first;
            }

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

            shiftByType[type] += _nodeSize[0] + 8;
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
        {
            _isHovered = true;

            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseDownDuration[0] > 0.0)
            {
                float dx = io.MouseDelta.x;
                float dy = io.MouseDelta.y;
                _viewShift[0] += dx;
                _viewShift[1] += dy;
            }
        }
        else
            _isHovered = false;
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
        {
            _scene.lock()->sendMessageToWorld("addObject", {_objectTypes[itemIndex]});
        }
        ImGui::PopID();
    }
}

/*************/
void GuiNodeView::renderNode(string name)
{
    auto& io = ImGui::GetIO();

    auto pos = _nodePositions.find(name);
    if (pos == _nodePositions.end())
    {
        auto cursorPos = ImGui::GetCursorPos();
        _nodePositions[name] = vector<float>({cursorPos.x + _viewShift[0], cursorPos.y + _viewShift[1]});
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
        ImGui::PushStyleColor(ImGuiCol_Header, ImColor(0.2f, 0.2f, 0.2f, 0.99f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImColor(0.2f, 0.2f, 0.2f, 0.99f));
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
                auto scene = _scene.lock();
                scene->sendMessageToWorld("sendAllScenes", {"link", _sourceNode, name});
                scene->sendMessageToWorld("sendAllScenes", {"linkGhost", _sourceNode, name});
            }
            else if (io.KeyCtrl)
            {
                auto scene = _scene.lock();
                scene->sendMessageToWorld("sendAllScenes", {"unlink", _sourceNode, name});
                scene->sendMessageToWorld("sendAllScenes", {"unlinklinkGhost", _sourceNode, name});
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
    ImGui::SetNextTreeNodeOpened(false);
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
