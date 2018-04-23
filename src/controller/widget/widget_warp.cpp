#include "./controller/widget/widget_warp.h"

#include <imgui.h>

#include "./core/scene.h"

using namespace std;

namespace Splash
{

/*************/
void GuiWarp::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto objects = getObjectsOfType("warp");
        auto warps = vector<shared_ptr<Warp>>();
        for (auto& object : objects)
            warps.push_back(dynamic_pointer_cast<Warp>(object));

        _currentWarpName = warps[_currentWarp]->getName();

        double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;

        ImGui::BeginChild("Warps", ImVec2(ImGui::GetWindowWidth() * 0.25, ImGui::GetWindowWidth() * 0.67), true);
        ImGui::Text("Select a warp:");
        for (uint32_t i = 0; i < warps.size(); ++i)
        {
            auto& warp = warps[i];

            // We need to update the underlying camera
            auto linkedObj = warp->getLinkedObjects();
            for (auto& obj : linkedObj)
                if (obj->getType() == "camera")
                    dynamic_pointer_cast<Camera>(obj)->render();

            if (_currentWarp == i)
                setObjectAttribute(warp->getName(), "showControlLattice", {1});
            else
                setObjectAttribute(warp->getName(), "showControlLattice", {0});

            warp->render();

            auto warpSpec = warp->getSpec();
            if (warpSpec.width == 0 || warpSpec.height == 0)
                continue;

            int w = ImGui::GetWindowWidth() - 4 * leftMargin;
            int h = w * warpSpec.height / warpSpec.width;

            if (ImGui::ImageButton((void*)(intptr_t)warp->getTexture()->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0)))
                _currentWarp = i;

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", warp->getAlias().c_str());
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Configure warp", ImVec2(0, ImGui::GetWindowWidth() * 0.67), false);
        if (_currentWarp < warps.size())
        {
            auto& warp = warps[_currentWarp];

            Values values;
            ImGui::PushID(warp->getName().c_str());

            warp->getAttribute("patchResolution", values);
            if (ImGui::InputInt("patchResolution", (int*)values[0].data(), 1, 32, ImGuiInputTextFlags_EnterReturnsTrue))
                setObjectAttribute(warp->getName(), "patchResolution", {values[0].as<int>()});

            warp->getAttribute("patchSize", values);
            vector<int> tmp;
            tmp.push_back(values[0].as<int>());
            tmp.push_back(values[1].as<int>());

            if (ImGui::InputInt2("patchSize", tmp.data(), ImGuiInputTextFlags_EnterReturnsTrue))
                setObjectAttribute(warp->getName(), "patchSize", {tmp[0], tmp[1]});

            if (auto texture = warp->getTexture())
            {
                auto warpSpec = warp->getSpec();
                int w = ImGui::GetWindowWidth() - 2 * leftMargin;
                int h = w * warpSpec.height / warpSpec.width;

                ImGui::Image((void*)(intptr_t)texture->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

                if (ImGui::IsItemHoveredRect())
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
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    else
    {
        if (_currentWarpName != "")
        {
            setObjectAttribute(_currentWarpName, "showControlLattice", {0});
            _currentWarpName = "";
        }
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
void GuiWarp::processKeyEvents(const shared_ptr<Warp>& warp)
{
    ImGuiIO& io = ImGui::GetIO();
    // Tabulation key
    if (io.KeysDown[258] && io.KeysDownDuration[258] == 0.0)
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

        if (io.KeysDown[262])
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[0] = point[0].as<float>() + delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }
        if (io.KeysDown[263])
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[0] = point[0].as<float>() - delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }
        if (io.KeysDown[264])
        {
            Values point = controlPoints[_currentControlPointIndex + 2].as<Values>();
            point[1] = point[1].as<float>() - delta;
            controlPoints[_currentControlPointIndex + 2] = point;
            setObjectAttribute(warp->getName(), "patchControl", controlPoints);
        }
        if (io.KeysDown[265])
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
void GuiWarp::processMouseEvents(const shared_ptr<Warp>& warp, int warpWidth, int warpHeight)
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

} // end of namespace
