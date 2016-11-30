#include "./widget_media.h"

#include <imgui.h>

#include "./osUtils.h"
#include "./queue.h"
#include "./scene.h"

using namespace std;

namespace Splash
{

/*************/
GuiMedia::GuiMedia(weak_ptr<Scene> scene, string name)
    : GuiWidget(scene, name)
{
    auto types = getTypesFromCategory(BaseObject::Category::IMAGE);
    for (auto& type : types)
        _mediaTypes[getShortDescription(type)] = type;

    for (auto& type : _mediaTypes)
        _mediaTypesReversed[type.second] = type.first;
}

/*************/
void GuiMedia::render()
{
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto mediaList = getSceneMedia();
        for (auto& media : mediaList)
        {
            auto mediaName = media->getName();
            if (ImGui::TreeNode(mediaName.c_str()))
            {
                ImGui::Text("Change media type: ");
                ImGui::SameLine();

                if (_mediaTypeIndex.find(mediaName) == _mediaTypeIndex.end())
                    _mediaTypeIndex[mediaName] = 0;

                vector<const char*> mediaTypes;
                for (auto& type : _mediaTypes)
                    mediaTypes.push_back(type.first.c_str());

                if (ImGui::Combo("", &_mediaTypeIndex[mediaName], mediaTypes.data(), mediaTypes.size()))
                    replaceMedia(mediaName, mediaTypes[_mediaTypeIndex[mediaName]]);

                ImGui::Text(("Current media type: " + _mediaTypesReversed[media->getRemoteType()]).c_str());

                ImGui::Text("Parameters:");
                auto attributes = media->getAttributes(true);
                drawAttributes(mediaName, attributes);

                // TODO: specific part for Queues. Need better Attributes definition to remove this
                // Display the playlist if this is a queue
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
                                auto values = source.as<Values>();
                                auto idStack = to_string(index) + values[0].as<string>();

                                ImGui::PushID((idStack + "delete").c_str());
                                if (ImGui::Button("-"))
                                    deleteIndex = index;
                                ImGui::PopID();
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Delete this media");

                                ImGui::SameLine();
                                ImGui::PushItemWidth(96);
                                ImGui::PushID((idStack + "media_type").c_str());
                                ImGui::Text(_mediaTypesReversed[values[0].as<string>()].c_str());
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Media type");
                                ImGui::PopID();
                                ImGui::PopItemWidth();

                                ImGui::SameLine();
                                ImGui::PushItemWidth(96);
                                float tmp = values[2].as<float>();
                                ImGui::PushID((idStack + "start").c_str());
                                if (ImGui::InputFloat("", &tmp, 1.0f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                                {
                                    source[2] = tmp;
                                    updated = true;
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Start time (s)");
                                ImGui::PopID();
                                ImGui::PopItemWidth();

                                ImGui::SameLine();
                                ImGui::PushItemWidth(96);
                                tmp = values[3].as<float>();
                                ImGui::PushID((idStack + "stop").c_str());
                                if (ImGui::InputFloat("", &tmp, 1.0f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                                {
                                    source[3] = tmp;
                                    updated = true;
                                }
                                if (ImGui::IsItemHovered())
                                    ImGui::SetTooltip("Stop time (s)");
                                ImGui::PopID();
                                ImGui::PopItemWidth();

                                ImGui::SameLine();
                                ImGui::PushItemWidth(-0.01f);
                                ImGui::PushID((idStack + "path").c_str());
                                ImGui::Text(values[1].as<string>().c_str());
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
                        ImGui::PopItemWidth();

                        ImGui::SameLine();
                        ImGui::PushItemWidth(96);
                        ImGui::PushID("newMediaStart");
                        if (ImGui::InputFloat("", &_newMediaStart, 0.1f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                            _newMedia[2] = _newMediaStart;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Start time (s)");
                        ImGui::PopID();
                        ImGui::PopItemWidth();

                        ImGui::SameLine();
                        ImGui::PushItemWidth(96);
                        ImGui::PushID("newMediaStop");
                        if (ImGui::InputFloat("", &_newMediaStop, 0.1f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                            _newMedia[3] = _newMediaStop;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Stop time (s)");
                        ImGui::PopID();
                        ImGui::PopItemWidth();

                        string filepath = _newMedia[1].as<string>();
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
                            static string path = _root.lock()->getMediaPath();
                            bool cancelled;
                            vector<string> extensions{{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"}, {"avi"}, {"mov"}, {"mp4"}};
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
                            setObject(mediaName, "playlist", playlist);
                        }

                        ImGui::TreePop();
                    }

                    // Display the filters associated with this queue
                    auto filter = dynamic_pointer_cast<QueueSurrogate>(media)->getFilter();
                    auto filterName = filter->getName();
                    if (ImGui::TreeNode(("Filter: " + filterName).c_str()))
                    {
                        auto filterAttributes = filter->getAttributes(true);
                        drawAttributes(filterName, filterAttributes);
                        ImGui::TreePop();
                    }
                }
                else
                {
                    // Display the filters associated with this media
                    auto filters = getFiltersForImage(media);
                    for (auto& filter : filters)
                    {
                        auto filterName = filter->getName();
                        if (ImGui::TreeNode(("Filter: " + filterName).c_str()))
                        {
                            auto filterAttributes = filter->getAttributes(true);
                            drawAttributes(filterName, filterAttributes);
                            ImGui::TreePop();
                        }
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
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());

    // We get the list of all objects linked to previousMedia
    auto targetObjects = list<weak_ptr<BaseObject>>();
    for (auto& objIt : scene->_objects)
    {
        auto& object = objIt.second;
        if (!object->getSavable())
            continue;
        auto linkedObjects = object->getLinkedObjects();
        for (auto& linked : linkedObjects)
            if (linked->getName() == previousMedia)
                targetObjects.push_back(object);
    }

    Values msg;
    msg.push_back(previousMedia);
    msg.push_back(_mediaTypes[type]);
    for (const auto& weakObject : targetObjects)
    {
        if (weakObject.expired())
            continue;
        auto object = weakObject.lock();
        msg.push_back(object->getName());
    }

    setGlobal("replaceObject", msg);
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
    auto mediaTypes = list<string>({"image", "queue", "texture"});

    for (auto& type : mediaTypes)
    {
        auto objects = getObjectsOfType(type);
        for (auto& object : objects)
            if (object->getSavable())
                mediaList.push_back(object);
    }

    return mediaList;
}

/*************/
list<shared_ptr<BaseObject>> GuiMedia::getFiltersForImage(const shared_ptr<BaseObject>& image)
{
    auto filterList = list<shared_ptr<BaseObject>>();
    auto scene = dynamic_pointer_cast<Scene>(_root.lock());

    for (auto& obj : scene->_objects)
    {
        if (obj.second->getType() != "filter")
            continue;

        auto linkedImages = obj.second->getLinkedObjects();
        auto matchingImage = find(linkedImages.begin(), linkedImages.end(), image);

        if (matchingImage != linkedImages.end())
            filterList.push_back(obj.second);
    }

    return filterList;
}

} // end of namespace
