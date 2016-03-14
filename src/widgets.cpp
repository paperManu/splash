#include "widgets.h"

#include <dirent.h>

#include <array>
#include <fstream>
#include <imgui.h>

#include "camera.h"
#include "image.h"
#include "image_ffmpeg.h"
#if HAVE_SHMDATA
    #include "image_shmdata.h"
#endif
#include "log.h"
#include "object.h"
#include "queue.h"
#include "osUtils.h"
#include "scene.h"
#include "texture.h"
#include "texture_image.h"
#include "timer.h"

#if HAVE_GPHOTO
#include "colorcalibrator.h"
#endif

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
GuiTextBox::GuiTextBox(string name)
    : GuiWidget(name)
{
}

/*************/
void GuiTextBox::render()
{
    if (getText)
    {
        if (ImGui::CollapsingHeader(_name.c_str()))
            ImGui::Text(getText().c_str());
    }
}

/*************/
/*************/
GuiMedia::GuiMedia(std::string name)
    : GuiWidget(name)
{
    for (auto& type : _mediaTypes)
        _mediaTypesReversed[type.second] = type.first;
}

/*************/
void GuiMedia::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto scene = _scene.lock();

        auto mediaList = getSceneMedia();
        for (auto& media : mediaList)
        {
            auto mediaName = media->getName();
            if (ImGui::TreeNode(mediaName.c_str()))
            {
                ImGui::Text("Media type: ");
                ImGui::SameLine();

                if (_mediaTypeIndex.find(mediaName) == _mediaTypeIndex.end())
                    _mediaTypeIndex[mediaName] = 0;

                vector<const char*> mediaTypes;
                for (auto& type : _mediaTypes)
                    mediaTypes.push_back(type.first.c_str());

                if (ImGui::Combo("", &_mediaTypeIndex[mediaName], mediaTypes.data(), mediaTypes.size()))
                    replaceMedia(mediaName, mediaTypes[_mediaTypeIndex[mediaName]]);

                ImGui::Text("Parameters:");
                auto attributes = media->getAttributes(true);
                for (auto& attr : attributes)
                {
                    if (attr.second.size() > 4 || attr.second.size() == 0)
                        continue;

                    if (attr.second[0].getType() == Value::Type::i
                        || attr.second[0].getType() == Value::Type::f)
                    {
                        int precision = 0;
                        if (attr.second[0].getType() == Value::Type::f)
                            precision = 2;

                        if (attr.second.size() == 1)
                        {
                            float tmp = attr.second[0].asFloat();
                            float step = attr.second[0].getType() == Value::Type::f ? 0.01 * tmp : 1.f;
                            if (ImGui::InputFloat(attr.first.c_str(), &tmp, step, step, precision, ImGuiInputTextFlags_EnterReturnsTrue))
                                scene->sendMessageToWorld("sendAll", {mediaName, attr.first, tmp});
                        }
                        else if (attr.second.size() == 2)
                        {
                            vector<float> tmp;
                            tmp.push_back(attr.second[0].asFloat());
                            tmp.push_back(attr.second[1].asFloat());
                            if (ImGui::InputFloat2(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                                scene->sendMessageToWorld("sendAll", {mediaName, attr.first, tmp[0], tmp[1]});
                        }
                        else if (attr.second.size() == 3)
                        {
                            vector<float> tmp;
                            tmp.push_back(attr.second[0].asFloat());
                            tmp.push_back(attr.second[1].asFloat());
                            tmp.push_back(attr.second[2].asFloat());
                            if (ImGui::InputFloat3(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                                scene->sendMessageToWorld("sendAll", {mediaName, attr.first, tmp[0], tmp[1], tmp[2]});
                        }
                        else if (attr.second.size() == 4)
                        {
                            vector<float> tmp;
                            tmp.push_back(attr.second[0].asFloat());
                            tmp.push_back(attr.second[1].asFloat());
                            tmp.push_back(attr.second[2].asFloat());
                            tmp.push_back(attr.second[3].asFloat());
                            if (ImGui::InputFloat4(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                                scene->sendMessageToWorld("sendAll", {mediaName, attr.first, tmp[0], tmp[1], tmp[2], tmp[3]});
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
                                ImGui::PushID((mediaName + attr.first).c_str());
                                if (ImGui::InputText("", const_cast<char*>(tmp.c_str()), tmp.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                                    scene->sendMessageToWorld("sendAll", {mediaName, attr.first, tmp});

                                ImGui::SameLine();
                                if (ImGui::Button("..."))
                                {
                                    _fileSelectorTarget = mediaName;
                                }
                                if (_fileSelectorTarget == mediaName)
                                {
                                    static string path = Utils::getPathFromFilePath("./");
                                    bool cancelled;
                                    vector<string> extensions {{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"}, {"avi"}, {"mov"}, {"mp4"}};
                                    if (SplashImGui::FileSelector(mediaName, path, cancelled, extensions))
                                    {
                                        if (!cancelled)
                                        {
                                            scene->sendMessageToWorld("sendAll", {mediaName, attr.first, path});
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
                                    scene->sendMessageToWorld("sendAll", {mediaName, attr.first, tmp});
                            }
                        }
                    }
                }


                // TODO: specific part for Queues. Need better Attributes definition to remove this
                if (dynamic_pointer_cast<QueueSurrogate>(media))
                {
                    if (ImGui::TreeNode("Playlist"))
                    {
                        auto updated = false;
                        Values playlist;

                        auto playlistIt = attributes.find("playlist");
                        if (playlistIt != attributes.end())
                        {
                            playlist = playlistIt->second;

                            // Current sources
                            int index = 0;
                            int deleteIndex = -1;
                            for (auto& source : playlist)
                            {
                                auto values = source.asValues();
                                auto idStack = to_string(index) + values[0].asString();

                                ImGui::PushID((idStack + "delete").c_str());
                                if (ImGui::Button("-"))
                                    deleteIndex = index;
                                ImGui::PopID();
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Delete this media");

                                ImGui::SameLine();
                                ImGui::PushItemWidth(96);
                                ImGui::PushID((idStack + "media_type").c_str());
                                ImGui::Text(_mediaTypesReversed[values[0].asString()].c_str());
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Media type");
                                ImGui::PopID();

                                ImGui::SameLine();
                                ImGui::PushItemWidth(96);
                                float tmp = values[2].asFloat();
                                ImGui::PushID((idStack + "start").c_str());
                                if (ImGui::InputFloat("", &tmp, 1.0f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                                {
                                    source[2] = tmp;
                                    updated = true;
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Start time (s)");
                                ImGui::PopID();

                                ImGui::SameLine();
                                ImGui::PushItemWidth(96);
                                tmp = values[3].asFloat();
                                ImGui::PushID((idStack + "stop").c_str());
                                if (ImGui::InputFloat("", &tmp, 1.0f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                                {
                                    source[3] = tmp;
                                    updated = true;
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Stop time (s)");
                                ImGui::PopID();

                                ImGui::SameLine();
                                ImGui::PushItemWidth(-0.01f);
                                ImGui::PushID((idStack + "path").c_str());
                                ImGui::Text(values[1].asString().c_str());
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Media path");
                                ImGui::PopID();
                                ImGui::PopItemWidth();

                                ++index;
                            }

                            if (deleteIndex >= 0)
                            {
                                playlist.erase(playlist.begin() + deleteIndex);
                                updated = true;
                            }
                        }

                        // Adding a source
                        ImGui::Text("Add a media:");

                        ImGui::PushID("addNewMedia");
                        if (ImGui::Button("+"))
                        {
                            playlist.push_back(_newMedia);
                            updated = true;
                        }
                        ImGui::PopID();
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Add this media");

                        int typeIndex;
                        ImGui::SameLine();
                        ImGui::PushItemWidth(96);
                        ImGui::PushID("newMediaType");
                        if (ImGui::Combo("", &_newMediaTypeIndex, mediaTypes.data(), mediaTypes.size()))
                            _newMedia[0] = _mediaTypes[mediaTypes[_newMediaTypeIndex]];
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Media type");
                        ImGui::PopID();

                        ImGui::SameLine();
                        ImGui::PushItemWidth(96);
                        ImGui::PushID("newMediaStart");
                        if (ImGui::InputFloat("", &_newMediaStart, 0.1f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                            _newMedia[2] = _newMediaStart;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Start time (s)");
                        ImGui::PopID();

                        ImGui::SameLine();
                        ImGui::PushItemWidth(96);
                        ImGui::PushID("newMediaStop");
                        if (_newMediaStop < _newMediaStart)
                            _newMediaStop = _newMediaStart;
                        if (ImGui::InputFloat("", &_newMediaStop, 0.1f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                            _newMedia[3] = _newMediaStop;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Stop time (s)");
                        ImGui::PopID();

                        string filepath = _newMedia[1].asString();
                        filepath.resize(512);
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-32.f);
                        ImGui::PushID("newMediaFile");
                        if (ImGui::InputText("", const_cast<char*>(filepath.c_str()), filepath.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                            _newMedia[1] = filepath;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Media path");

                        ImGui::PopItemWidth();

                        ImGui::SameLine();
                        if (ImGui::Button("..."))
                        {
                            _fileSelectorTarget = mediaName;
                        }
                        if (_fileSelectorTarget == mediaName)
                        {
                            static string path = Utils::getPathFromFilePath("./");
                            bool cancelled;
                            vector<string> extensions {{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"}, {"avi"}, {"mov"}, {"mp4"}};
                            if (SplashImGui::FileSelector(mediaName, path, cancelled, extensions))
                            {
                                if (!cancelled)
                                {
                                    _newMedia[1] = path;
                                    path = Utils::getPathFromFilePath("./");
                                }
                                _fileSelectorTarget = "";
                            }
                        }
                        ImGui::PopID();
                        
                        if (updated)
                        {
                            playlist.push_front("playlist");
                            playlist.push_front(mediaName);
                            scene->sendMessageToWorld("sendAll", playlist);
                        }

                        ImGui::TreePop();
                    }

                }

                ImGui::TreePop();
            }
        }
    }
}

/*************/
void GuiMedia::replaceMedia(string previousMedia, string type)
{
    auto scene = _scene.lock();

    // We get the list of all objects linked to previousMedia
    auto targetObjects = list<weak_ptr<BaseObject>>();
    for (auto& objIt : scene->_objects)
    {
        auto& object = objIt.second;
        if (!object->_savable)
            continue;
        auto linkedObjects = object->getLinkedObjects();
        for (auto& linked : linkedObjects)
            if (linked->getName() == previousMedia)
                targetObjects.push_back(object);
    }

    // Delete the media
    scene->sendMessageToWorld("deleteObject", {previousMedia});
    
    // Replace the current Image with the new one
    // TODO: this has to be done in a thread because "deleteObject" is asynched and "addObject" is synched...
    SThread::pool.enqueueWithoutId([=]() {
        this_thread::sleep_for(chrono::milliseconds(100));
        scene->sendMessageToWorld("addObject", {_mediaTypes[type], previousMedia});

        for (auto& weakObject : targetObjects)
        {
            if (weakObject.expired())
                continue;
            auto object = weakObject.lock();
            scene->sendMessageToWorld("sendAllScenes", {"link", previousMedia, object->getName()});
            scene->sendMessageToWorld("sendAllScenes", {"linkGhost", previousMedia, object->getName()});
        }
    });
}

/*************/
int GuiMedia::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    return flags;
}

/*************/
list<shared_ptr<BaseObject>> GuiMedia::getSceneMedia()
{
    auto mediaList = list<shared_ptr<BaseObject>>();
    auto scene = _scene.lock();

    for (auto& obj : scene->_objects)
    {
        if (!obj.second->_savable)
            continue;

        if (dynamic_pointer_cast<Image>(obj.second))
            mediaList.push_back(obj.second);
        else if (dynamic_pointer_cast<Texture>(obj.second))
            mediaList.push_back(obj.second);
    }

    for (auto& obj : scene->_ghostObjects)
    {
        if (!obj.second->_savable)
            continue;

        if (dynamic_pointer_cast<Image>(obj.second))
            mediaList.push_back(obj.second);
        else if (dynamic_pointer_cast<Texture>(obj.second))
            mediaList.push_back(obj.second);
    }

    return mediaList;
}

/*************/
/*************/
void GuiControl::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        // World control
        ImGui::Text("World configuration (not saved!)");
        static auto worldFramerate = 60;
        if (ImGui::InputInt("World framerate", &worldFramerate, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            worldFramerate = std::max(worldFramerate, 0);
            auto scene = _scene.lock();
            scene->sendMessageToWorld("framerate", {worldFramerate});
        }
        static auto syncTestFrameDelay = 0;
        if (ImGui::InputInt("Frames between color swap", &syncTestFrameDelay, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            syncTestFrameDelay = std::max(syncTestFrameDelay, 0);
            auto scene = _scene.lock();
            scene->sendMessageToWorld("swapTest", {syncTestFrameDelay});
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Node view
        if (!_nodeView)
        {
            auto nodeView = make_shared<GuiNodeView>("Nodes");
            nodeView->setScene(_scene);
            _nodeView = dynamic_pointer_cast<GuiWidget>(nodeView);
        }
        ImGui::Text("Configuration global view");
        _nodeView->render();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Configuration applied to multiple objects
        ImGui::Text("Global configuration (saved!)");
        static auto blendWidth = 0.05f;
        if (ImGui::InputFloat("Blending width", &blendWidth, 0.01f, 0.04f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            sendValuesToObjectsOfType("camera", "blendWidth", {blendWidth});

        static auto blendPrecision = 0.1f;
        if (ImGui::InputFloat("Blending precision", &blendPrecision, 0.01f, 0.04f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            sendValuesToObjectsOfType("camera", "blendPrecision", {blendPrecision});

        static auto blackLevel = 0.0f;
        if (ImGui::InputFloat("Black level", &blackLevel, 0.01f, 0.04f, 3, ImGuiInputTextFlags_EnterReturnsTrue))
            sendValuesToObjectsOfType("camera", "blackLevel", {blackLevel});

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Node configuration
        ImGui::Text("Objects configuration (saved!)");
        // Select the object the control
        {
            vector<string> objectNames = getObjectNames();
            vector<const char*> items;

            int index = 0;
            string clickedNode = dynamic_pointer_cast<GuiNodeView>(_nodeView)->getClickedNode(); // Used to set the object selected for configuration
            for (auto& name : objectNames)
            {
                items.push_back(name.c_str());
                // If the object name is the same as the item selected in the node view, we change the targetIndex
                if (name == clickedNode)
                    _targetIndex = index;
                index++;
            }
            ImGui::Combo("Selected object", &_targetIndex, items.data(), items.size());
        }

        // Initialize the target
        if (_targetIndex >= 0)
        {
            vector<string> objectNames = getObjectNames();
            if (objectNames.size() <= _targetIndex)
                return;
            _targetObjectName = objectNames[_targetIndex];
        }

        if (_targetObjectName == "")
            return;

        auto scene = _scene.lock();

        bool isDistant = false;
        if (scene->_ghostObjects.find(_targetObjectName) != scene->_ghostObjects.end())
            isDistant = true;

        unordered_map<string, Values> attributes;
        if (!isDistant)
            attributes = scene->_objects[_targetObjectName]->getAttributes(true);
        else
            attributes = scene->_ghostObjects[_targetObjectName]->getAttributes(true);

        for (auto& attr : attributes)
        {
            if (attr.second.size() > 4 || attr.second.size() == 0)
                continue;

            if (attr.second[0].getType() == Value::Type::i
                || attr.second[0].getType() == Value::Type::f)
            {
                int precision = 0;
                if (attr.second[0].getType() == Value::Type::f)
                    precision = 2;

                if (attr.second.size() == 1)
                {
                    float tmp = attr.second[0].asFloat();
                    float step = attr.second[0].getType() == Value::Type::f ? 0.01 * tmp : 1.f;
                    if (ImGui::InputFloat(attr.first.c_str(), &tmp, step, step, precision, ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp});
                }
                else if (attr.second.size() == 2)
                {
                    vector<float> tmp;
                    tmp.push_back(attr.second[0].asFloat());
                    tmp.push_back(attr.second[1].asFloat());
                    if (ImGui::InputFloat2(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp[0], tmp[1]});
                }
                else if (attr.second.size() == 3)
                {
                    vector<float> tmp;
                    tmp.push_back(attr.second[0].asFloat());
                    tmp.push_back(attr.second[1].asFloat());
                    tmp.push_back(attr.second[2].asFloat());
                    if (ImGui::InputFloat3(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp[0], tmp[1], tmp[2]});
                }
                else if (attr.second.size() == 4)
                {
                    vector<float> tmp;
                    tmp.push_back(attr.second[0].asFloat());
                    tmp.push_back(attr.second[1].asFloat());
                    tmp.push_back(attr.second[2].asFloat());
                    tmp.push_back(attr.second[3].asFloat());
                    if (ImGui::InputFloat4(attr.first.c_str(), tmp.data(), precision, ImGuiInputTextFlags_EnterReturnsTrue))
                        scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp[0], tmp[1], tmp[2], tmp[3]});
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
                        ImGui::PushID((_targetObjectName + attr.first).c_str());
                        if (ImGui::InputText("", const_cast<char*>(tmp.c_str()), tmp.size(), ImGuiInputTextFlags_EnterReturnsTrue))
                            scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp});

                        ImGui::SameLine();
                        if (ImGui::Button("..."))
                        {
                            _fileSelectorTarget = _targetObjectName;
                        }
                        if (_fileSelectorTarget == _targetObjectName)
                        {
                            static string path = Utils::getPathFromFilePath("./");
                            bool cancelled;
                            vector<string> extensions {{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"},
                                                       {"avi"}, {"mov"}, {"mp4"},
                                                       {"obj"}};
                            if (SplashImGui::FileSelector(_targetObjectName, path, cancelled, extensions))
                            {
                                if (!cancelled)
                                {
                                    scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, path});
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
                            scene->sendMessageToWorld("sendAll", {_targetObjectName, attr.first, tmp});
                    }
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Delete selected object"))
            scene->sendMessageToWorld("deleteObject", {_targetObjectName});
    }
}

/*************/
vector<string> GuiControl::getObjectNames()
{
    auto scene = _scene.lock();
    vector<string> objNames;

    for (auto& o : scene->_objects)
    {
        if (!o.second->_savable)
            continue;
        objNames.push_back(o.first);
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->_savable)
            continue;
        objNames.push_back(o.first);
    }

    return objNames;
}

/*************/
void GuiControl::sendValuesToObjectsOfType(string type, string attr, Values values)
{
    auto scene = _scene.lock();
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == type)
            obj.second->setAttribute(attr, values);
    
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == type)
        {
            obj.second->setAttribute(attr, values);
            scene->sendMessageToWorld("sendAll", {obj.first, attr, values});
        }
}

/*************/
int GuiControl::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    if (_nodeView)
        flags |= _nodeView->updateWindowFlags();
    return flags;
}

/*************/
/*************/
GuiGlobalView::GuiGlobalView(string name)
    : GuiWidget(name)
{
}

/*************/
void GuiGlobalView::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        if (ImGui::Button("Hide other cameras"))
            switchHideOtherCameras();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Hide all but the selected camera (H while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Button("Show targets"))
            showAllCalibrationPoints();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show the target positions for the calibration points (A while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Button("Show points everywhere"))
            showAllCamerasCalibrationPoints();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show this camera's calibration points in other cameras (O while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Button("Calibrate camera"))
            doCalibration();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Calibrate the selected camera (C while hovering the view)");
        ImGui::SameLine();

        if (ImGui::Button("Revert camera"))
            revertCalibration();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Revert the selected camera to its previous calibration (Ctrl + Z while hovering the view)");

        ImVec2 winSize = ImGui::GetWindowSize();
        double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;

        auto cameras = getCameras();
        
        ImGui::BeginChild("Cameras", ImVec2(ImGui::GetWindowWidth() * 0.25, ImGui::GetWindowWidth() * 0.67), true);
        ImGui::Text("Select a camera:");
        for (auto& camera : cameras)
        {
            camera->render();

            Values size;
            camera->getAttribute("size", size);

            int w = ImGui::GetWindowWidth() - 4 * leftMargin;
            int h = w * size[1].asInt() / size[0].asInt();

            if(ImGui::ImageButton((void*)(intptr_t)camera->getTextures()[0]->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0)))
            {
                auto scene = _scene.lock();

                // Empty previous camera parameters
                _previousCameraParameters.clear();

                // Ensure that all cameras are shown
                _camerasHidden = false;
                for (auto& cam : cameras)
                    scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 0});

                scene->sendMessageToWorld("sendAll", {_camera->getName(), "frame", 0});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "displayCalibration", 0});

                _camera = camera;

                scene->sendMessageToWorld("sendAll", {_camera->getName(), "frame", 1});
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "displayCalibration", 1});
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(camera->getName().c_str());
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("Calibration", ImVec2(0, ImGui::GetWindowWidth() * 0.67), false);
        if (_camera != nullptr)
        {
            Values size;
            _camera->getAttribute("size", size);

            int w = ImGui::GetWindowWidth() - 2 * leftMargin;
            int h = w * size[1].asInt() / size[0].asInt();

            _camWidth = w;
            _camHeight = h;

            ImGui::Text(("Current camera: " + _camera->getName()).c_str());

            ImGui::Image((void*)(intptr_t)_camera->getTextures()[0]->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));
            if (ImGui::IsItemHoveredRect())
            {
                _noMove = true;
                processKeyEvents();
                processMouseEvents();
                processJoystickState();
            }
            else
            {
                _noMove = false;
            }
        }
        ImGui::EndChild();
    }
}

/*************/
int GuiGlobalView::updateWindowFlags()
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
void GuiGlobalView::setCamera(CameraPtr cam)
{
    if (cam != nullptr)
    {
        _camera = cam;
        _guiCamera = cam;
        _camera->setAttribute("size", {800, 600});
    }
}

/*************/
void GuiGlobalView::setJoystick(const vector<float>& axes, const vector<uint8_t>& buttons)
{
    _joyAxes = axes;
    _joyButtons = buttons;
}

/*************/
vector<glm::dmat4> GuiGlobalView::getCamerasRTMatrices()
{
    auto scene = _scene.lock();
    auto rtMatrices = vector<glm::dmat4>();

    for (auto& obj : scene->_objects)
        if (obj.second->getType() == "camera")
            rtMatrices.push_back(dynamic_pointer_cast<Camera>(obj.second)->computeViewMatrix());
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == "camera")
            rtMatrices.push_back(dynamic_pointer_cast<Camera>(obj.second)->computeViewMatrix());

    return rtMatrices;
}

/*************/
void GuiGlobalView::nextCamera()
{
    auto scene = _scene.lock();
    vector<CameraPtr> cameras;
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == "camera")
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == "camera")
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));

    // Empty previous camera parameters
    _previousCameraParameters.clear();

    // Ensure that all cameras are shown
    _camerasHidden = false;
    for (auto& cam : cameras)
        scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 0});

    scene->sendMessageToWorld("sendAll", {_camera->getName(), "frame", 0});
    scene->sendMessageToWorld("sendAll", {_camera->getName(), "displayCalibration", 0});

    if (cameras.size() == 0)
        _camera = _guiCamera;
    else if (_camera == _guiCamera)
        _camera = cameras[0];
    else
    {
        for (int i = 0; i < cameras.size(); ++i)
        {
            if (cameras[i] == _camera && i == cameras.size() - 1)
            {
                _camera = _guiCamera;
                break;
            }
            else if (cameras[i] == _camera)
            {
                _camera = cameras[i + 1];
                break;
            }
        }
    }

    if (_camera != _guiCamera)
    {
        scene->sendMessageToWorld("sendAll", {_camera->getName(), "frame", 1});
        scene->sendMessageToWorld("sendAll", {_camera->getName(), "displayCalibration", 1});
    }

    return;
}

/*************/
void GuiGlobalView::revertCalibration()
{
    if (_previousCameraParameters.size() == 0)
        return;
    
    Log::get() << Log::MESSAGE << "GuiGlobalView::" << __FUNCTION__ << " - Reverting camera to previous parameters" << Log::endl;
    
    auto params = _previousCameraParameters.back();
    // We keep the very first calibration, it has proven useful
    if (_previousCameraParameters.size() > 1)
        _previousCameraParameters.pop_back();
    
    _camera->setAttribute("eye", params.eye);
    _camera->setAttribute("target", params.target);
    _camera->setAttribute("up", params.up);
    _camera->setAttribute("fov", params.fov);
    _camera->setAttribute("principalPoint", params.principalPoint);
    
    auto scene = _scene.lock();
    for (auto& obj : scene->_ghostObjects)
        if (_camera->getName() == obj.second->getName())
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "eye", params.eye[0], params.eye[1], params.eye[2]});
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "target", params.target[0], params.target[1], params.target[2]});
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "up", params.up[0], params.up[1], params.up[2]});
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "fov", params.fov[0]});
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "principalPoint", params.principalPoint[0], params.principalPoint[1]});
        }
}

