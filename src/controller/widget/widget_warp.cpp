#include "./controller/widget/widget_warp.h"

#include <imgui.h>

#include "./core/scene.h"

namespace Splash
{

/*************/
void GuiWarp::render()
{
    auto objects = getObjectsPtr(getObjectsOfType("warp"));
    auto warps = std::vector<std::shared_ptr<Warp>>();

    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(warps), [](const auto& warp) { return std::dynamic_pointer_cast<Warp>(warp); });
    _currentWarpName = _currentWarp < warps.size() ? warps[_currentWarp]->getName() : "";

    ImVec2 availableSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("Warps", ImVec2(ImGui::GetWindowWidth() * 0.25, availableSize.y), true);
    ImGui::Text("Warp list");

    auto leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
    for (uint32_t i = 0; i < warps.size(); ++i)
    {
        auto& warp = warps[i];

        // We need to update the underlying camera
        auto linkedObj = warp->getLinkedObjects();
        for (auto& weakLinkedObject : linkedObj)
        {
            auto linkedObject = weakLinkedObject.lock();
            if (!linkedObject)
                continue;

            if (linkedObject->getType() == "camera")
                std::dynamic_pointer_cast<Camera>(linkedObject)->render();
        }

        if (_currentWarp == i)
            setObjectAttribute(warp->getName(), "showControlLattice", {true});
        else
            setObjectAttribute(warp->getName(), "showControlLattice", {false});

        warp->render();

        const auto warpSpec = warp->getSpec();
        if (warpSpec.width == 0 || warpSpec.height == 0)
            continue;

        int w = ImGui::GetWindowWidth() - 4 * leftMargin;
        int h = w * warpSpec.height / warpSpec.width;

        if (ImGui::ImageButton((void*)(intptr_t)warp->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0)))
            _currentWarp = i;

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", warp->getAlias().c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Configure warp", ImVec2(0, 0), true);
    if (_currentWarp < warps.size())
    {
        auto& warp = warps[_currentWarp];

        Values values;
        ImGui::PushID(warp->getName().c_str());

        auto availableSize = ImGui::GetContentRegionAvail();
        if (ImGui::Button("Reset warp", ImVec2(availableSize[0], 24.f)))
            setObjectAttribute(warp->getName(), "resetPatch");

        warp->getAttribute("patchResolution", values);
        if (ImGui::InputInt("patchResolution", static_cast<int*>(values[0].data()), 1, 32, ImGuiInputTextFlags_EnterReturnsTrue))
            setObjectAttribute(warp->getName(), "patchResolution", {values[0].as<int>()});

        warp->getAttribute("patchSize", values);
        std::vector<int> tmp;
        tmp.push_back(values[0].as<int>());
        tmp.push_back(values[1].as<int>());

        if (ImGui::InputInt2("patchSize", tmp.data(), ImGuiInputTextFlags_EnterReturnsTrue))
            setObjectAttribute(warp->getName(), "patchSize", {tmp[0], tmp[1]});

        if (const auto texture = warp->getTexture())
        {
            const auto warpSpec = warp->getSpec();
            if (warpSpec.width != 0 && warpSpec.height != 0)
            {
                int w = ImGui::GetWindowWidth() - 2 * leftMargin;
                int h = w * warpSpec.height / warpSpec.width;

                availableSize = ImGui::GetContentRegionAvail();
                if (h > availableSize.y)
                {
                    h = availableSize.y;
                    w = h * warpSpec.width / warpSpec.height;
                }

                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture->getTexId())), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
                {
                    _noMove = true;
                    processKeyEvents(warp);
                    processMouseEvents(warp, w, h);
                }
                else
                {
                    _noMove = false;
                }
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    _rendered = true;
}

/*************/
void GuiWarp::update()
{
    if (_rendered)
    {
        _rendered = false;
        return;
    }

    if (_currentWarpName != "")
    {
        setObjectAttribute(_currentWarpName, "showControlLattice", {false});
        _currentWarpName = "";
    }
}

/*************/
int GuiWarp::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    if (_noMove)
    {
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoScrollWithMouse;
    }
    return flags;
}

/*************/
void GuiWarp::processKeyEvents(const std::shared_ptr<Warp>& warp)
{
    ImGuiIO& io = ImGui::GetIO();
    // Tabulation key
    if (ImGui::IsKeyPressed(ImGuiKey_Tab, false))
    {
        Values controlPoints;
        warp->getAttribute("patchControl", controlPoints);

        if (io.KeyShift)
        {
            _currentControlPointIndex--;
            if (_currentControlPointIndex < 0)
                _currentControlPointIndex = controlPoints.size() - 3;
        }
        else
        {
            _currentControlPointIndex++;
            if (static_cast<uint32_t>(_currentControlPointIndex) + 2 >= controlPoints.size())
                _currentControlPointIndex = 0;
        }

        setObjectAttribute(warp->getName(), "showControlPoint", {_currentControlPointIndex});
    }
    // Arrow keys
    {
        Values controlPoints;
        warp->getAttribute("patchControl", controlPoints);
        if (controlPoints.size() < static_cast<uint32_t>(_currentControlPointIndex) + 2)
            return;

        float delta = 0.001f;
        if (io.KeyShift)
            delta = 0.0001f;
        else if (io.KeyCtrl)
            delta = 0.01f;

        if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[0] = point[0].as<float>() + delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[0] = point[0].as<float>() - delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }
        if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[1] = point[1].as<float>() - delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }
        if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[1] = point[1].as<float>() + delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }

        return;
    }
}

/*************/
void GuiWarp::processMouseEvents(const std::shared_ptr<Warp>& warp, int warpWidth, int warpHeight)
{
    ImGuiIO& io = ImGui::GetIO();

    // Get mouse pos
    ImVec2 mousePos = ImVec2((io.MousePos.x - ImGui::GetCursorScreenPos().x) / warpWidth, -(io.MousePos.y - ImGui::GetCursorScreenPos().y) / warpHeight);
    mousePos.x = mousePos.x * 2.0 - 1.0;
    mousePos.y = mousePos.y * 2.0 - 1.0;

    if (io.MouseDownDuration[0] == 0.0)
    {
        // Select a control point
        glm::vec2 picked;
        _currentControlPointIndex = warp->pickControlPoint(glm::vec2(mousePos.x, mousePos.y), picked);
        setObjectAttribute(warp->getName(), "showControlPoint", {_currentControlPointIndex});

        // Get the distance between the mouse and the point
        Values controlPoints;
        warp->getAttribute("patchControl", controlPoints);
        if (controlPoints.size() < static_cast<uint32_t>(_currentControlPointIndex))
            return;
        Value point = controlPoints[_currentControlPointIndex + 2].as<Values>();
        _deltaAtPicking = glm::vec2(point[0].as<float>() - mousePos.x, point[1].as<float>() - mousePos.y);
    }
    else if (io.MouseDownDuration[0] > 0.0)
    {
        Values controlPoints;
        warp->getAttribute("patchControl", controlPoints);
        if (controlPoints.size() < static_cast<uint32_t>(_currentControlPointIndex))
            return;

        Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
        point[0] = mousePos.x + _deltaAtPicking.x;
        point[1] = mousePos.y + _deltaAtPicking.y;
        controlPoints[_currentControlPointIndex + 2] = point;

        setObjectAttribute(warp->getName(), "patchControl", controlPoints);
    }
}

} // namespace Splash
