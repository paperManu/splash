#include "./controller/widget/widget.h"

#include <dirent.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "./graphics/camera.h"
#include "./image/image.h"
#include "./image/image_ffmpeg.h"
#if HAVE_SHMDATA
#include "./image/image_shmdata.h"
#endif
#include "./core/scene.h"
#include "./graphics/object.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./utils/log.h"
#include "./utils/osutils.h"
#include "./utils/timer.h"

#if HAVE_GPHOTO and HAVE_OPENCV
#include "./controller/colorcalibrator.h"
#endif

using namespace std;

namespace Splash
{

namespace SplashImGui
{
/*********/
bool FileSelectorParseDir(const string& sourcePath, vector<string>& list, const vector<string>& extensions, bool showNormalFiles)
{
    // Make the path absolute and if path is a file, get its parent directory
    bool isDirectoryPath = filesystem::is_directory(sourcePath);
    filesystem::path path = Utils::getPathFromFilePath(sourcePath);

    if (isDirectoryPath)
    {
        list.clear();
        vector<string> files = Utils::listDirContent(path);

        // Alphabetical order
        std::sort(files.begin(), files.end(), [](string a, string b) { return a < b; });

        // if path is not root add ".." to the list
        if (path != path.root_path())
            list.push_back("..");

        // But we put directories first
        std::copy_if(files.begin(), files.end(), std::back_inserter(list), [&path](string p) { return std::filesystem::is_directory(path / p); });

        std::copy_if(files.begin(), files.end(), std::back_inserter(list), [&path](string p) { return !std::filesystem::is_directory(path / p); });

        // Filter files based:
        // * on extension -> only show some specific extensions
        // * showNormalFiles: when false only show directories
        // * hidden files
        list.erase(std::remove_if(list.begin(),
                       list.end(),
                       [&extensions, &path, &showNormalFiles](const string& p) {
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
                                   if (std::string(filesystem::path(p).extension()) == ext)
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
bool FileSelector(const string& label, string& path, bool& cancelled, const vector<string>& extensions, bool showNormalFiles, bool newFile)
{
    path = Utils::getPathFromFilePath(path);

    static bool filterExtension = true;
    bool manualPath = false;
    bool selectionDone = false;
    cancelled = false;

    ImGui::PushID(label.c_str());

    string windowName = "Select file path";
    if (label.size() != 0)
        windowName += " - " + label;

    ImGui::Begin(windowName.c_str(), nullptr, ImVec2(400, 600), 0.99f);

    ImGui::PushItemWidth(-64.f);
    vector<string> fileList;

    string newPath = Utils::getPathFromFilePath(path);
    string newFilename = Utils::getFilenameFromFilePath(path);
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
    static unordered_map<string, uint32_t> selectedId{};
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
            path = std::filesystem::path(Utils::getPathFromFilePath(path)) / filename;
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
                path = std::filesystem::path(path) / fileList[selectedIdIt->second];
            }
        }
        selectionDone = true;
    }
    if (disableSelectPathButton)
    {
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        cancelled = true;

    if (ImGui::IsRootWindowOrAnyChildFocused())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.KeysDown[io.KeyMap[ImGuiKey_Escape]] && io.KeysDownDuration[io.KeyMap[ImGuiKey_Escape]] == 0.0)
            cancelled = true;
        else if (io.KeysDown[io.KeyMap[ImGuiKey_Enter]] && io.KeysDownDuration[io.KeyMap[ImGuiKey_Enter]] == 0.0)
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
GuiWidget::GuiWidget(Scene* scene, const string& name)
    : ControllerObject(scene)
    , _scene(scene)
{
    _name = name;
}

/*************/
void GuiWidget::drawAttributes(const string& objName, const unordered_map<string, Values>& attributes)
{
    auto objAlias = getObjectAlias(objName);
    vector<string> attributeNames;
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
        if (attribute.empty() || attribute.size() > 4)
            continue;

        switch (attribute[0].getType())
        {
        default:
            continue;
        case Value::Type::boolean:
        {
            ImGui::PushID(attrName.c_str());
            if (ImGui::Button("L"))
                setObjectAttribute(objName, "switchLock", {attrName});
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lock / Unlock this attribute");
            ImGui::SameLine();

            if (ImGui::Button("R"))
                setObjectAttribute(objName, attrName, attribute);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("If pressed, resend the value as-is");
            ImGui::SameLine();

            ImGui::PopID();
            auto tmp = attribute[0].as<bool>();
            if (ImGui::Checkbox(attrName.c_str(), &tmp))
                setObjectAttribute(objName, attrName, {static_cast<bool>(tmp)});

            break;
        }
        case Value::Type::integer:
        {
            ImGui::PushID(attrName.c_str());
            if (ImGui::Button("L"))
                setObjectAttribute(objName, "switchLock", {attrName});
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lock / Unlock this attribute");
            ImGui::SameLine();

            if (ImGui::Button("R"))
                setObjectAttribute(objName, attrName, attribute);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("If pressed, resend the value as-is");
            ImGui::SameLine();

            ImGui::PopID();

            switch (attribute.size())
            {
            default:
                continue;
            case 1:
            {
                auto tmp = attribute[0].as<int64_t>();
                static const float step = 1;
                if (ImGui::InputScalar(attrName.c_str(), ImGuiDataType_S64, &tmp, &step, &step, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {static_cast<int64_t>(tmp)});
                break;
            }
            case 2:
            {
                array<int64_t, 2> tmp;
                tmp[0] = attribute[0].as<int64_t>();
                tmp[1] = attribute[1].as<int64_t>();
                if (ImGui::InputScalarN(attrName.c_str(), ImGuiDataType_S64, tmp.data(), 2, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {tmp[0], tmp[1]});
                break;
            }
            case 3:
            {
                array<int64_t, 3> tmp;
                tmp[0] = attribute[0].as<int64_t>();
                tmp[1] = attribute[1].as<int64_t>();
                tmp[2] = attribute[2].as<int64_t>();
                if (ImGui::InputScalarN(attrName.c_str(), ImGuiDataType_S64, tmp.data(), 3, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2]});
                break;
            }
            case 4:
            {
                array<int64_t, 4> tmp;
                tmp[0] = attribute[0].as<int64_t>();
                tmp[1] = attribute[1].as<int64_t>();
                tmp[2] = attribute[2].as<int64_t>();
                tmp[3] = attribute[3].as<int64_t>();
                if (ImGui::InputScalarN(attrName.c_str(), ImGuiDataType_S64, tmp.data(), 4, nullptr, nullptr, nullptr, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2], tmp[3]});
                break;
            }
            }

            break;
        }
        case Value::Type::real:
        {
            ImGui::PushID(attrName.c_str());
            if (ImGui::Button("L"))
                setObjectAttribute(objName, "switchLock", {attrName});
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lock / Unlock this attribute");
            ImGui::SameLine();

            if (ImGui::Button("R"))
                setObjectAttribute(objName, attrName, attribute);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("If pressed, resend the value as-is");
            ImGui::SameLine();

            ImGui::PopID();

            int precision = 0;
            if (attribute[0].getType() == Value::Type::real)
                precision = 3;

            switch (attribute.size())
            {
            default:
                continue;
            case 1:
            {
                auto tmp = attribute[0].as<float>();
                float step = 0.01 * tmp;
                if (ImGui::InputFloat(attrName.c_str(), &tmp, step, step, precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {static_cast<float>(tmp)});
                break;
            }
            case 2:
            {
                array<float, 2> tmp;
                tmp[0] = attribute[0].as<float>();
                tmp[1] = attribute[1].as<float>();
                if (ImGui::InputFloat2(attrName.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {tmp[0], tmp[1]});
                break;
            }
            case 3:
            {
                array<float, 3> tmp;
                tmp[0] = attribute[0].as<float>();
                tmp[1] = attribute[1].as<float>();
                tmp[2] = attribute[2].as<float>();
                if (ImGui::InputFloat3(attrName.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2]});
                break;
            }
            case 4:
            {
                array<float, 4> tmp;
                tmp[0] = attribute[0].as<float>();
                tmp[1] = attribute[1].as<float>();
                tmp[2] = attribute[2].as<float>();
                tmp[3] = attribute[3].as<float>();
                if (ImGui::InputFloat4(attrName.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    setObjectAttribute(objName, attrName, {tmp[0], tmp[1], tmp[2], tmp[3]});
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
                    float minValue = numeric_limits<float>::max();
                    float maxValue = numeric_limits<float>::min();
                    vector<float> samples;
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
                        ("[" + to_string(minValue) + ", " + to_string(maxValue) + "]").c_str(),
                        minValue,
                        maxValue,
                        ImVec2(0, 100));
                }
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

                    string tmp = v.as<string>();
                    ImGui::PushID((objName + attrName).c_str());
                    if (SplashImGui::InputText("", tmp, ImGuiInputTextFlags_EnterReturnsTrue))
                        setObjectAttribute(objName, attrName, {tmp});

                    // Callback for dragndrop: replace the file path in the field
                    if (ImGui::IsItemHovered())
                        UserInput::setCallback(
                            UserInput::State("dragndrop"), [=](const UserInput::State& state) { setObjectAttribute(objName, attrName, {state.value[0].as<string>()}); });
                    else // Not really necessary as the GUI sets a default dragndrop callback at each frame, but anyway
                        UserInput::resetCallback(UserInput::State("dragndrop"));

                    ImGui::SameLine();
                    if (ImGui::Button("..."))
                    {
                        _fileSelectorTarget = objName;
                    }
                    if (_fileSelectorTarget == objName)
                    {
                        static string path = _root->getMediaPath();
                        bool cancelled;
                        vector<string> extensions{{".bmp"}, {".jpg"}, {".png"}, {".tga"}, {".tif"}, {".avi"}, {".mov"}, {".mp4"}, {".obj"}};
                        if (SplashImGui::FileSelector(objAlias, path, cancelled, extensions))
                        {
                            if (!cancelled)
                                setObjectAttribute(objName, attrName, {path});
                            path = _root->getMediaPath();
                            _fileSelectorTarget = "";
                        }
                    }

                    ImGui::PopID();
                }
                // For everything else ...
                else
                {
                    string tmp = v.as<string>();
                    if (SplashImGui::InputText(attrName.c_str(), tmp, ImGuiInputTextFlags_EnterReturnsTrue))
                        setObjectAttribute(objName, attrName, {tmp});
                }
            }
            break;
        }
        }

        if (ImGui::IsItemHovered())
        {
            auto answer = getObjectAttributeDescription(objName, attrName);
            if (answer.size() != 0)
                ImGui::SetTooltip("%s", answer[0].as<string>().c_str());
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
