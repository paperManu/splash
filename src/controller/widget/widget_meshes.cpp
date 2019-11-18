#include "./controller/widget/widget_meshes.h"

#include <imgui.h>

#include "./core/scene.h"
#include "./image/queue.h"
#include "./utils/osutils.h"

using namespace std;

namespace Splash
{

/*************/
GuiMeshes::GuiMeshes(Scene* scene, const string& name)
    : GuiWidget(scene, name)
{
    auto types = getTypesFromCategory(GraphObject::Category::MESH);
    for (auto& type : types)
        _meshType[getShortDescription(type)] = type;

    for (auto& type : _meshType)
        _meshTypeReversed[type.second] = type.first;
}

/*************/
void GuiMeshes::render()
{
    auto meshList = getSceneMeshes();

    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##meshes", ImVec2(availableSize.x * 0.25, availableSize.y), true);
    ImGui::Text("Mesh list");

    auto leftMargin = static_cast<int>(ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x);
    for (auto& mesh : meshList)
    {
        int w = ImGui::GetWindowWidth() - 2 * leftMargin;

        if (ImGui::Button(mesh->getName().c_str(), ImVec2(w, w)))
            _selectedMeshName = mesh->getName();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", mesh->getName().c_str());
    }
    ImGui::EndChild();

    auto mesh = getObjectPtr(_selectedMeshName);
    if (!mesh)
        return;

    auto meshAlias = getObjectAlias(_selectedMeshName);

    ImGui::SameLine();
    ImGui::BeginChild("##meshInfo", ImVec2(0, 0), true);

    ImGui::Text("Change mesh type: ");
    ImGui::SameLine();
    if (_meshTypeIndex.find(_selectedMeshName) == _meshTypeIndex.end())
        _meshTypeIndex[_selectedMeshName] = 0;

    vector<const char*> meshTypes;
    for (auto& type : _meshType)
        meshTypes.push_back(type.first.c_str());

    if (ImGui::Combo("##meshType", &_meshTypeIndex[_selectedMeshName], meshTypes.data(), meshTypes.size()))
        replaceMesh(_selectedMeshName, meshAlias, meshTypes[_meshTypeIndex[_selectedMeshName]]);

    ImGui::Text("Current mesh type: %s", _meshTypeReversed[mesh->getRemoteType()].c_str());

    ImGui::Text("Parameters:");
    auto attributes = getObjectAttributes(mesh->getName());
    drawAttributes(_selectedMeshName, attributes);

    ImGui::EndChild();
}

/*************/
void GuiMeshes::replaceMesh(const string& previousMedia, const string& alias, const string& type)
{
    // We get the list of all objects linked to previousMedia
    auto targetObjects = list<weak_ptr<GraphObject>>();
    auto objects = getObjectsPtr(getObjectList());
    for (auto& object : objects)
    {
        if (!object->getSavable())
            continue;

        auto linkedObjects = object->getLinkedObjects();
        for (auto& weakLinkedObject : linkedObjects)
        {
            auto linkedObject = weakLinkedObject.lock();
            if (linkedObject && linkedObject->getName() == previousMedia)
                targetObjects.push_back(object);
        }
    }

    Values msg;
    msg.push_back(previousMedia);
    msg.push_back(_meshType[type]);
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
int GuiMeshes::updateWindowFlags()
{
    ImGuiWindowFlags flags = 0;
    return flags;
}

/*************/
list<shared_ptr<GraphObject>> GuiMeshes::getSceneMeshes()
{
    auto meshList = list<shared_ptr<GraphObject>>();
    auto meshTypes = list<string>({"mesh"});

    for (auto& type : meshTypes)
    {
        auto objects = getObjectsPtr(getObjectsOfType(type));
        for (auto& object : objects)
            if (object->getSavable())
                meshList.push_back(object);
    }

    return meshList;
}

} // namespace Splash