/*************/
void GuiGlobalView::showAllCalibrationPoints()
{
    auto scene = _scene.lock();
    scene->sendMessageToWorld("sendAll", {_camera->getName(), "switchShowAllCalibrationPoints"});
}

/*************/
void GuiGlobalView::showAllCamerasCalibrationPoints()
{
    auto scene = _scene.lock();
    if (_camera == _guiCamera)
        _guiCamera->setAttribute("switchDisplayAllCalibration", {});
    else
        scene->sendMessageToWorld("sendAll", {_camera->getName(), "switchDisplayAllCalibration"});
}

/*************/
void GuiGlobalView::doCalibration()
{
    CameraParameters params;
     // We keep the current values
    _camera->getAttribute("eye", params.eye);
    _camera->getAttribute("target", params.target);
    _camera->getAttribute("up", params.up);
    _camera->getAttribute("fov", params.fov);
    _camera->getAttribute("principalPoint", params.principalPoint);
    _previousCameraParameters.push_back(params);

    // Calibration
    _camera->doCalibration();
    propagateCalibration();

    return;
}

/*************/
void GuiGlobalView::propagateCalibration()
{
    bool isDistant {false};
    auto scene = _scene.lock();
    for (auto& obj : scene->_ghostObjects)
        if (_camera->getName() == obj.second->getName())
            isDistant = true;
    
    if (isDistant)
    {
        vector<string> properties {"eye", "target", "up", "fov", "principalPoint"};
        auto scene = _scene.lock();
        for (auto& p : properties)
        {
            Values values;
            _camera->getAttribute(p, values);

            Values sendValues {_camera->getName(), p};
            for (auto& v : values)
                sendValues.push_back(v);

            scene->sendMessageToWorld("sendAll", sendValues);
        }
    }
}

