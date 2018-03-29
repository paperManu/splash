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
        auto& durationMap = Timer::get().getDurationMap();

        for (auto& t : durationMap)
        {
            if (_durationGraph.find(t.first) == _durationGraph.end())
                _durationGraph[t.first] = deque<unsigned long long>({t.second});
            else
            {
                if (_durationGraph[t.first].size() == _maxHistoryLength)
                    _durationGraph[t.first].pop_front();
                _durationGraph[t.first].push_back(t.second);
            }
        }

        if (_durationGraph.size() == 0)
            return;

        auto width = ImGui::GetWindowSize().x;
        for (auto& duration : _durationGraph)
        {
            float maxValue{0.f};
            vector<float> values;
            for (auto& v : duration.second)
            {
                maxValue = std::max((float)v * 0.001f, maxValue);
                values.push_back((float)v * 0.001f);
            }

            maxValue = ceil(maxValue * 0.1f) * 10.f;

            ImGui::PlotLines(
                "", values.data(), values.size(), values.size(), (duration.first + " - " + to_string((int)maxValue) + "ms").c_str(), 0.f, maxValue, ImVec2(width - 30, 80));
        }
    }
}

} // end of namespace
