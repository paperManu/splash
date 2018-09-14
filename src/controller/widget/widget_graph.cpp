#include "./controller/widget/widget_graph.h"

#include <imgui.h>

using namespace std;

namespace Splash
{

/*************/
void GuiGraph::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto tree = _root->getTree();
        auto width = ImGui::GetWindowSize().x;

        for (const auto& branchName : tree->getBranchList())
        {
            if (ImGui::TreeNode(branchName.c_str()))
            {
                auto branchPath = "/" + branchName + "/durations";
                for (const auto& leafName : tree->getLeafListAt(branchPath))
                {
                    auto durationName = branchName + "_" + leafName;
                    Value value;
                    tree->getValueForLeafAt(branchPath + "/" + leafName, value);
                    assert(value.size() == 1);
                    if (_durationGraph.find(durationName) == _durationGraph.end())
                    {
                        _durationGraph[durationName] = deque<unsigned long long>(value[0].as<float>());
                    }
                    else
                    {
                        while (_durationGraph[durationName].size() >= _maxHistoryLength)
                            _durationGraph[durationName].pop_front();
                        _durationGraph[durationName].push_back(value[0].as<float>());
                    }

                    float maxValue{0.f};
                    vector<float> values;
                    for (auto& v : _durationGraph[durationName])
                    {
                        maxValue = std::max((float)v * 0.001f, maxValue);
                        values.push_back((float)v * 0.001f);
                    }

                    maxValue = ceil(maxValue * 0.1f) * 10.f;

                    ImGui::PlotLines(
                        "", values.data(), values.size(), values.size(), (leafName + " - " + to_string((int)maxValue) + "ms").c_str(), 0.f, maxValue, ImVec2(width - 30, 80));
                }

                ImGui::TreePop();
            }
        }
    }
}

} // namespace Splash
