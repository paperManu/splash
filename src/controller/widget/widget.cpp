#include "./controller/widget/widget.h"

#include <dirent.h>

#include <algorithm>
#include <array>
#include <filesystem>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>

#if HAVE_SHMDATA
#include "./image/image_shmdata.h"
#endif
#include "./core/scene.h"
#include "./utils/osutils.h"
#include "./utils/scope_guard.h"

namespace filesystem = std::filesystem;

namespace Splash
{

namespace SplashImGui
{
/*********/
bool FileSelectorParseDir(const std::string& sourcePath, std::vector<std::string>& list, const std::vector<std::string>& extensions, bool showNormalFiles)
{
    // Make the path absolute and if path is a file, get its parent directory
    bool isDirectoryPath = filesystem::is_directory(sourcePath);
    filesystem::path path = Utils::getPathFromFilePath(sourcePath);

    if (isDirectoryPath)
    {
        list.clear();
        std::vector<std::string> files = Utils::listDirContent(path.string());

        // Alphabetical order
        std::sort(files.begin(), files.end(), [](std::string a, std::string b) { return a < b; });

        // if path is not root add ".." to the list
        if (path != path.root_path())
            list.push_back("..");

        // But we put directories first
        std::copy_if(files.begin(), files.end(), std::back_inserter(list), [&path](std::string p) { return std::filesystem::is_directory(path / p); });

        std::copy_if(files.begin(), files.end(), std::back_inserter(list), [&path](std::string p) { return !std::filesystem::is_directory(path / p); });

        // Filter files based:
        // * on extension -> only show some specific extensions
        // * showNormalFiles: when false only show directories
        // * hidden files
        list.erase(std::remove_if(list.begin(),
                       list.end(),
                       [&extensions, &path, &showNormalFiles](const std::string& p) {
                           // remove hidden files and directories
                           if (p.size() > 2 && p[0] == '.')
                               return true;

                           // We don't want to filter the directories
                           if (filesystem::is_directory(path / p))
                               return false;

                           if (!showNormalFiles)
                               return true;

                           if (extensions.size() > 0)
                           {
                               for (const auto& ext : extensions)
                               {
                                   if (filesystem::path(p).extension().string() == ext)
                                       return false;
                               }

                               return true;
                           }
                           else
                               return false;
                       }),
            list.end());
    }

    return isDirectoryPath;
}

/*********/
bool FileSelector(const std::string& label, std::string& path, bool& cancelled, const std::vector<std::string>& extensions, bool showNormalFiles, bool newFile)
{
    path = Utils::getPathFromFilePath(path);

    static bool filterExtension = true;
    bool manualPath = false;
    bool selectionDone = false;
    cancelled = false;

    ImGui::PushID(label.c_str());

    std::string windowName = "Select file path";
    if (label.size() != 0)
        windowName += " - " + label;

    ImGui::SetNextWindowSize(ImVec2(400, 600));
    ImGui::SetNextWindowBgAlpha(0.99f);
    ImGui::Begin(windowName.c_str(), nullptr);

    ImGui::PushItemWidth(-64.f);
    std::vector<std::string> fileList;

    std::string newPath = Utils::getPathFromFilePath(path);
    std::string newFilename = Utils::getFilenameFromFilePath(path);
    if (SplashImGui::InputText("Path##FileSelectFullPath", newPath))
    {
        path = Utils::getPathFromFilePath(newPath);
    }
    if (SplashImGui::InputText("Filename##FileSelectFilename", newFilename))
    {
        path += Utils::getFilenameFromFilePath(newFilename);
    }
    ImGui::PopItemWidth();

    if (filterExtension)
    {
        if (!FileSelectorParseDir(path, fileList, extensions, showNormalFiles))
            manualPath = true;
    }
    else
    {
        if (!FileSelectorParseDir(path, fileList, {}, showNormalFiles))
            manualPath = true;
    }

    ImGui::BeginChild("##filelist", ImVec2(0, -48), true);
    static std::unordered_map<std::string, uint32_t> selectedId{};
    if (selectedId.find(label) == selectedId.end())
        selectedId[label] = 0;
    for (uint32_t i = 0; i < fileList.size(); ++i)
    {
        bool isSelected = (selectedId[label] == i);

        auto filename = fileList[i];
        if (std::filesystem::is_directory(path / filesystem::path(filename)))
            filename += "/";

        if (ImGui::Selectable(filename.c_str(), isSelected))
        {
            selectedId[label] = i;
            manualPath = false;
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            path = (std::filesystem::path(Utils::getPathFromFilePath(path)) / filename).string();
            if (!FileSelectorParseDir(path, fileList, extensions, showNormalFiles))
                selectionDone = true;
        }
    }
    ImGui::EndChild();

    ImGui::Checkbox("Filter files", &filterExtension);

    bool disableSelectPathButton = (selectedId[label] == 0) && newFile;
    if (disableSelectPathButton)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.1f);
    }
    if (ImGui::Button("Select path"))
    {
        if (!manualPath)
        {
            auto selectedIdIt = selectedId.find(label);
            if (selectedIdIt != selectedId.end())
            {
                if (selectedIdIt->second >= fileList.size())
                    selectedIdIt->second = 0;
                path = (std::filesystem::path(path) / fileList[selectedIdIt->second]).string();
            }
        }
        selectionDone = true;
    }
    if (disableSelectPathButton)
    {
        ImGui::PopStyleVar();
        ImGui::PopItemFlag();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        cancelled = true;

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            cancelled = true;
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter))
            selectionDone = true;
    }

    ImGui::End();
    ImGui::PopID();

    // Clean the path
    path = Utils::cleanPath(path);

    if (selectionDone || cancelled)
        return true;

    return false;
}

