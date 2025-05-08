#include "./controller/widget/widget_meshes.h"

#include <imgui.h>

#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/filter.h"
#include "./graphics/object.h"
#include "./image/image.h"
#include "./utils/osutils.h"

#define PREVIEW_TEXTURE "color_map.png"
#define PREVIEW_SHADER "shaders/mesh_preview_shader"

namespace Splash
{

/*************/
GuiMeshes::GuiMeshes(Scene* scene, const std::string& name)
    : GuiWidget(scene, name)
{
    auto types = getTypesFromCategory(GraphObject::Category::MESH);
    for (auto& type : types)
        _meshType[getShortDescription(type)] = type;

    for (auto& type : _meshType)
        _meshTypeReversed[type.second] = type.first;

    // Create the GUI camera
    _previewCamera = std::make_shared<Camera>(scene, TreeRegisterStatus::NotRegistered);
    _previewCamera->setName("Overview camera");
    _previewCamera->setAttribute("eye", {4.0, 4.0, 0.0});
    _previewCamera->setAttribute("target", {0.0, 0.0, 0.5});
    _previewCamera->setAttribute("fov", {50.0});
    _previewCamera->setAttribute("size", {800, 600});
    _previewCamera->setAttribute("savable", {false});

    // Load a default texture
    _previewImage = std::make_shared<Image>(scene, TreeRegisterStatus::NotRegistered);
    _previewImage->read(std::string(DATADIR) + PREVIEW_TEXTURE);

    // Create an object to render the mesh preview
    _previewObject = std::make_shared<Object>(scene, TreeRegisterStatus::NotRegistered);
    _previewObject->setAttribute("savable", {false});
    _previewObject->linkTo(_previewImage);

    // Use a specific shader, which shows the wireframe and the texture at the same time
    _previewShader = std::make_shared<Shader>(_renderer);
    _previewShader->setSourceFromFile(gfx::ShaderType::geometry, std::string(DATADIR) + PREVIEW_SHADER + ".geom");
    _previewShader->setSourceFromFile(gfx::ShaderType::fragment, std::string(DATADIR) + PREVIEW_SHADER + ".frag");
    _previewObject->setShader(_previewShader);

    // Show the object in the preview camera
    _previewCamera->linkTo(_previewObject);
}

/*************/
void GuiMeshes::render()
{
    auto meshList = getSceneMeshes();

    if (meshList.size() != 0 && _selectedMeshName.empty())
    {
        assert(meshList.front() != nullptr);
        _selectedMeshName = meshList.front()->getName();
        _previewObject->linkTo(meshList.front());
    }

    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##meshes", ImVec2(availableSize.x * 0.25, availableSize.y), ImGuiChildFlags_Borders);
    ImGui::Text("Mesh list");

    auto leftMargin = static_cast<int>(ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x);
    for (auto& mesh : meshList)
    {
        int w = ImGui::GetWindowWidth() - 2 * leftMargin;

        if (ImGui::Button(mesh->getAlias().c_str(), ImVec2(w, w)))
        {
            _selectedMeshName = mesh->getName();
            if (_currentMesh)
                _previewObject->unlinkFrom(_currentMesh);
            _previewObject->linkTo(mesh);
            _currentMesh = mesh;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", mesh->getAlias().c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##meshInfo", ImVec2(0, 0), ImGuiChildFlags_Borders);

    auto mesh = getObjectPtr(_selectedMeshName);
    if (!mesh)
    {
        ImGui::Text("Select a mesh on the left");
    }
    else
    {
        auto meshAlias = getObjectAlias(_selectedMeshName);

        ImGui::Text("Mesh name: %s", _selectedMeshName.c_str());
        ImGui::Text("Current mesh type: ");
        ImGui::SameLine();
        if (_meshTypeIndex.find(_selectedMeshName) == _meshTypeIndex.end())
            _meshTypeIndex[_selectedMeshName] = 0;

        std::vector<const char*> meshTypes;
        for (const auto& type : _meshType)
            meshTypes.push_back(type.first.c_str());

        const auto meshTypeIt = _meshType.find(_meshTypeReversed[mesh->getRemoteType()]);
        int meshTypeIndex = std::distance(_meshType.begin(), meshTypeIt);

        if (ImGui::Combo("##meshType", &meshTypeIndex, meshTypes.data(), meshTypes.size()))
            replaceMesh(_selectedMeshName, meshAlias, meshTypes[meshTypeIndex]);

        if (_previewCamera != nullptr)
        {
            availableSize = ImGui::GetContentRegionAvail();
            _previewCamera->setAttribute("size", {static_cast<int>(availableSize.x), static_cast<int>(availableSize.y)});
            _previewCamera->render();

            Values size;
            _previewCamera->getAttribute("size", size);

            auto sizeX = size[0].as<int>();
            auto sizeY = size[1].as<int>();

            int w = ImGui::GetWindowWidth() - 2 * leftMargin;
            int h = sizeX != 0 ? w * sizeY / sizeX : 1;

            availableSize = ImGui::GetContentRegionAvail();
            if (h > availableSize.y)
            {
                h = availableSize.y;
                w = sizeY != 0 ? h * sizeX / sizeY : 1;
            }

            _camWidth = w;
            _camHeight = h;

            ImGui::Image((ImTextureID)(intptr_t)_previewCamera->getTexture()->getTexId(), ImVec2(w, h), ImVec2(0, 1), ImVec2(1, 0));

            processMouseEvents();
        }
    }

    ImGui::EndChild();
}

/*************/
void GuiMeshes::processMouseEvents()
{
    ImGuiIO& io = ImGui::GetIO();

    // Get mouse pos
    ImVec2 mousePos = ImVec2((io.MousePos.x - ImGui::GetCursorScreenPos().x) / _camWidth, -(io.MousePos.y - ImGui::GetCursorScreenPos().y) / _camHeight);

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
    {
        if (io.MouseWheel != 0)
        {
            Values fov;
            _previewCamera->getAttribute("fov", fov);
            const auto camFov = std::max(2.f, std::min(180.f, fov[0].as<float>() + io.MouseWheel));
            _previewCamera->setAttribute("fov", {camFov});
        }

        // Calibration point set
        if (io.MouseClicked[2])
        {
            float fragDepth = 0.f;
            _cameraTarget = _previewCamera->pickFragment(mousePos.x, mousePos.y, fragDepth);

            if (fragDepth == 0.f)
                _cameraTargetDistance = 1.f;
            else
                _cameraTargetDistance = -fragDepth * 0.1f;
        }
    }

    // This handles the mouse capture even when the mouse goes outside the view
    // widget, which controls are defined next
    static bool viewCaptured = false;
    if (io.MouseDownDuration[2] > 0.0)
    {
        if (ImGui::IsItemHovered())
            viewCaptured = true;
    }
    else if (viewCaptured)
    {
        viewCaptured = false;
    }

    // View widget controls
    if (viewCaptured)
    {
        // Move the camera
        if (!io.KeyCtrl && !io.KeyShift)
        {
            const float dx = io.MouseDelta.x;
            const float dy = io.MouseDelta.y;

            // We reset the up vector. Not ideal, but prevent the camera from being
            // unusable.
            setObjectAttribute(_previewCamera->getName(), "up", {0.0, 0.0, 1.0});
            if (_cameraTarget.size() == 3)
                _previewCamera->setAttribute("rotateAroundPoint", {dx / 100.f, dy / 100.f, 0.0, _cameraTarget[0].as<float>(), _cameraTarget[1].as<float>(), _cameraTarget[2].as<float>()});
            else
                _previewCamera->setAttribute("rotateAroundTarget", {dx / 100.f, dy / 100.f, 0.0});
        }
        // Move the target and the camera (in the camera plane)
        else if (io.KeyShift && !io.KeyCtrl)
        {
            const float dx = io.MouseDelta.x * _cameraTargetDistance;
            const float dy = io.MouseDelta.y * _cameraTargetDistance;
            _previewCamera->setAttribute("pan", {-dx / 100.f, dy / 100.f, 0.0});
        }
        else if (!io.KeyShift && io.KeyCtrl)
        {
            const float dy = io.MouseDelta.y * _cameraTargetDistance / 100.f;
            _previewCamera->setAttribute("forward", {dy});
        }
    }
}

/*************/
void GuiMeshes::replaceMesh(const std::string& previousMedia, const std::string& alias, const std::string& type)
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
const std::list<std::shared_ptr<Mesh>> GuiMeshes::getSceneMeshes()
{
    auto meshList = std::list<std::shared_ptr<Mesh>>();
    const auto meshTypes = getTypesFromCategory(GraphObject::Category::MESH);

    for (const auto& type : meshTypes)
    {
        const auto objects = getObjectsPtr(getObjectsOfType(type));
        for (const auto& object : objects)
            if (object->getSavable())
                meshList.push_back(std::dynamic_pointer_cast<Mesh>(object));
    }

    return meshList;
}

} // namespace Splash
