#include "./controller/widget/widget_filters.h"

#include <imgui.h>

#include "./graphics/filter.h"

using namespace std;

namespace Splash
{

/*************/
void GuiFilters::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto filterList = getObjectsPtr(getObjectsOfType("filter"));
        for (auto& filterObj : filterList)
        {
            auto filterName = filterObj->getName();
            if (ImGui::TreeNode(filterObj->getAlias().c_str()))
            {
                ImGui::Text("Parameters:");
                auto attributes = getObjectAttributes(filterObj->getName());
                drawAttributes(filterName, attributes);

                // If RGB curves are present, special treatment for them
                auto colorCurves = attributes.find("colorCurves");
                if (colorCurves != attributes.end())
                {
                    const int curveHeight = 128;

                    ImGui::BeginChild(colorCurves->first.c_str(), ImVec2(0, curveHeight + 4), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

                    double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
                    int w = ImGui::GetWindowWidth() - 3 * leftMargin;

                    const vector<string> channels{"Red", "Green", "Blue"};
                    int curveId = 0;
                    for (auto& curve : colorCurves->second)
                    {
                        vector<float> samples;
                        for (auto& v : curve.as<Values>())
                            samples.push_back(v.as<float>());

                        if (curveId != 0)
                            ImGui::SameLine();

                        ImGui::PlotLines(
                            ("##" + channels[curveId]).c_str(), samples.data(), samples.size(), samples.size(), channels[curveId].c_str(), 0.0, 1.0, ImVec2(w / 3, curveHeight));

                        ImGuiIO& io = ImGui::GetIO();
                        if (io.MouseDownDuration[0] > 0.0)
                        {
                            if (ImGui::IsItemHovered())
                            {
                                ImVec2 delta = io.MouseDelta;
                                ImVec2 size = ImGui::GetItemRectSize();
                                ImVec2 curvePos = ImGui::GetItemRectMin();
                                ImVec2 mousePos = ImGui::GetMousePos();

                                float deltaY = (float)delta.y / (float)size.y;
                                int id = (mousePos.x - curvePos.x) * curve.size() / size.x;

                                auto newCurves = colorCurves->second;
                                newCurves[curveId][id] = max(0.f, min(1.f, newCurves[curveId][id].as<float>() - deltaY));
                                setObjectAttribute(filterName, colorCurves->first, newCurves);
                            }
                        }

                        ++curveId;
                    }

                    ImGui::EndChild();
                }

                if (ImGui::TreeNode(("Filter preview: " + filterName).c_str()))
                {
                    auto filter = dynamic_pointer_cast<Filter>(filterObj);
                    auto spec = filter->getSpec();
                    auto ratio = static_cast<float>(spec.height) / static_cast<float>(spec.width);
                    auto texture = dynamic_pointer_cast<Texture_Image>(filter->getOutTexture());

                    double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
                    int w = ImGui::GetWindowWidth() - 2 * leftMargin;
                    int h = w * ratio;

                    ImGui::Image(reinterpret_cast<ImTextureID>(texture->getTexId()), ImVec2(w, h));
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }
    }
}

} // end of namespaces