/*************/
bool InputText(const char* label, std::string& str, ImGuiInputTextFlags flags)
{
    str.resize(512, 0);
    if (ImGui::InputText(label, str.data(), str.size(), flags))
    {
        str.resize(str.find((char)0));
        return true;
    }
    return false;
}

} // end of namespace SplashImGui

/*************/
/*************/
GuiWidget::GuiWidget(Scene* scene, const std::string& name)
    : ControllerObject(scene, TreeRegisterStatus::NotRegistered)
    , _scene(scene)
{
    _name = name;
}

/*************/
void GuiWidget::drawAttributes(const std::string& objName, const std::unordered_map<std::string, Values>& attributes)
{
    const auto objAlias = getObjectAlias(objName);
    const auto lockedAttributes = getObjectAttribute(objName, "lockedAttributes");
    const auto generatedAttributes = getGeneratedAttributes(objName);

    std::vector<std::string> attributeNames;
    for (const auto& attr : attributes)
        attributeNames.push_back(attr.first);
    sort(attributeNames.begin(), attributeNames.end());

    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::PushItemWidth(availableSize.x * 0.5);

    for (const auto& attrName : attributeNames)
    {
        if (find(_hiddenAttributes.begin(), _hiddenAttributes.end(), attrName) != _hiddenAttributes.end())
            continue;

        const auto& attribute = attributes.find(attrName)->second;
        if (attribute.empty())
            continue;

        // Check whether the attribute is locked, in which case we set
        // the locking button color to red
        const bool isLocked =
            std::find_if(lockedAttributes.begin(), lockedAttributes.end(), [&](const Value& name) { return name.as<std::string>() == attrName; }) != lockedAttributes.end();

        ImGui::PushID((objName + attrName).c_str());
        OnScopeExit
        {
            ImGui::PopID();
        };

        if (isLocked)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15, 0.01, 0.01, 1.0));
        if (ImGui::Button("L"))
            setObjectAttribute(objName, "switchLock", {attrName});
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Lock / Unlock this attribute");
        if (isLocked)
            ImGui::PopStyleColor();
        ImGui::SameLine();

        if (ImGui::Button("R"))
            setObjectAttribute(objName, attrName, attribute);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("If pressed, resend the value as-is");
        ImGui::SameLine();

        const auto isGenerated = std::find(generatedAttributes.begin(), generatedAttributes.end(), attrName) != generatedAttributes.end();

        if (isGenerated && attribute.size() > 1)
        {
            /**
             * If the valid attribute values are generated, we should show a
             * combo box. In this case the list of valid values are stored inside
             * the attribute value (as per returned by the getter), and as such
             * start at index 1.
             */
            std::vector<std::string> labels;
            size_t currentSelection = 0;

            switch (attribute[0].getType())
            {
            default:
                assert(false);
                continue;
            case Value::Type::boolean:
            {
                const auto currentValue = attribute[0].as<bool>();
                for (size_t i = 1; i < attribute.size(); ++i)
                {
                    labels.push_back(attribute[i].as<std::string>());
                    if (attribute[i].as<bool>() == currentValue)
                        currentSelection = i - 1;
                }
                break;
            }
            case Value::Type::integer:
            {
                const auto currentValue = attribute[0].as<int64_t>();
                for (size_t i = 1; i < attribute.size(); ++i)
                {
                    labels.push_back(attribute[i].as<std::string>());
                    if (attribute[i].as<int64_t>() == currentValue)
                        currentSelection = i - 1;
                }
                break;
            }
            case Value::Type::real:
            {
                const auto currentValue = attribute[0].as<float>();
                for (size_t i = 1; i < attribute.size(); ++i)
                {
                    labels.push_back(attribute[i].as<std::string>());
                    if (attribute[i].as<float>() == currentValue)
                        currentSelection = i - 1;
                }
                break;
            }
            case Value::Type::string:
            {
                const auto currentValue = attribute[0].as<std::string>();
                for (size_t i = 1; i < attribute.size(); ++i)
                {
                    labels.push_back(attribute[i].as<std::string>());
                    if (attribute[i].as<std::string>() == currentValue)
                        currentSelection = i - 1;
                }
                break;
            }
            }

            std::vector<const char*> labelsAsChars;
            for (const auto& label : labels)
                labelsAsChars.push_back(label.c_str());

            // Now that we know the valid values, show them
            if (ImGui::BeginCombo(attrName.c_str(), labelsAsChars[currentSelection]))
            {
                for (size_t i = 0; i < labelsAsChars.size(); ++i)
                {
                    const bool isSelected = (currentSelection == i);
                    if (ImGui::Selectable(labelsAsChars[i], isSelected))
                        setObjectAttribute(objName, attrName, {attribute[i + 1]});
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            switch (attribute[0].getType())
            {
            default:
                ImGui::Text("%s: not displayed", attrName.c_str());
                continue;
            case Value::Type::boolean:
            {
                auto tmp = attribute[0].as<bool>();
                if (ImGui::Checkbox(attrName.c_str(), &tmp))
                    setObjectAttribute(objName, attrName, {static_cast<bool>(tmp)});

                break;
            }
            case Value::Type::integer:
            {
                switch (attribute.size())
                {
                default:
                    ImGui::Text("%s: not displayed", attrName.c_str());
                    continue;
                case 1:
                {
                    auto tmp = attribute[0].as<int64_t>();
                    static const int step = 1;
                    ImGui::InputScalar(attrName.c_str(), ImGuiDataType_S64, &tmp, &step, &step, nullptr);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        setObjectAttribute(objName, attrName, {static_cast<int64_t>(tmp)});
                    break;
                }
                case 2:
                {
                    std::array<int64_t, 2> tmp;
                    tmp[0] = attribute[0].as<int64_t>();
                    tmp[1] = attribute[1].as<int64_t>();
                    ImGui::InputScalarN(attrName.c_str(), ImGuiDataType_S64, tmp.data(), 2, nullptr, nullptr, nullptr);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        setObjectAttribute(objName, attrName, {tmp[0], tmp[1]});
                    break;
                }
                case 3:
                {
                    std::array<int64_t, 3> tmp;
                    tmp[0] = attribute[0].as<int64_t>();
                    tmp[1] = attribute[1].as<int64_t>();
                    tmp[2] = attribute[2].as<int64_t>();
                    ImGui::InputScalarN(attrName.c_str(), ImGuiDataType_S64, tmp.data(), 3, nullptr, nullptr, nullptr);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2]});
                    break;
                }
                case 4:
                {
                    std::array<int64_t, 4> tmp;
                    tmp[0] = attribute[0].as<int64_t>();
                    tmp[1] = attribute[1].as<int64_t>();
                    tmp[2] = attribute[2].as<int64_t>();
                    tmp[3] = attribute[3].as<int64_t>();
                    ImGui::InputScalarN(attrName.c_str(), ImGuiDataType_S64, tmp.data(), 4, nullptr, nullptr, nullptr);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2], tmp[3]});
                    break;
                }
                }

                break;
            }
            case Value::Type::real:
            {
                bool isReal = false;
                if (attribute[0].getType() == Value::Type::real)
                    isReal = true;

                switch (attribute.size())
                {
                default:
                    ImGui::Text("%s: not displayed", attrName.c_str());
                    continue;
                case 1:
                {
                    if (isReal)
                    {
                        auto tmp = attribute[0].as<float>();
                        const auto step = 0.01f * tmp;
                        ImGui::InputFloat(attrName.c_str(), &tmp, step, step, "%.3f");
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp});
                    }
                    else
                    {
                        auto tmp = attribute[0].as<int32_t>();
                        const auto stepSlow = 1;
                        const auto stepFast = 100;
                        ImGui::InputInt(attrName.c_str(), &tmp, stepSlow, stepFast);
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp});
                    }
                    break;
                }
                case 2:
                {
                    if (isReal)
                    {
                        std::array<float, 2> tmp;
                        tmp[0] = attribute[0].as<float>();
                        tmp[1] = attribute[1].as<float>();
                        ImGui::InputFloat2(attrName.c_str(), tmp.data(), "%.3f");
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp[0], tmp[1]});
                    }
                    else
                    {
                        std::array<int32_t, 2> tmp;
                        tmp[0] = attribute[0].as<int32_t>();
                        tmp[1] = attribute[1].as<int32_t>();
                        ImGui::InputInt2(attrName.c_str(), tmp.data());
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp[0], tmp[1]});
                    }
                    break;
                }
                case 3:
                {
                    if (isReal)
                    {
                        std::array<float, 3> tmp;
                        tmp[0] = attribute[0].as<float>();
                        tmp[1] = attribute[1].as<float>();
                        tmp[2] = attribute[2].as<float>();
                        ImGui::InputFloat3(attrName.c_str(), tmp.data(), "%.3f");
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2]});
                    }
                    else
                    {
                        std::array<int32_t, 3> tmp;
                        tmp[0] = attribute[0].as<int32_t>();
                        tmp[1] = attribute[1].as<int32_t>();
                        tmp[2] = attribute[2].as<int32_t>();
                        ImGui::InputInt3(attrName.c_str(), tmp.data());
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2]});
                    }
                    break;
                }
                case 4:
                {
                    if (isReal)
                    {
                        std::array<float, 4> tmp;
                        tmp[0] = attribute[0].as<float>();
                        tmp[1] = attribute[1].as<float>();
                        tmp[2] = attribute[2].as<float>();
                        tmp[3] = attribute[3].as<float>();
                        ImGui::InputFloat4(attrName.c_str(), tmp.data(), "%.3f");
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2], tmp[3]});
                    }
                    else
                    {
                        std::array<int32_t, 4> tmp;
                        tmp[0] = attribute[0].as<int32_t>();
                        tmp[1] = attribute[1].as<int32_t>();
                        tmp[2] = attribute[2].as<int32_t>();
                        tmp[3] = attribute[3].as<int32_t>();
                        ImGui::InputInt4(attrName.c_str(), tmp.data());
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2], tmp[3]});
                    }
                    break;
                }
                }

                break;
            }
            case Value::Type::values:
            {
                // We skip anything that looks like a vector / matrix
                // (for usefulness reasons...)
                Values values = attribute[0].as<Values>();
                if (values.size() > 16)
                {
                    if (values[0].isConvertibleToType(Value::Type::real))
                    {
                        float minValue = std::numeric_limits<float>::max();
                        float maxValue = std::numeric_limits<float>::min();
                        std::vector<float> samples;
                        for (const auto& v : values)
                        {
                            auto value = v.as<float>();
                            maxValue = std::max(value, maxValue);
                            minValue = std::min(value, minValue);
                            samples.push_back(value);
                        }

                        ImGui::PlotLines(attrName.c_str(),
                            samples.data(),
                            samples.size(),
                            samples.size(),
                            ("[" + std::to_string(minValue) + ", " + std::to_string(maxValue) + "]").c_str(),
                            minValue,
                            maxValue,
                            ImVec2(0, 100));
                    }
                }
                else
                {
                    ImGui::Text("%s: not displayed", attrName.c_str());
                }
                break;
            }
            case Value::Type::string:
            {
                for (const auto& v : attribute)
                {
                    // We have a special way to handle file paths
                    if (attrName.find("file") == 0)
                    {

                        std::string tmp = v.as<std::string>();
                        SplashImGui::InputText("", tmp);
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp});

                        // Callback for dragndrop: replace the file path in the field
                        if (ImGui::IsItemHovered())
                            UserInput::setCallback(
                                UserInput::State("dragndrop"), [=](const UserInput::State& state) { setObjectAttribute(objName, attrName, {state.value[0].as<std::string>()}); });
                        else // Not really necessary as the GUI sets a default dragndrop
                             // callback at each frame, but anyway
                            UserInput::resetCallback(UserInput::State("dragndrop"));

                        ImGui::SameLine();
                        if (ImGui::Button("..."))
                        {
                            _fileSelectorTarget = objName;
                        }
                        if (_fileSelectorTarget == objName)
                        {
                            static std::string path = _root->getMediaPath();
                            bool cancelled;
                            std::vector<std::string> extensions{{".bmp"}, {".jpg"}, {".png"}, {".tga"}, {".tif"}, {".avi"}, {".mov"}, {".mp4"}, {".obj"}};
                            if (SplashImGui::FileSelector(objAlias, path, cancelled, extensions))
                            {
                                if (!cancelled)
                                    setObjectAttribute(objName, attrName, {path});
                                path = _root->getMediaPath();
                                _fileSelectorTarget = "";
                            }
                        }
                    }
                    // For everything else ...
                    else
                    {
                        std::string tmp = v.as<std::string>();
                        SplashImGui::InputText(attrName.c_str(), tmp);
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            setObjectAttribute(objName, attrName, {tmp});
                    }
                }
                break;
            }
            }
        }

        if (ImGui::IsItemHovered())
        {
            auto answer = getObjectAttributeDescription(objName, attrName);
            if (answer.size() != 0)
                ImGui::SetTooltip("%s", answer[0].as<std::string>().c_str());
        }
    }
    ImGui::PopItemWidth();
}

/*************/
const char* GuiWidget::getLocalKeyName(char key)
{
    return glfwGetKeyName(key, 0);
}

} // namespace Splash
