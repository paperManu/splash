#include "./controller/widget/widget_node_view.h"

#include "./core/factory.h"
#include "./core/scene.h"

namespace Splash
{

/*************/
GuiNodeView::GuiNodeView(Scene* scene, const std::string& name)
    : GuiWidget(scene, name)
{
    auto factory = Factory();
    _objectTypes = factory.getObjectTypes();
}

/*************/
void GuiNodeView::initializeNodePositions(const std::vector<std::string>& objectNames)
{
    // Get the link level for each object, corresponding to the level of descendants
    std::map<std::string, uint32_t> descendants;
    {
        auto links = getObjectLinks();
        auto reversedLinks = getObjectReversedLinks();

        for (const auto& objectName : objectNames)
        {
            std::vector<std::string> crossedObject;
            std::vector<std::string> currentLinks = links[objectName];

            uint32_t level = 0;
            while (!currentLinks.empty())
            {
                ++level;
                std::vector<std::string> nextLinks;
                for (const auto& link : currentLinks)
                {
                    if (std::find(objectNames.cbegin(), objectNames.cend(), link) == objectNames.cend())
                        continue;

                    if (std::find(crossedObject.cbegin(), crossedObject.cend(), link) != crossedObject.cend())
                        continue;

                    crossedObject.push_back(link);
                    std::transform(links[link].cbegin(), links[link].cend(), std::back_inserter(nextLinks), [](const auto& name) { return name; });
                }
                std::swap(currentLinks, nextLinks);
            }

            descendants[objectName] = level;

            // We leave non-linked objects at the root level, by themselves
            if (!links[objectName].empty() || !reversedLinks[objectName].empty())
                descendants[objectName] += 1;
        }
    }

    std::map<int, int> countPerLevel;
    int maxCountPerLevel = 0;
    for (const auto& object : descendants)
    {
        auto count = countPerLevel.find(object.second);
        if (count == countPerLevel.end())
        {
            countPerLevel[object.second] = 0;
            count = countPerLevel.find(object.second);
        }
        count->second++;
        maxCountPerLevel = std::max(maxCountPerLevel, count->second);
    }

    // Compute nodes position
    static ImVec2 nodeMargin{32, 32};
    std::map<int, float> positionPerLevel;
    for (const auto& objectName : objectNames)
    {
        auto level = descendants.find(objectName);
        assert(level != descendants.end());

        auto position = positionPerLevel.find(level->second);
        if (position == positionPerLevel.end())
        {
            positionPerLevel[level->second] = 0;
            position = positionPerLevel.find(level->second);
        }
        else
        {
            position->second += 1;
        }

        auto diffToMaxCount = static_cast<float>(maxCountPerLevel - countPerLevel[level->second]);

        auto xPosition = static_cast<float>(level->second) * (_nodeSize[0] + nodeMargin[0]);
        auto yPosition = (static_cast<float>(position->second) + diffToMaxCount / 2.f) * (_nodeSize[1] + nodeMargin[1]);
        _nodePositions[objectName] = std::vector<float>({xPosition, yPosition});
    }
}

/*************/
void GuiNodeView::render()
{
    auto objectLinks = getObjectLinks();
    auto objectNames = getObjectList();
    auto objectAliases = getObjectAliases();

    if (!_viewNonSavableObjects)
    {
        objectNames.erase(std::remove_if(objectNames.begin(),
                              objectNames.end(),
                              [this](const auto& name) -> bool {
                                  Values savable = getObjectAttribute(name, "savable");
                                  if (savable.empty())
                                      return true;
                                  return !savable[0].as<bool>();
                              }),
            objectNames.end());
    }

    // Combo box for adding objects
    {
        std::vector<const char*> items;
        std::transform(_objectTypes.cbegin(), _objectTypes.cend(), std::back_inserter(items), [&](auto& type) -> const char* { return type.c_str(); });
        static int itemIndex = 0;
        ImGui::Text("Add an object:");
        ImGui::SameLine();
        if (ImGui::Combo("##addObject", &itemIndex, items.data(), items.size()))
            setWorldAttribute("addObject", {_objectTypes[itemIndex]});
    }

    _clickedNode.clear();

    ImGui::Text("Click: select ; Shift + click: link ; Ctrl + click: unlink ; Middle click or Alt + right click: pan");

    // Begin a subwindow to enclose nodes
    ImGui::BeginChild("NodeView", ImVec2(0, -26), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (_firstRender)
    {
        // First render for a node: organize it given its links
        initializeNodePositions(objectNames);
        _firstRender = false;
    }
    else
    {
        // For subsequent renders: compute the position for new nodes only
        for (const auto& objectName : objectNames)
        {
            if (_nodePositions.find(objectName) != _nodePositions.end())
                continue;
            _nodePositions[objectName] = std::vector<float>({-_viewShift[0] + _graphSize[0] / 2.f - _nodeSize[0] / 2.f, -_viewShift[1] + _graphSize[1] / 2.f - _nodeSize[1] / 2.f});
        }
    }

    // Apply view shift
    auto canvasPos = ImGui::GetCursorScreenPos();
    canvasPos.x += _viewShift[0];
    canvasPos.y += _viewShift[1];

    // Draw objects according to their link level
    for (const auto& objectName : objectNames)
        renderNode(objectName);

    // Draw lines
    auto& style = ImGui::GetStyle();
    auto drawList = ImGui::GetWindowDrawList();
    for (const auto& object : objectLinks)
    {
        auto& name = object.first;
        auto& links = object.second;

        // Check that the source is displayed
        if (std::find(objectNames.begin(), objectNames.end(), name) == objectNames.end())
            continue;

        if (links.empty())
            continue;

        auto& currentNodePos = _nodePositions[name];
        auto firstPoint = ImVec2(currentNodePos[0] + canvasPos.x - style.WindowPadding[0], currentNodePos[1] + canvasPos.y - style.WindowPadding[1] + _nodeSize[1] / 2.0);

        for (const auto& target : links)
        {
            // Check that the source is displayed
            if (std::find(objectNames.begin(), objectNames.end(), target) == objectNames.end())
                continue;

            auto targetIt = _nodePositions.find(target);
            if (targetIt == _nodePositions.end())
                continue;

            auto& targetPos = targetIt->second;
            auto secondPoint = ImVec2(targetPos[0] + canvasPos.x + _nodeSize[0] - style.WindowPadding[0], targetPos[1] + canvasPos.y - style.WindowPadding[1] + _nodeSize[1] / 2.0);

            drawList->AddLine(firstPoint, secondPoint, 0xBB0088FF, 2.f);
        }
    }

    // end of the subwindow
    ImGui::EndChild();
    _graphSize = ImGui::GetItemRectSize();

    // Interactions
    if (ImGui::IsItemHovered())
        _isHovered = true;
    else
        _isHovered = false;

    // This handles the mouse capture even when the mouse goes outside the node view widget
    ImGuiIO& io = ImGui::GetIO();
    static bool viewCaptured = false;
    if (io.MouseDownDuration[2] > 0.0)
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
        _viewShift[0] += io.MouseDelta.x;
        _viewShift[1] += io.MouseDelta.y;
    }

    // Select an object by its name
    std::vector<const char*> items;
    std::transform(objectNames.begin(), objectNames.end(), std::back_inserter(items), [&](auto& name) -> const char* { return objectAliases[name].c_str(); });

    ImGui::Text("Object list:");
    ImGui::SameLine();

    if (auto sourceItem = std::find(items.begin(), items.end(), _sourceNode); sourceItem != items.end())
        _comboObjectIndex = std::distance(items.begin(), sourceItem);

    ImGui::PushItemWidth(-260.f);
    if (ImGui::Combo("##objectList", &_comboObjectIndex, items.data(), items.size()))
    {
        _sourceNode = items[_comboObjectIndex];
        _clickedNode = _sourceNode;

        // Move the focus to the selected object
        auto nodePos = _nodePositions.find(_sourceNode);
        assert(nodePos != _nodePositions.end());
        _viewShift[0] = -nodePos->second[0] + _graphSize[0] / 2.f - _nodeSize[0] / 2.f;
        _viewShift[1] = -nodePos->second[1] + _graphSize[1] / 2.f - _nodeSize[1] / 2.f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reorder graph"))
        initializeNodePositions(objectNames);
    ImGui::SameLine();
    ImGui::Checkbox("Show non-savable objects", &_viewNonSavableObjects);
}

/*************/
void GuiNodeView::renderNode(const std::string& name)
{
    auto& io = ImGui::GetIO();

    auto pos = _nodePositions.find(name);
    // If the node does not have a drawing position, do not draw it
    if (pos == _nodePositions.end())
        return;

    auto& nodePos = pos->second;
    ImGui::SetCursorPos(ImVec2(nodePos[0] + _viewShift[0], nodePos[1] + _viewShift[1]));

    // Beginning of node rendering
    bool coloredSelectedNode = false;
    if (name == _sourceNode)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        coloredSelectedNode = true;
    }

    ImGui::BeginChild(std::string("node_" + name).c_str(), ImVec2(_nodeSize[0], _nodeSize[1]), false);

    ImGui::Button(getObjectAlias(name).c_str(), ImVec2(_nodeSize[0], _nodeSize[1]));
    if (ImGui::IsItemHovered())
    {
        if (io.MouseClicked[0])
        {
            // Object linking
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
            else
            {
                _clickedNode = name;
                _sourceNode = name;
            }
        }
        else if (io.MouseDownDuration[0] > 0.0)
        {
            // Dragging
            _capturedNode = name;
        }
    }

    if (io.MouseReleased[0])
        _capturedNode.clear();

    if (_capturedNode == name)
    {
        float dx = io.MouseDelta.x;
        float dy = io.MouseDelta.y;
        auto& nodePos = (*pos).second;
        nodePos[0] += dx;
        nodePos[1] += dy;
    }

    ImGui::EndChild();

    if (coloredSelectedNode)
        ImGui::PopStyleColor(1);
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

} // namespace Splash