/*************/
void GuiGlobalView::switchHideOtherCameras()
{
    auto scene = _scene.lock();
    vector<CameraPtr> cameras;
    for (auto& obj : scene->_objects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& obj : scene->_ghostObjects)
        if (dynamic_pointer_cast<Camera>(obj.second).get() != nullptr)
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));

    if (!_camerasHidden)
    {
        for (auto& cam : cameras)
            if (cam.get() != _camera.get())
                scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 1});
        _camerasHidden = true;
    }
    else
    {
        for (auto& cam : cameras)
            if (cam.get() != _camera.get())
                scene->sendMessageToWorld("sendAll", {cam->getName(), "hide", 0});
        _camerasHidden = false;
    }
}

/*************/
void GuiGlobalView::processJoystickState()
{
    auto scene = _scene.lock();

    float speed = 1.f;

    // Buttons
    if (_joyButtons.size() >= 4)
    {
        if (_joyButtons[0] == 1 && _joyButtons[0] != _joyButtonsPrevious[0])
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "selectPreviousCalibrationPoint"});
        }
        else if (_joyButtons[1] == 1 && _joyButtons[1] != _joyButtonsPrevious[1])
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "selectNextCalibrationPoint"});
        }
        else if (_joyButtons[2] == 1)
        {
            speed = 10.f;
        }
        else if (_joyButtons[3] == 1 && _joyButtons[3] != _joyButtonsPrevious[3])
        {
            doCalibration();
        }
    }
    if (_joyButtons.size() >= 6)
    {
        if (_joyButtons[4] == 1 && _joyButtons[4] != _joyButtonsPrevious[4])
        {
            showAllCalibrationPoints();
        }
        else if (_joyButtons[5] == 1 && _joyButtons[5] != _joyButtonsPrevious[5])
        {
            switchHideOtherCameras();
        }
    }

    _joyButtonsPrevious = _joyButtons;

    // Axes
    if (_joyAxes.size() >= 2)
    {
        float xValue = _joyAxes[0];
        float yValue = -_joyAxes[1]; // Y axis goes downward for joysticks...

        if (xValue != 0.f || yValue != 0.f)
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", xValue * speed, yValue * speed});
            _camera->moveCalibrationPoint(0.0, 0.0);
            propagateCalibration();
        }
    }

    // This prevents issues when disconnecting the joystick
    _joyAxes.clear();
    _joyButtons.clear();
}

