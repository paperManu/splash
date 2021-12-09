#include "./controller/controller_blender.h"

#include "./core/scene.h"
#include "./graphics/camera.h"
#include "./graphics/geometry.h"
#include "./graphics/object.h"

namespace chrono = std::chrono;

namespace Splash
{

/*************/
Blender::Blender(RootObject* root)
    : ControllerObject(root)
{
    _type = "blender";
    _renderingPriority = Priority::BLENDING;
    registerAttributes();
}

/*************/
void Blender::update()
{
    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return;

    auto isMaster = scene->isMaster();

    auto getObjLinkedToCameras = [&]() -> std::vector<std::shared_ptr<GraphObject>> {
        std::vector<std::shared_ptr<GraphObject>> objLinkedToCameras{};

        const auto cameras = getObjectsPtr(getObjectsOfType("camera"));
        const auto links = getObjectLinks();
        for (auto& camera : cameras)
        {
            const auto cameraLinks = links.at(camera->getName());
            for (auto& linked : cameraLinks)
            {
                const auto object = std::dynamic_pointer_cast<Object>(getObjectPtr(linked));
                if (object)
                    objLinkedToCameras.push_back(getObjectPtr(linked));
            }
        }

        return objLinkedToCameras;
    };

    if (_computeBlending && (!_blendingComputed || _continuousBlending))
    {
        _blendingComputed = true;

        // Only the master scene computes the blending
        if (isMaster)
        {
            std::vector<std::shared_ptr<Camera>> cameras;
            std::vector<std::shared_ptr<Object>> objects;

            auto objectList = getObjectsPtr(getObjectsOfType("camera"));
            std::transform(objectList.cbegin(), objectList.cend(), std::back_inserter(cameras), [](auto cameraPtr) {
                auto camera = std::dynamic_pointer_cast<Camera>(cameraPtr);
                assert(camera != nullptr);
                return camera;
            });

            objectList = getObjLinkedToCameras();
            std::transform(objectList.cbegin(), objectList.cend(), std::back_inserter(objects), [](auto objectPtr) {
                auto object = std::dynamic_pointer_cast<Object>(objectPtr);
                assert(object != nullptr);
                return object;
            });

            // If depth-aware depth computation is deactivated, also
            // deactivate vertex depth computation to prevent wasting
            // processing power
            setObjectsOfType("object", "computeFarthestVisibleVertexDistance", {_depthAwareBlending});

            if (!cameras.empty())
            {
                for (auto& object : objects)
                    object->resetTessellation();

                // Tessellate
                for (auto& camera : cameras)
                {
                    camera->computeVertexVisibility();
                    camera->blendingTessellateForCurrentCamera();
                }

                for (auto& object : objects)
                    object->resetBlendingAttribute();

                // Compute each camera contribution
                float maxVertexDistance = 0.f;
                for (auto& camera : cameras)
                {
                    camera->computeVertexVisibility();
                    camera->computeBlendingContribution();
                    maxVertexDistance = std::max(maxVertexDistance, camera->getFarthestVisibleVertexDistance());
                }

                // Set the farthest visible vertex distance into all objects,
                // so it can be used when rendering the blending
                for (auto& object : objects)
                    object->setAttribute("farthestVisibleVertexDistance", {maxVertexDistance});
            }
            else
            {
                return;
            }

            for (auto& object : objects)
                object->setAttribute("activateVertexBlending", {true});

            // If there are some other scenes, send them the blending
            auto geometries = getObjectsPtr(getObjectsOfType("geometry"));
            for (auto& geometry : geometries)
            {
                auto serializedGeometry = std::dynamic_pointer_cast<Geometry>(geometry)->serialize();
                sendBuffer(std::move(serializedGeometry));
            }

            setObjectAttribute(_name, "blendingUpdated", {});
        }
        // The non-master scenes only need to activate blending
        else
        {
            // Wait for the master scene to notify us that the blending was updated
            // Note that we do not wait more that 2 seconds
            std::unique_lock<std::mutex> updateBlendingLock(_vertexBlendingMutex);
            int maxSecElapsed = 2;
            while (!_vertexBlendingReceptionStatus && maxSecElapsed)
            {
                _vertexBlendingCondition.wait_for(updateBlendingLock, chrono::seconds(1));
                maxSecElapsed--;
            }

            if (_vertexBlendingReceptionStatus)
            {
                _vertexBlendingReceptionStatus = false;

                for (auto& object : getObjLinkedToCameras())
                    object->setAttribute("activateVertexBlending", {true});
            }
        }
    }
    // This deactivates the blending
    else if (_blendingComputed && !_computeBlending)
    {
        _blendingComputed = false;

        auto cameras = getObjectsPtr(getObjectsOfType("camera"));
        auto objects = getObjLinkedToCameras();

        if (isMaster && cameras.size() != 0)
        {
            for (auto& it : objects)
            {
                auto object = std::dynamic_pointer_cast<Object>(it);
                object->resetTessellation();
                object->resetVisibility();
            }
        }

        for (auto& object : objects)
            object->setAttribute("activateVertexBlending", {false});
    }
}

/*************/
void Blender::registerAttributes()
{
    ControllerObject::registerAttributes();

    addAttribute(
        "mode",
        [&](const Values& args) {
            auto mode = args[0].as<std::string>();
            if (mode == "none")
            {
                _computeBlending = false;
                _continuousBlending = false;
            }
            else if (mode == "once")
            {
                _computeBlending = true;
                _continuousBlending = false;
            }
            else if (mode == "continuous")
            {
                _computeBlending = true;
                _continuousBlending = true;
            }
            else
            {
                return false;
            }

            _blendingMode = mode;

            return true;
        },
        [&]() -> Values { return {_blendingMode}; },
        {'s'});
    setAttributeDescription("mode", "Set the blending mode. Can be 'none', 'once' or 'continuous'");

    addAttribute("blendingUpdated",
        [&](const Values&) {
            _vertexBlendingReceptionStatus = true;
            _vertexBlendingCondition.notify_one();
            return true;
        },
        {});
    setAttributeDescription("blendingUpdated", "Message sent by the master Scene to notify that a new blending has been computed");
    setAttributeSyncMethod("blendingUpdated", Attribute::Sync::force_sync);

    addAttribute(
        "depthAwareBlending",
        [&](const Values& args) {
            _depthAwareBlending = args[0].as<bool>();
            return true;
        },
        [&]() -> Values { return {_depthAwareBlending}; },
        {'b'});
    setAttributeDescription("depthAwareBlending",
        "When active, blending computation will adapt projections "
        "intensity with respect towards the distance to the projectors,"
        "to produce a uniform surface illumination");
}

} // namespace Splash
