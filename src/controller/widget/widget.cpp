#include "./controller/widget/widget.h"

#include <dirent.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <imgui.h>

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

#if HAVE_GPHOTO
#include "./controller/colorcalibrator.h"
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

using namespace std;

namespace Splash
{

namespace SplashImGui
{
/*********/
bool FileSelectorParseDir(string& path, vector<FilesystemFile>& list, const vector<string>& extensions, bool showNormalFiles)
{
    bool isDirectoryPath = true;
    path = Utils::cleanPath(path);
    string tmpPath = path;

    auto directory = opendir(tmpPath.c_str());
    if (directory == nullptr)
    {
        isDirectoryPath = false;
        tmpPath = Utils::getPathFromFilePath(tmpPath);
        directory = opendir(tmpPath.c_str());
    }

    if (directory != nullptr)
    {
        list.clear();
        vector<FilesystemFile> files{};

        struct dirent* dirEntry;
        while ((dirEntry = readdir(directory)) != nullptr)
        {
            FilesystemFile path;
            path.filename = dirEntry->d_name;

            // Do not show hidden files
            if (path.filename.size() > 2 && path.filename[0] == '.')
                continue;

            if (dirEntry->d_type == DT_DIR)
                path.isDir = true;
            else if (!showNormalFiles)
                continue;

            files.push_back(path);
        }
        closedir(directory);

        // Alphabetical order
        std::sort(files.begin(), files.end(), [](FilesystemFile a, FilesystemFile b) { return a.filename < b.filename; });

        // But we put directories first
        std::copy_if(files.begin(), files.end(), std::back_inserter(list), [](FilesystemFile p) { return p.isDir; });

        std::copy_if(files.begin(), files.end(), std::back_inserter(list), [](FilesystemFile p) { return !p.isDir; });

        // Filter files based on extension
        if (extensions.size() != 0)
        {
            list.erase(std::remove_if(list.begin(),
                           list.end(),
                           [&extensions](FilesystemFile p) {
                               if (p.isDir)
                                   return false;

                               bool filteredOut = true;
                               for (auto& ext : extensions)
                               {
                                   auto pos = p.filename.rfind(ext);
                                   if (pos != string::npos && pos == p.filename.size() - ext.size())
                                       filteredOut = false;
                               }

                               return filteredOut;
                           }),
                list.end());
        }
    }

    return isDirectoryPath;
}

/*********/
bool FileSelector(const string& label, string& path, bool& cancelled, const vector<string>& extensions, bool showNormalFiles)
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

    ImGui::Begin(windowName.c_str(), nullptr, ImVec2(400, 600), 0.95f);
    char textBuffer[512];
    memset(textBuffer, 0, 512);
    strncpy(textBuffer, path.c_str(), 510);

    ImGui::PushItemWidth(-1.f);
    vector<FilesystemFile> fileList;
    if (ImGui::InputText("##FileSelectFullPath", textBuffer, 512))
    {
        path = string(textBuffer);
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

        auto filename = fileList[i].filename;
        if (fileList[i].isDir)
            filename += "/";

        if (ImGui::Selectable(filename.c_str(), isSelected))
        {
            selectedId[label] = i;
            manualPath = false;
        }
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        path = path + "/" + fileList[selectedId[label]].filename;
        if (!FileSelectorParseDir(path, fileList, extensions, showNormalFiles))
            selectionDone = true;
    }
    ImGui::EndChild();

    ImGui::Checkbox("Filter files", &filterExtension);