/*************/
void GuiGlobalView::processKeyEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeysDown[' '] && io.KeysDownDuration[' '] == 0.0)
    {
        nextCamera();
        return;
    }
    else if (io.KeysDown['A'] && io.KeysDownDuration['A'] == 0.0)
    {
        showAllCalibrationPoints();
        return;
    }
    else if (io.KeysDown['C'] && io.KeysDownDuration['C'] == 0.0)
    {
        doCalibration();
        return;
    }
    else if (io.KeysDown['H'] && io.KeysDownDuration['H'] == 0.0)
    {
        switchHideOtherCameras();
        return;
    }
    else if (io.KeysDown['O'] && io.KeysDownDuration['O'] == 0.0)
    {
        showAllCamerasCalibrationPoints();
        return;
    }
    // Reset to the previous camera calibration
    else if (io.KeysDown['Z'] && io.KeysDownDuration['Z'] == 0.0)
    {
        if (io.KeyCtrl)
            revertCalibration();
        return;
    }
    // Arrow keys
    else
    {
        auto scene = _scene.lock();

        float delta = 1.f;
        if (io.KeyShift)
            delta = 0.1f;
        else if (io.KeyCtrl)
            delta = 10.f;
            
        if (io.KeysDown[262])
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", delta, 0});
            _camera->moveCalibrationPoint(0.0, 0.0);
            propagateCalibration();
        }
        if (io.KeysDown[263])
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", -delta, 0});
            _camera->moveCalibrationPoint(0.0, 0.0);
            propagateCalibration();
        }
        if (io.KeysDown[264])
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", 0, -delta});
            _camera->moveCalibrationPoint(0.0, 0.0);
            propagateCalibration();
        }
        if (io.KeysDown[265])
        {
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "moveCalibrationPoint", 0, delta});
            _camera->moveCalibrationPoint(0.0, 0.0);
            propagateCalibration();
        }

        return;
    }
}

