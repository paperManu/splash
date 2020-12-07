#include "./controller/widget/widget_media.h"

#include <imgui.h>

#include "./core/scene.h"
#include "./image/queue.h"
#include "./utils/osutils.h"

namespace Splash
{

/*************/
GuiMedia::GuiMedia(Scene* scene, const std::string& name)
    : GuiWidget(scene, name)
{
    auto types = getTypesFromCategory(GraphObject::Category::IMAGE);
    for (const auto& type : types)
        _mediaTypes[getShortDescription(type)] = type;

    for (const auto& type : _mediaTypes)
        _mediaTypesReversed[type.second] = type.first;

    // Make sure that the default new media for queues is set to the one
    // specified by _newMediaTypeIndex, otherwise it would be an empty string
    _newMedia[_newMediaTypeIndex] = _mediaTypes.begin()->second;
}

/*************/
void GuiMedia::render()
{
    auto mediaList = getSceneMedia();

    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##medias", ImVec2(availableSize.x * 0.25, availableSize.y), true);
    ImGui::Text("Media list");

    auto leftMargin = static_cast<int>(ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x);
    for (const auto& media : mediaList)
    {
        auto filters = getFiltersForImage(media);
        std::shared_ptr<Filter> filter;
        if (!filters.empty() && (filter = std::dynamic_pointer_cast<Filter>(filters.front())))
        {
            auto spec = filter->getSpec();

            int w = ImGui::GetWindowWidth() - 3 * leftMargin;
            int h = w * spec.height / spec.width;

            if (ImGui::ImageButton((void*)(intptr_t)filter->getTexId(), ImVec2(w, h)))
                _selectedMediaName = media->getName();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", media->getName().c_str());
        }
        else
        {
            // If the media is not connected to anything, we need to
            // draw a mere button with its name to access it without a preview
            int w = ImGui::GetWindowWidth() - 2 * leftMargin;

            if (ImGui::Button(media->getName().c_str(), ImVec2(w, w)))
                _selectedMediaName = media->getName();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", media->getName().c_str());
        }
    }
    ImGui::EndChild();

    auto media = getObjectPtr(_selectedMediaName);
    if (!media)
        return;

    auto mediaAlias = getObjectAlias(_selectedMediaName);

    ImGui::SameLine();
    ImGui::BeginChild("##mediaInfo", ImVec2(0, 0), true);

    ImGui::Text("Change media type: ");
    ImGui::SameLine();
    if (_mediaTypeIndex.find(_selectedMediaName) == _mediaTypeIndex.end())
        _mediaTypeIndex[_selectedMediaName] = 0;

    std::vector<const char*> mediaTypes;
    for (const auto& type : _mediaTypes)
        mediaTypes.push_back(type.first.c_str());

    if (ImGui::Combo("##mediaType", &_mediaTypeIndex[_selectedMediaName], mediaTypes.data(), mediaTypes.size()))
        replaceMedia(_selectedMediaName, mediaAlias, mediaTypes[_mediaTypeIndex[_selectedMediaName]]);

    ImGui::Text("Current media type: %s", _mediaTypesReversed[media->getRemoteType()].c_str());

    ImGui::Text("Parameters:");
    auto attributes = getObjectAttributes(media->getName());
    drawAttributes(_selectedMediaName, attributes);

    // Display the playlist if this is a queue
    if (std::dynamic_pointer_cast<QueueSurrogate>(media))
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
                size_t index = 0;
                int deleteIndex = -1;
                int moveIndex = -1;

                for (auto& source : playlist)
                {
                    auto values = source.as<Values>();
                    auto idStack = std::to_string(index) + values[0].as<std::string>();

                    ImGui::PushID((idStack + "up").c_str());
                    if (ImGui::Button("<<"))
                        moveIndex = index;
                    ImGui::PopID();
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Move this media before the previous one");

                    ImGui::SameLine();
                    ImGui::PushID((idStack + "down").c_str());
                    if (ImGui::Button(">>") && index < playlist.size() - 1)
                        moveIndex = index + 1;
                    ImGui::PopID();
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Move this media after the next one");

                    ImGui::SameLine();
                    ImGui::PushID((idStack + "delete").c_str());
                    if (ImGui::Button("-"))
                        deleteIndex = index;
                    ImGui::PopID();
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Delete this media");

                    ImGui::SameLine();
                    ImGui::PushItemWidth(96);
                    ImGui::PushID((idStack + "media_type").c_str());
                    ImGui::Text("%s", _mediaTypesReversed[values[0].as<std::string>()].c_str());
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Media type");
                    ImGui::PopID();
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::PushItemWidth(96);
                    auto tmpFloat = values[2].as<float>();
                    ImGui::PushID((idStack + "start").c_str());
                    if (ImGui::InputFloat("", &tmpFloat, 1.0f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        source[2] = tmpFloat;
                        updated = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Start time (s)");
                    ImGui::PopID();
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::PushItemWidth(96);
                    tmpFloat = values[3].as<float>();
                    ImGui::PushID((idStack + "stop").c_str());
                    if (ImGui::InputFloat("", &tmpFloat, 1.0f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        source[3] = tmpFloat;
                        updated = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Stop time (s)");
                    ImGui::PopID();
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::PushItemWidth(32);
                    auto tmpBool = values[4].as<bool>();
                    ImGui::PushID((idStack + "freeRun").c_str());
                    if (ImGui::Checkbox("", &tmpBool))
                    {
                        source[4] = static_cast<int>(tmpBool);
                        updated = true;
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Freerun: if true, play this media even when paused");
                    ImGui::PopID();
                    ImGui::PopItemWidth();

                    ImGui::SameLine();
                    ImGui::PushItemWidth(-0.01f);
                    ImGui::PushID((idStack + "path").c_str());
                    ImGui::Text("%s", values[1].as<std::string>().c_str());
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

                if (moveIndex > 0)
                {
                    auto delta = playlist[moveIndex][2].as<float>() - playlist[moveIndex - 1][2].as<float>();
                    auto duration = playlist[moveIndex][3].as<float>() - playlist[moveIndex][2].as<float>();
                    playlist[moveIndex][2] = playlist[moveIndex][2].as<float>() - delta;
                    playlist[moveIndex][3] = playlist[moveIndex][3].as<float>() - delta;
                    playlist[moveIndex - 1][2] = playlist[moveIndex - 1][2].as<float>() + duration;
                    playlist[moveIndex - 1][3] = playlist[moveIndex - 1][3].as<float>() + duration;
                    std::swap(playlist[moveIndex], playlist[moveIndex - 1]);
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

            ImGui::SameLine();
            ImGui::PushItemWidth(96); // Width for the media type combo selector
            ImGui::PushID("newMediaType");
            if (ImGui::Combo("", &_newMediaTypeIndex, mediaTypes.data(), mediaTypes.size()))
                _newMedia[0] = _mediaTypes[mediaTypes[_newMediaTypeIndex]];
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Media type");
            ImGui::PopID();
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::PushItemWidth(96); // Width for the media start time input box
            ImGui::PushID("newMediaStart");
            if (ImGui::InputFloat("", &_newMediaStart, 0.1f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                _newMedia[2] = _newMediaStart;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Start time (s)");
            ImGui::PopID();
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::PushItemWidth(96); // Width for the media stop time input box
            ImGui::PushID("newMediaStop");
            if (ImGui::InputFloat("", &_newMediaStop, 0.1f, 1.0f, 2, ImGuiInputTextFlags_EnterReturnsTrue))
                _newMedia[3] = _newMediaStop;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Stop time (s)");
            ImGui::PopID();
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::PushItemWidth(32); // Width for the freerun checkbox
            ImGui::PushID("newMediaFreeRun");
            if (ImGui::Checkbox("", &_newMediaFreeRun))
                _newMedia[4] = static_cast<int>(_newMediaFreeRun);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Freerun: if true, play this media even when paused");
            ImGui::PopID();
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::PushItemWidth(-32.f); // Go up to the other edge, minus this value
            ImGui::PushID("newMediaFile");
            std::string filepath = _newMedia[1].as<std::string>();
            if (SplashImGui::InputText("", filepath, ImGuiInputTextFlags_EnterReturnsTrue))
                _newMedia[1] = filepath;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Media path");
            ImGui::PopItemWidth();

            ImGui::SameLine();
            if (ImGui::Button("..."))
                _fileSelectorTarget = mediaAlias;

            if (_fileSelectorTarget == mediaAlias)
            {
                static std::string path = _root->getMediaPath();
                bool cancelled;
                static const std::vector<std::string> extensions{{"bmp"}, {"jpg"}, {"png"}, {"tga"}, {"tif"}, {"avi"}, {"mov"}, {"mp4"}};
                if (SplashImGui::FileSelector(mediaAlias, path, cancelled, extensions))
                {
                    if (!cancelled)
                        _newMedia[1] = path;
                    path = _root->getMediaPath();
                    _fileSelectorTarget = "";
                }
            }
            ImGui::PopID();

            if (updated)
                setObjectAttribute(_selectedMediaName, "playlist", playlist);

            ImGui::TreePop();
        }

        // Display the filters associated with this queue
        auto filter = std::dynamic_pointer_cast<QueueSurrogate>(media)->getFilter();
        auto filterName = filter->getName();
        if (ImGui::TreeNode(("Filter: " + filterName).c_str()))
        {
            auto filterAttributes = getObjectAttributes(filter->getName());
            drawAttributes(filterName, filterAttributes);
            ImGui::TreePop();
        }

        if (ImGui::TreeNode(("Filter preview: " + filterName).c_str()))
        {
            auto spec = filter->getSpec();
            auto ratio = static_cast<float>(spec.height) / static_cast<float>(spec.width);

            auto leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
            int w = ImGui::GetWindowWidth() - 2 * leftMargin;
            int h = w * ratio;

            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(filter->getTexId())), ImVec2(w, h));
            ImGui::TreePop();
        }
    }
    else
    {
        // Display the filters associated with this media
        auto filters = getFiltersForImage(media);
        for (const auto& filterAsObj : filters)
        {
            auto filterName = filterAsObj->getName();
            if (ImGui::TreeNode(("Filter: " + filterName).c_str()))
            {
                auto filterAttributes = getObjectAttributes(filterAsObj->getName());
                drawAttributes(filterName, filterAttributes);
                ImGui::TreePop();
            }
        }

        for (auto& filterAsObj : filters)
        {
            auto filterName = filterAsObj->getName();
            if (ImGui::TreeNode(("Filter preview: " + filterName).c_str()))
            {
                auto filter = std::dynamic_pointer_cast<Filter>(filterAsObj);
                auto spec = filter->getSpec();
                auto ratio = static_cast<float>(spec.height) / static_cast<float>(spec.width);

                double leftMargin = ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
                int w = ImGui::GetWindowWidth() - 2 * leftMargin;
                int h = w * ratio;

                ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(filter->getTexId())), ImVec2(w, h));
                ImGui::TreePop();
            }
        }
    }

    ImGui::EndChild();
}

/*************/
void GuiMedia::replaceMedia(const std::string& previousMedia, const std::string& alias, const std::string& type)
{
    // We get the list of all objects linked to previousMedia
    auto targetObjects = std::list<std::weak_ptr<GraphObject>>();
    auto objects = getObjectsPtr(getObjectList());
    for (auto& object : objects)
    {
        if (!object->getSavable())
            continue;
        auto linkedObjects = object->getLinkedObjects();
        for (auto& weakLinkedObject : linkedObjects)
        {
            auto linked = weakLinkedObject.lock();
            if (linked && linked->getName() == previousMedia)
                targetObjects.push_back(object);
        }
    }

    Values msg;
    msg.push_back(previousMedia);
    msg.push_back(_mediaTypes[type]);
    msg.push_back(alias);
    for (const auto& weakObject : targetObjects)
    {
        if (weakObject.expired())
            continue;
        auto object = weakObject.lock();
        msg.push_back(object->getName());
    }

    setWorldAttribute("replaceObject", msg);
}

/*************/
int GuiMedia::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    return flags;
}

/*************/
std::list<std::shared_ptr<GraphObject>> GuiMedia::getSceneMedia()
{
    auto mediaList = std::list<std::shared_ptr<GraphObject>>();
    auto mediaTypes = getTypesFromCategory(GraphObject::Category::IMAGE);

    for (const auto& type : mediaTypes)
    {
        auto objects = getObjectsPtr(getObjectsOfType(type));
        for (auto& object : objects)
        {
            if (object->getSavable())
                mediaList.push_back(object);
        }
    }

    mediaList.sort();
    mediaList.unique();

    return mediaList;
}

/*************/
std::list<std::shared_ptr<GraphObject>> GuiMedia::getFiltersForImage(const std::shared_ptr<GraphObject>& image)
{
    auto filterList = std::list<std::shared_ptr<GraphObject>>();
    auto allFilters = getObjectsPtr(getObjectsOfType("filter"));
    for (auto& obj : allFilters)
    {
        if (obj->getType() != "filter")
            continue;

        auto linkedImages = obj->getLinkedObjects();
        auto matchingImage = find_if(linkedImages.begin(), linkedImages.end(), [&](const auto& weakLinkedImage) { return image == weakLinkedImage.lock(); });

        if (matchingImage != linkedImages.end())
            filterList.push_back(obj);
    }

    return filterList;
}

} // namespace Splash