    if (ImGui::Button("Select path"))
    {
        if (!manualPath)
        {
            auto selectedIdIt = selectedId.find(label);
            if (selectedIdIt != selectedId.end())
            {
                if (selectedIdIt->second >= fileList.size())
                    selectedIdIt->second = 0;
                path = path + "/" + fileList[selectedIdIt->second].filename;
            }
        }
        selectionDone = true;
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

} // end of namespace SplashImGui

/*************/
/*************/
GuiWidget::GuiWidget(Scene* scene, string name)
    : ControllerObject(scene)
    , _name(name)
    , _scene(scene)
{
}

/*************/
void GuiWidget::drawAttributes(const string& objName, const unordered_map<string, Values>& attributes)
{
    auto scene = dynamic_cast<Scene*>(_scene);
    if (!scene)
        return;

    vector<string> attributeNames;
    for (auto& attr : attributes)
        attributeNames.push_back(attr.first);
    sort(attributeNames.begin(), attributeNames.end());

    for (auto& attrName : attributeNames)
    {
        const auto& attribute = attributes.find(attrName)->second;
        if (attribute.empty() || attribute.size() > 4)
            continue;

        switch (attribute[0].getTypeAsChar())
        {
        default:
            continue;
        case 'n':
        {
            ImGui::PushID(attrName.c_str());
            if (ImGui::Button("L"))
                scene->sendMessageToWorld("sendAll", {objName, "switchLock", attrName});
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Lock / Unlock this attribute");
            ImGui::PopID();
            ImGui::SameLine();

            int precision = 0;
            if (attribute[0].getType() == Value::Type::f)
                precision = 3;

            switch (attribute.size())
            {
            default:
                continue;
            case 1:
            {
                float tmp = attribute[0].as<float>();
                float step = attribute[0].getType() == Value::Type::f ? 0.01 * tmp : 1.f;
                if (ImGui::InputFloat(attrName.c_str(), &tmp, step, step, precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attrName, tmp});
                break;
            }
            case 2:
            {
                vector<float> tmp;
                tmp.push_back(attribute[0].as<float>());
                tmp.push_back(attribute[1].as<float>());
                if (ImGui::InputFloat2(attrName.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attrName, tmp[0], tmp[1]});
                break;
            }
            case 3:
            {
                vector<float> tmp;
                tmp.push_back(attribute[0].as<float>());
                tmp.push_back(attribute[1].as<float>());
                tmp.push_back(attribute[2].as<float>());
                if (ImGui::InputFloat3(attrName.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attrName, tmp[0], tmp[1], tmp[2]});
                break;
            }
            case 4:
            {
                vector<float> tmp;
                tmp.push_back(attribute[0].as<float>());
                tmp.push_back(attribute[1].as<float>());
                tmp.push_back(attribute[2].as<float>());
                tmp.push_back(attribute[3].as<float>());
                if (ImGui::InputFloat4(attrName.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attrName, tmp[0], tmp[1], tmp[2], tmp[3]});
                break;
            }
            }
            break;
        }
        case 'v':
        {
            // We skip anything that looks like a vector / matrix
            // (for usefulness reasons...)
            Values values = attribute[0].as<Values>();
            if (values.size() > 16)
            {
                if (values[0].getTypeAsChar() == 'n')
                {
                    float minValue = numeric_limits<float>::max();
                    float maxValue = numeric_limits<float>::min();
                    vector<float> samples;
                    for (auto& v : values)
                    {
                        float value = v.as<float>();
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
        case 's':
        {
            for (auto& v : attribute)
            {
                // We have a special way to handle file paths
                if (attrName.find("file") == 0)
                {

                    string tmp = v.as<string>();
                    tmp.resize(512);
                    ImGui::PushID((objName + attrName).c_str());
                    if (ImGui::InputText("", const_cast<char*>(tmp.c_str()), tmp.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {objName, attrName, tmp});

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
                        vector<string> extensions{{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"}, {"avi"}, {"mov"}, {"mp4"}, {"obj"}};
                        if (SplashImGui::FileSelector(objName, path, cancelled, extensions))
                        {
                            if (!cancelled)
                                scene->sendMessageToWorld("sendAll", {objName, attrName, path});
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
                    tmp.resize(256);
                    if (ImGui::InputText(attrName.c_str(), const_cast<char*>(tmp.c_str()), tmp.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {objName, attrName, tmp});
                }
            }
            break;
        }
        }

        if (ImGui::IsItemHovered())
        {
            auto answer = scene->getAttributeDescriptionFromObject(objName, attrName);
            if (answer.size() != 0)
                ImGui::SetTooltip("%s", answer[0].as<string>().c_str());
        }
    }
}

#pragma clang diagnostic pop

} // end of namespace