/*************/
void GuiGlobalView::processMouseEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    // Get mouse pos
    ImVec2 mousePos = ImVec2((io.MousePos.x - ImGui::GetCursorScreenPos().x) / _camWidth,
                             -(io.MousePos.y - ImGui::GetCursorScreenPos().y) / _camHeight);

    if (io.MouseDown[0])
    {
        // If selected camera is guiCamera, do nothing
        if (_camera == _guiCamera)
            return;

        // Set a calibration point
        auto scene = _scene.lock();
        if (io.KeyCtrl && io.MouseClicked[0])
        {
            auto scene = _scene.lock();
            Values position = _camera->pickCalibrationPoint(mousePos.x, mousePos.y);
            if (position.size() == 3)
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "removeCalibrationPoint", position[0], position[1], position[2]});
        }
        else if (io.KeyShift) // Define the screenpoint corresponding to the selected calibration point
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "setCalibrationPoint", mousePos.x * 2.f - 1.f, mousePos.y * 2.f - 1.f});
        else if (io.MouseClicked[0]) // Add a new calibration point
        {
            Values position = _camera->pickVertexOrCalibrationPoint(mousePos.x, mousePos.y);
            if (position.size() == 3)
            {
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "addCalibrationPoint", position[0], position[1], position[2]});
                _previousPointAdded = position;
            }
            else
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "deselectCalibrationPoint"});
        }
        return;
    }
    if (io.MouseClicked[1])
    {
        float fragDepth = 0.f;
        _newTarget = _camera->pickFragment(mousePos.x, mousePos.y, fragDepth);

        if (fragDepth == 0.f)
            _newTargetDistance = 1.f;
        else
            _newTargetDistance = -fragDepth * 0.1f;
    }
    if (io.MouseDownDuration[1] > 0.0) 
    {
        // Move the camera
        if (!io.KeyCtrl && !io.KeyShift)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;
            auto scene = _scene.lock();

            if (_camera != _guiCamera)
            {
                if (_newTarget.size() == 3)
                    scene->sendMessageToWorld("sendAll", {_camera->getName(), "rotateAroundPoint", dx / 100.f, dy / 100.f, 0, _newTarget[0].asFloat(), _newTarget[1].asFloat(), _newTarget[2].asFloat()});
                else
                    scene->sendMessageToWorld("sendAll", {_camera->getName(), "rotateAroundTarget", dx / 100.f, dy / 100.f, 0});
            }
            else
            {
                if (_newTarget.size() == 3)
                    _camera->setAttribute("rotateAroundPoint", {dx / 100.f, dy / 100.f, 0, _newTarget[0].asFloat(), _newTarget[1].asFloat(), _newTarget[2].asFloat()});
                else
                    _camera->setAttribute("rotateAroundTarget", {dx / 100.f, dy / 100.f, 0});
            }
        }
        // Move the target and the camera (in the camera plane)
        else if (io.KeyShift && !io.KeyCtrl)
        {
            float dx = io.MouseDelta.x * _newTargetDistance;
            float dy = io.MouseDelta.y * _newTargetDistance;
            auto scene = _scene.lock();
            if (_camera != _guiCamera)
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "pan", -dx / 100.f, dy / 100.f, 0.f});
            else
                _camera->setAttribute("pan", {-dx / 100.f, dy / 100.f, 0});
        }
        else if (!io.KeyShift && io.KeyCtrl)
        {
            float dy = io.MouseDelta.y * _newTargetDistance / 100.f;
            auto scene = _scene.lock();
            if (_camera != _guiCamera)
                scene->sendMessageToWorld("sendAll", {_camera->getName(), "forward", dy});
            else
                _camera->setAttribute("forward", {dy});
        }
    }
    if (io.MouseWheel != 0)
    {
        Values fov;
        _camera->getAttribute("fov", fov);
        float camFov = fov[0].asFloat();

        camFov += io.MouseWheel;
        camFov = std::max(2.f, std::min(180.f, camFov));

        auto scene = _scene.lock();
        if (_camera != _guiCamera)
            scene->sendMessageToWorld("sendAll", {_camera->getName(), "fov", camFov});
        else
            _camera->setAttribute("fov", {camFov});
    }
}

