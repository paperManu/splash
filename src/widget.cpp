#include "widget.h"

#include <dirent.h>

#include <array>
#include <fstream>
#include <imgui.h>

#include "./camera.h"
#include "./image.h"
#include "./image_ffmpeg.h"
#if HAVE_SHMDATA
    #include "./image_shmdata.h"
#endif
#include "./log.h"
#include "./object.h"
#include "./osUtils.h"
#include "./scene.h"
#include "./texture.h"
#include "./texture_image.h"
#include "./timer.h"

#if HAVE_GPHOTO
#include "./colorcalibrator.h"
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
            vector<FilesystemFile> files {};

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
            std::sort(files.begin(), files.end(), [](FilesystemFile a, FilesystemFile b) {
                return a.filename < b.filename;
            });

            // But we put directories first
            std::copy_if(files.begin(), files.end(), std::back_inserter(list), [](FilesystemFile p) {
                return p.isDir;
            });

            std::copy_if(files.begin(), files.end(), std::back_inserter(list), [](FilesystemFile p) {
                return !p.isDir;
            });

            // Filter files based on extension
            if (extensions.size() != 0)
            {
                list.erase(std::remove_if(list.begin(), list.end(), [&extensions](FilesystemFile p) {
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
                }), list.end());
            }
        }

        return isDirectoryPath;
    }

    /*********/
    bool FileSelector(const string& label, string& path, bool& cancelled, const vector<string>& extensions, bool showNormalFiles)
    {
        static bool filterExtension = true;;
        bool manualPath = false;
        bool selectionDone = false;
        cancelled = false;

        ImGui::PushID(label.c_str());

        string windowName = "Select file path";
        if (label.size() != 0)
            windowName += " - " + label;

        ImGui::Begin(windowName.c_str(), nullptr, ImVec2(400, 600), 0.95f);
        char textBuffer[512];
        strcpy(textBuffer, path.c_str());

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
        static int selectedId = 0;
        for (int i = 0; i < fileList.size(); ++i)
        {
            bool isSelected = (selectedId == i);

            auto filename = fileList[i].filename;
            if (fileList[i].isDir)
                filename += "/";

            if (ImGui::Selectable(filename.c_str(), isSelected))
            {
                selectedId = i;
                manualPath = false;
            }
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            path = path + "/" + fileList[selectedId].filename;
            if (!FileSelectorParseDir(path, fileList, extensions, showNormalFiles))
                selectionDone = true;
        }
        ImGui::EndChild();

        ImGui::Checkbox("Filter files", &filterExtension);

        if (ImGui::Button("Select path"))
        {
            if (!manualPath)
                path = path + "/" + fileList[selectedId].filename;
            selectionDone = true;;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            cancelled = true;;

        ImGui::End();

        ImGui::PopID();

        if (selectionDone || cancelled)
            return true;

        return false;
    }
}

/*************/
/*************/
GuiWidget::GuiWidget(string name)
{
    _name = name;
}

/*************/
void GuiWidget::drawAttributes(const string& objName, const unordered_map<string, Values>& attributes)
{
    auto scene = _scene.lock();

    for (auto& attr : attributes)
    {
        if (attr.second.size() > 4 || attr.second.size() == 0)
            continue;
    
        if (attr.second[0].getType() == Value::Type::i
            || attr.second[0].getType() == Value::Type::f)
        {
            int precision = 0;
            if (attr.second[0].getType() == Value::Type::f)
                precision = 3;
    
            if (attr.second.size() == 1)
            {
                float tmp = attr.second[0].asFloat();
                float step = attr.second[0].getType() == Value::Type::f ? 0.01 * tmp : 1.f;
                if (ImGui::InputFloat(attr.first.c_str(), &tmp, step, step, precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attr.first, tmp});
            }
            else if (attr.second.size() == 2)
            {
                vector<float> tmp;
                tmp.push_back(attr.second[0].asFloat());
                tmp.push_back(attr.second[1].asFloat());
                if (ImGui::InputFloat2(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attr.first, tmp[0], tmp[1]});
            }
            else if (attr.second.size() == 3)
            {
                vector<float> tmp;
                tmp.push_back(attr.second[0].asFloat());
                tmp.push_back(attr.second[1].asFloat());
                tmp.push_back(attr.second[2].asFloat());
                if (ImGui::InputFloat3(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attr.first, tmp[0], tmp[1], tmp[2]});
            }
            else if (attr.second.size() == 4)
            {
                vector<float> tmp;
                tmp.push_back(attr.second[0].asFloat());
                tmp.push_back(attr.second[1].asFloat());
                tmp.push_back(attr.second[2].asFloat());
                tmp.push_back(attr.second[3].asFloat());
                if (ImGui::InputFloat4(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                    scene->sendMessageToWorld("sendAll", {objName, attr.first, tmp[0], tmp[1], tmp[2], tmp[3]});
            }
        }
        else if (attr.second.size() == 1 && attr.second[0].getType() == Value::Type::v)
        {
            // We skip anything that looks like a vector / matrix
            // (for usefulness reasons...)
            Values values = attr.second[0].asValues();
            if (values.size() > 16)
            {
                if (values[0].getType() == Value::Type::i || values[0].getType() == Value::Type::f)
                {
                    float minValue = numeric_limits<float>::max();
                    float maxValue = numeric_limits<float>::min();
                    vector<float> samples;
                    for (auto& v : values)
                    {
                        float value = v.asFloat();
                        maxValue = std::max(value, maxValue);
                        minValue = std::min(value, minValue);
                        samples.push_back(value);
                    }
                    
                    ImGui::PlotLines(attr.first.c_str(), samples.data(), samples.size(), samples.size(), ("[" + to_string(minValue) + ", " + to_string(maxValue) + "]").c_str(), minValue, maxValue, ImVec2(0, 100));
                }
            }
        }
        else if (attr.second[0].getType() == Value::Type::s)
        {
            for (auto& v : attr.second)
            {
                // We have a special way to handle file paths
                if (attr.first == "file")
                {
                    string tmp = v.asString();
                    tmp.resize(512);
                    ImGui::PushID((objName + attr.first).c_str());
                    if (ImGui::InputText("", const_cast<char*>(tmp.c_str()), tmp.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {objName, attr.first, tmp});
    
                    ImGui::SameLine();
                    if (ImGui::Button("..."))
                    {
                        _fileSelectorTarget = objName;
                    }
                    if (_fileSelectorTarget == objName)
                    {
                        static string path = Utils::getPathFromFilePath("./");
                        bool cancelled;
                        vector<string> extensions {{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"},
                                                   {"avi"}, {"mov"}, {"mp4"},
                                                   {"obj"}};
                        if (SplashImGui::FileSelector(objName, path, cancelled, extensions))
                        {
                            if (!cancelled)
                            {
                                scene->sendMessageToWorld("sendAll", {objName, attr.first, path});
                                path = Utils::getPathFromFilePath("./");
                            }
                            _fileSelectorTarget = "";
                        }
                    }
    
                    ImGui::PopID();
                }
                // For everything else ...
                else
                {
                    string tmp = v.asString();
                    tmp.resize(256);
                    if (ImGui::InputText(attr.first.c_str(), const_cast<char*>(tmp.c_str()), tmp.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {objName, attr.first, tmp});
                }
            }
        }

        if (ImGui::IsItemHovered())
        {
            auto answer = scene->getAttributeDescriptionFromObject(objName, attr.first);
            if (answer.size() != 0)
                ImGui::SetTooltip(answer[0].asString().c_str());
        }
    }
}

#pragma clang diagnostic pop

} // end of namespace
