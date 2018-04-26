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
    if (ImGui::CollapsingHeader(_name.c_str()))
    {
        auto meshList = getSceneMeshes();
        for (auto& mesh : meshList)
        {
            auto meshName = mesh->getName();
            if (ImGui::TreeNode(mesh->getAlias().c_str()))
            {
                ImGui::Text("Change mesh type: ");
                ImGui::SameLine();

                if (_meshTypeIndex.find(meshName) == _meshTypeIndex.end())
                    _meshTypeIndex[meshName] = 0;

                vector<const char*> meshTypes;
                for (auto& type : _meshType)
                    meshTypes.push_back(type.first.c_str());

                if (ImGui::Combo("", &_meshTypeIndex[meshName], meshTypes.data(), meshTypes.size()))
                    replaceMesh(meshName, meshTypes[_meshTypeIndex[meshName]]);

                ImGui::Text("Current mesh type: %s", _meshTypeReversed[mesh->getRemoteType()].c_str());

                ImGui::Text("Parameters:");
                auto attributes = mesh->getAttributes(true);
                drawAttributes(meshName, attributes);

                ImGui::TreePop();
            }
        }
    }
}

/*************/
void GuiMeshes::replaceMesh(const string& previousMedia, const string& type)
{
    // We get the list of all objects linked to previousMedia
    auto targetObjects = list<weak_ptr<GraphObject>>();
    auto objects = getObjectsOfType("");
    for (auto& object : objects)
    {
        if (!object->getSavable())
            continue;
        auto linkedObjects = object->getLinkedObjects();
        for (auto& linked : linkedObjects)
            if (linked->getName() == previousMedia)
                targetObjects.push_back(object);
    }

    Values msg;
    msg.push_back(previousMedia);
    msg.push_back(_meshType[type]);
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
        auto objects = getObjectsOfType(type);
        for (auto& object : objects)
            if (object->getSavable())
                meshList.push_back(object);
    }

    return meshList;
}

} // end of namespace