/*************/
vector<shared_ptr<Camera>> GuiGlobalView::getCameras()
{
    auto cameras = vector<shared_ptr<Camera>>();

    _guiCamera->setAttribute("size", {ImGui::GetWindowWidth(), ImGui::GetWindowWidth() * 3 / 4});

    auto rtMatrices = getCamerasRTMatrices();
    for (auto& matrix : rtMatrices)
        _guiCamera->drawModelOnce("camera", matrix);
    cameras.push_back(_guiCamera);

    auto scene = _scene.lock();
    for (auto& obj : scene->_objects)
        if (obj.second->getType() == "camera")
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));
    for (auto& obj : scene->_ghostObjects)
        if (obj.second->getType() == "camera")
            cameras.push_back(dynamic_pointer_cast<Camera>(obj.second));

    return cameras;
}

/*************/
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
            float maxValue {0.f};
            vector<float> values;
            for (auto& v : duration.second)
            {
                maxValue = std::max((float)v * 0.001f, maxValue);
                values.push_back((float)v * 0.001f);
            }

            maxValue = ceil(maxValue * 0.1f) * 10.f;

            ImGui::PlotLines("", values.data(), values.size(), values.size(), (duration.first + " - " + to_string((int)maxValue) + "ms").c_str(), 0.f, maxValue, ImVec2(width - 30, 80));
        }
    }
}

