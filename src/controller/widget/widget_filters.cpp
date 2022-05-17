#include "./controller/widget/widget_filters.h"

#include <algorithm>

#include <imgui.h>

#include "./graphics/filter.h"

namespace Splash
{

/*************/
void GuiFilters::render()
{
    auto types = getTypesFromCategory(GraphObject::Category::FILTER);
    std::vector<std::shared_ptr<GraphObject>> filterList;
    for (const auto& type : types)
    {
        auto objectList = getObjectsPtr(getObjectsOfType(type));
        std::copy(objectList.begin(), objectList.end(), std::back_inserter(filterList));
    }
    filterList.erase(std::remove_if(filterList.begin(), filterList.end(), [](const auto& filter) { return !filter->getSavable(); }), filterList.end());

    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##filters", ImVec2(availableSize.x * 0.25, availableSize.y), true);
    ImGui::Text("Filter list");

    auto leftMargin = static_cast<int>(ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x);
    for (const auto& object : filterList)
    {
        auto filter = std::dynamic_pointer_cast<Filter>(object);
        auto spec = filter->getSpec();

        int w = ImGui::GetWindowWidth() - 3 * leftMargin;
        int h = w * spec.height / spec.width;

        if (ImGui::ImageButton((void*)(intptr_t)filter->getTexId(), ImVec2(w, h), ImVec2(0, 0), ImVec2(1, 1)))
            _selectedFilterName = filter->getName();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", filter->getAlias().c_str());
    }
    ImGui::EndChild();

    auto filter = std::dynamic_pointer_cast<Filter>(getObjectPtr(_selectedFilterName));
    if (!filter)
        return;

    ImGui::SameLine();
    ImGui::BeginChild("##filterInfo", ImVec2(0, 0), true);

    ImGui::Text("Parameters:");
    auto attributes = getObjectAttributes(_selectedFilterName);
    drawAttributes(_selectedFilterName, attributes);

    // If RGB curves are present, special treatment for them
    auto colorCurves = attributes.find("colorCurves");
    if (colorCurves != attributes.end())
    {
        const int curveHeight = 128;

        // Check that the attribute has real values in it
        bool hasCurves = false;
        for (const auto& curve : colorCurves->second)
        {
            if (!curve.as<Values>().empty())
                hasCurves = true;
        }

        if (hasCurves)
        {
            ImGui::BeginChild(colorCurves->first.c_str(), ImVec2(0, curveHeight + 4), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

            int curveCount = colorCurves->second.size();
            int w = ImGui::GetWindowWidth() - 3 * leftMargin;

            const std::vector<std::string> channels{"Red", "Green", "Blue"};
            int curveId = 0;
            for (const auto& curve : colorCurves->second)
            {
                std::vector<float> samples;
                for (const auto& v : curve.as<Values>())
                    samples.push_back(v.as<float>());

                if (curveId != 0)
                    ImGui::SameLine();

                ImGui::PlotLines(("##" + channels[curveId]).c_str(),
                    samples.data(),
                    samples.size(),
                    samples.size(),
                    channels[curveId].c_str(),
                    0.0,
                    1.0,
                    ImVec2(w / static_cast<float>(curveCount), curveHeight));

                ImGuiIO& io = ImGui::GetIO();
                if (io.MouseDownDuration[0] > 0.0)
                {
                    if (ImGui::IsItemHovered())
                    {
                        ImVec2 size = ImGui::GetItemRectSize();
                        ImVec2 curvePos = ImGui::GetItemRectMin();
                        ImVec2 mousePos = ImGui::GetMousePos();

                        float deltaY = static_cast<float>(io.MouseDelta.y) / static_cast<float>(size.y);
                        int id = (mousePos.x - curvePos.x) * curve.size() / size.x;

                        auto newCurves = colorCurves->second;
                        newCurves[curveId][id] = std::max(0.f, std::min(1.f, newCurves[curveId][id].as<float>() - deltaY));
                        setObjectAttribute(_selectedFilterName, colorCurves->first, newCurves);
                    }
                }

                ++curveId;
            }

            ImGui::EndChild();
        }
    }

    if (ImGui::TreeNode(("Filter preview: " + _selectedFilterName).c_str()))
    {
        auto spec = filter->getSpec();
        auto ratio = static_cast<float>(spec.height) / static_cast<float>(spec.width);

        auto leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
        int w = ImGui::GetWindowWidth() - 2 * leftMargin;
        int h = w * ratio;

        ImGui::Image(reinterpret_cast<ImTextureID>(filter->getTexId()), ImVec2(w, h));
        ImGui::TreePop();
    }

    ImGui::EndChild();
}

} // namespace Splash
