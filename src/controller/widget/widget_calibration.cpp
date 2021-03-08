#include "./controller/widget/widget_calibration.h"

#if HAVE_CALIMIRO
#include <calimiro/texture_coordinates.h>
#endif

namespace Splash
{

/*************/
GuiCalibration::GuiCalibration(Scene* scene, const std::string& name)
    : GuiWidget(scene, name)
{
#if HAVE_CALIMIRO
    std::string methodToString;
    for (auto const& method : calimiro::TextureCoordinates::getUVMethodList())
    {
        methodToString = calimiro::TextureCoordinates::getUVMethodTitle(method);
        _uvMethodsList.push_back(methodToString);
        _stringToUVMethod[methodToString] = method;
    }
#endif
}

/*************/
void GuiCalibration::render()
{
    ImVec2 availableSize = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("##calibration_wrapper", ImVec2(availableSize.x, availableSize.y), true);

#if !HAVE_CALIMIRO
    ImGui::Text("Install Calimiro for calibration options");
#else
    assert(getObjectPtr("geometricCalibrator") != nullptr);

    // Header
    ImGui::Text("Geometric Calibration");
    ImGui::Text("Positions captured: %i (recommended: 3+)", getObjectAttribute("geometricCalibrator", "positionCount")[0].as<int>());

    // Calibration actions
    if (ImGui::Button("Start calibration", ImVec2(availableSize.x * 0.5f - 12.f, 32.f)))
    {
        setObjectAttribute("geometricCalibrator", "calibrate", {1});
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Enter the calibration mode");

    ImGui::SameLine();
    if (ImGui::Button("Capture new position", ImVec2(availableSize.x * 0.5f - 12.f, 32.f)))
    {
        setObjectAttribute("geometricCalibrator", "nextPosition", {1});
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Capture the patterns for the current camera position");

    if (ImGui::Button("Finalize calibration", ImVec2(availableSize.x * 0.5f - 12.f, 32.f)))
        setObjectAttribute("geometricCalibrator", "finalizeCalibration", {1});
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Compute calibration from the captured patterns");

    ImGui::SameLine();
    if (ImGui::Button("Abort calibration", ImVec2(availableSize.x * 0.5f - 12.f, 32.f)))
        setObjectAttribute("geometricCalibrator", "abortCalibration", {1});
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Abort the calibration process");

    ImGui::Separator();

    // UV settings
    ImGui::Text("Automatic texture coordinates generation");
    ImGui::Text("Projection type: ");
    ImGui::SameLine();

    auto currentMethod = getObjectAttribute("geometricCalibrator", "uvMethod")[0];
    std::string tmp_method = currentMethod[0].as<std::string>();
    if (ImGui::BeginCombo("##UV method", tmp_method.c_str()))
    {
        for (const auto& method : _uvMethodsList)
        {
            bool isSelected = (tmp_method == method);
            if (ImGui::Selectable(method.c_str(), isSelected))
            {
                tmp_method = method;
                setObjectAttribute("geometricCalibrator", "uvMethod", {(int)_stringToUVMethod[tmp_method]});
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TextWrapped("%s: %s",
        calimiro::TextureCoordinates::getUVMethodTitle(_stringToUVMethod[tmp_method]).c_str(),
        calimiro::TextureCoordinates::getUVMethodDescription(_stringToUVMethod[tmp_method]).c_str());

    // Camera position
    auto cameraPosition = getObjectAttribute("geometricCalibrator", "uvCameraPosition");
    std::array<float, 3> tmp_position;
    tmp_position[0] = cameraPosition[0].as<float>();
    tmp_position[1] = cameraPosition[1].as<float>();
    tmp_position[2] = cameraPosition[2].as<float>();
    std::string position_name = "Eye position";
    if (ImGui::InputFloat3(position_name.c_str(), tmp_position.data(), 5, ImGuiInputTextFlags_None))
        setObjectAttribute("geometricCalibrator", "uvCameraPosition", {tmp_position[0], tmp_position[1], tmp_position[2]});

    // Camera orientation
    auto cameraOrientation = getObjectAttribute("geometricCalibrator", "uvCameraOrientation");
    std::array<float, 3> tmp_orientation;
    tmp_orientation[0] = cameraOrientation[0].as<float>();
    tmp_orientation[1] = cameraOrientation[1].as<float>();
    tmp_orientation[2] = cameraOrientation[2].as<float>();
    std::string orientation_name = "Eye orientation";
    if (ImGui::InputFloat3(orientation_name.c_str(), tmp_orientation.data(), 5, ImGuiInputTextFlags_None))
        setObjectAttribute("geometricCalibrator", "uvCameraOrientation", {tmp_orientation[0], tmp_orientation[1], tmp_orientation[2]});

#endif

    ImGui::EndChild();
}

/*************/
int GuiCalibration::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    return flags;
}

} // namespace Splash