/*************/
void GuiTemplate::render()
{
    if (!_templatesLoaded)
    {
        loadTemplates();
        _templatesLoaded = true;
    }

    if (_textures.size() == 0)
        return;

    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        bool firstTemplate = true;
        for (auto& name : _names)
        {
            if (!firstTemplate)
                ImGui::SameLine(0.f, 2.f);
            firstTemplate = false;

            if (ImGui::ImageButton((void*)(intptr_t)_textures[name]->getTexId(), ImVec2(128, 128)))
            {
                string configPath = string(DATADIR) + "templates/" + name + ".json";
#if HAVE_OSX
                if (!ifstream(configPath, ios::in | ios::binary))
                    configPath = "../Resources/templates/" + name + ".json";
#endif
                auto scene = _scene.lock();
                scene->sendMessageToWorld("loadConfig", {configPath});
            }

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(_descriptions[name].data());
        }
    }
}

/*************/
void GuiTemplate::loadTemplates()
{
    auto templatePath = string(DATADIR);
    auto examples = vector<string>();
    auto descriptions = vector<string>();
    
    // Try to read the template file
    ifstream in(templatePath + "templates.txt", ios::in | ios::binary);
#if HAVE_OSX
    if (!in)
    {
        templatePath = "../Resources/";
        in = ifstream(templatePath + "templates.txt", ios::in | ios::binary);
    }
#endif
    if (in)
    {
        auto newTemplate = false;
        auto templateName = string();
        auto templateDescription = string();
        auto endTemplate = false;

        for (array<char, 256> line; in.getline(&line[0], 256);)
        {
            auto strLine = string(line.data());
            if (!newTemplate)
            {
                if (strLine == "{")
                {
                    newTemplate = true;
                    endTemplate = false;
                    templateName = "";
                    templateDescription = "";
                }
            }
            else
            {
                if (strLine == "}")
                    endTemplate = true;
                else if (templateName == "")
                    templateName = strLine;
                else
                    templateDescription = strLine;
            }

            if (endTemplate)
            {
                examples.push_back(templateName);
                descriptions.push_back(templateDescription);
                templateName = "";
                templateDescription = "";
                newTemplate = false;
            }
        }
    }
    else
    {
        Log::get() << Log::WARNING << "GuiTemplate::" << __FUNCTION__ << " - Could not load the templates file list in " << templatePath << "templates.txt" << Log::endl;
        return;
    }

    _textures.clear();
    _descriptions.clear();

    for (unsigned int i = 0; i < examples.size(); ++i)
    {
        auto& example = examples[i];
        auto& description = descriptions[i];

        glGetError();
        auto image = make_shared<Image>();
        image->setName("template_" + example);
        if (!image->read(templatePath + "templates/" + example + ".png"))
            continue;

        auto texture = make_shared<Texture_Image>();
        texture->linkTo(image);
        texture->update();
        texture->flushPbo();

        _names.push_back(example);
        _textures[example] = texture;
        _descriptions[example] = description;
    }
}

/*************/
map<string, vector<string>> GuiNodeView::getObjectLinks()
{
    auto scene = _scene.lock();

    auto links = map<string, vector<string>>();

    for (auto& o : scene->_objects)
    {
        if (!o.second->_savable)
            continue;
        links[o.first] = vector<string>();
        auto linkedObjects = o.second->getLinkedObjects();
        for (auto& link : linkedObjects)
            links[o.first].push_back(link->getName());
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->_savable)
            continue;
        links[o.first] = vector<string>();
        auto linkedObjects = o.second->getLinkedObjects();
        for (auto& link : linkedObjects)
            links[o.first].push_back(link->getName());
    }

    return links;
}

/*************/
map<string, string> GuiNodeView::getObjectTypes()
{
    auto scene = _scene.lock();

    auto types = map<string, string>();

    for (auto& o : scene->_objects)
    {
        if (!o.second->_savable)
            continue;
        types[o.first] = o.second->getType();
    }
    for (auto& o : scene->_ghostObjects)
    {
        if (!o.second->_savable)
            continue;
        types[o.first] = o.second->getType();
    }

    return types;
}

/*************/
void GuiNodeView::render()
{
    _clickedNode = "";

    //if (ImGui::CollapsingHeader(_name.c_str()))
    if (true)
    {
        // This defines the default positions for various node types
        static auto defaultPositionByType = map<string, ImVec2>({{"default", {8, 8}},
                                                                 {"window", {8, 32}},
                                                                 {"camera", {32, 64}},
                                                                 {"object", {8, 96}},
                                                                 {"texture filter queue", {32, 128}},
                                                                 {"image", {8, 160}},
                                                                 {"mesh", {32, 192}}
                                                                });
        std::map<std::string, int> shiftByType;

        // Begin a subwindow to enclose nodes
        ImGui::BeginChild("NodeView", ImVec2(_viewSize[0], _viewSize[1]), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // Get objects and their relations
        auto objectLinks = getObjectLinks();
        auto objectTypes = getObjectTypes();

        // Objects useful for drawing
        auto drawList = ImGui::GetWindowDrawList();
        auto canvasPos = ImGui::GetCursorScreenPos();
        // Apply view shift
        canvasPos.x += _viewShift[0];
        canvasPos.y += _viewShift[1];

        // Draw objects
        for (auto& object : objectTypes)
        {
            auto& name = object.first;
            auto& type = object.second;
            type = type.substr(0, type.find("_"));

            // We keep count of the number of objects by type, more precisely their shifting
            auto shiftIt = shiftByType.find(type);
            if (shiftIt == shiftByType.end())
                shiftByType[type] = 0;
            auto shift = shiftByType[type];

            // Get the default position for the given type
            auto nodePosition = ImVec2(-1, -1);

            string defaultPosName = "default";
            for (auto& position : defaultPositionByType)
            {
                if (position.first.find(type) != string::npos)
                    defaultPosName = position.first;
            }

            if (defaultPosName == "default")
            {
                type = "default";
                nodePosition = defaultPositionByType["default"];
            }
            else
            {
                nodePosition = defaultPositionByType[defaultPosName];
                nodePosition.x += shift;
            }
            
            ImGui::SetCursorPos(nodePosition);
            renderNode(name);

            shiftByType[type] += _nodeSize[0] + 8;
        }

        // Draw lines
        for (auto& object : objectLinks)
        {
            auto& name = object.first;
            auto& links = object.second;

            auto& currentNodePos = _nodePositions[name];
            auto firstPoint = ImVec2(currentNodePos[0] + canvasPos.x, currentNodePos[1] + canvasPos.y);

            for (auto& target : links)
            {
                auto targetIt = _nodePositions.find(target);
                if (targetIt == _nodePositions.end())
                    continue;

                auto& targetPos = targetIt->second;
                auto secondPoint = ImVec2(targetPos[0] + canvasPos.x, targetPos[1] + canvasPos.y);

                drawList->AddLine(firstPoint, secondPoint, 0xBB0088FF, 2.f);
            }
        }

        // end of the subwindow
        ImGui::EndChild();

        // Interactions
        if (ImGui::IsItemHovered())
        {
            _isHovered = true;

            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseDownDuration[0] > 0.0)
            {
                float dx = io.MouseDelta.x;
                float dy = io.MouseDelta.y;
                _viewShift[0] += dx;
                _viewShift[1] += dy;
            }
        }
        else
            _isHovered = false;
    }

    // Combo box for adding objects
    {
        vector<string> typeNames = {"image", "image_ffmpeg", "image_shmdata",
                                    "texture_syphon",
                                    "mesh", "mesh_shmdata",
                                    "camera", "window"};
        vector<const char*> items;
        for (auto& typeName : typeNames)
            items.push_back(typeName.c_str());
        static int itemIndex = 0;
        if (ImGui::Combo("Add an object", &itemIndex, items.data(), items.size()))
        {
            _scene.lock()->sendMessageToWorld("addObject", {typeNames[itemIndex]});
        }
    }
}

/*************/
void GuiNodeView::renderNode(string name)
{
    auto& io = ImGui::GetIO();

    auto pos = _nodePositions.find(name);
    if (pos == _nodePositions.end())
    {
        auto cursorPos = ImGui::GetCursorPos();
        _nodePositions[name] = vector<float>({cursorPos.x + _viewShift[0], cursorPos.y + _viewShift[1]});
    }
    else
    {
        auto& nodePos = (*pos).second;
        ImGui::SetCursorPos(ImVec2(nodePos[0] + _viewShift[0], nodePos[1] + _viewShift[1]));
    }

    // Beginning of node rendering
    ImGui::BeginChild(string("node_" + name).c_str(), ImVec2(_nodeSize[0], _nodeSize[1]), false);

    ImGui::SetCursorPos(ImVec2(0, 2));
    
    if (ImGui::IsItemHovered())
    {
        // Object linking
        if (io.MouseClicked[0])
        {
            if (io.KeyShift)
            {
                auto scene = _scene.lock();
                scene->sendMessageToWorld("sendAllScenes", {"link", _sourceNode, name});
                scene->sendMessageToWorld("sendAllScenes", {"linkGhost", _sourceNode, name});
            }
            else if (io.KeyCtrl)
            {
                auto scene = _scene.lock();
                scene->sendMessageToWorld("sendAllScenes", {"unlink", _sourceNode, name});
                scene->sendMessageToWorld("sendAllScenes", {"unlinklinkGhost", _sourceNode, name});
            }
        }
        // Dragging
        if (io.MouseDownDuration[0] > 0.0)
        {
            float dx = io.MouseDelta.x;
            float dy = io.MouseDelta.y;
            auto& nodePos = (*pos).second;
            nodePos[0] += dx;
            nodePos[1] += dy;
        }
    }

    // This tricks allows for detecting clicks on the node, while keeping it closed
    ImGui::SetNextTreeNodeOpened(false);
    if (ImGui::CollapsingHeader(name.c_str()))
    {
        _clickedNode = name;
        _sourceNode = name;
    }
    
    // End of node rendering
    ImGui::EndChild();
}

/*************/
int GuiNodeView::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    if (_isHovered)
    {
        flags |= ImGuiWindowFlags_NoMove;
    }
    return flags;
}

} // end of namespace
