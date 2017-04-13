#include "./controller_blender.h"

#include "./camera.h"
#include "./geometry.h"
#include "./object.h"
#include "./scene.h"

using namespace std;

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
Blender::~Blender()
{
}

/*************/
void Blender::update()
{
    auto scene = dynamic_cast<Scene*>(_root);
    if (!scene)
        return;

    auto isMaster = scene->isMaster();

    if (_computeBlending && (!_blendingComputed || _continuousBlending))
    {
        _blendingComputed = true;

        // Only the master scene computes the blending
        if (isMaster)
        {
            auto cameras = getObjectsOfType("camera");
            auto objects = getObjectsOfType("object");

            if (cameras.size() != 0)
            {
                for (auto& it : objects)
                    dynamic_pointer_cast<Object>(it)->resetTessellation();

                // Tessellate
                for (auto& it : cameras)
                {
                    auto camera = dynamic_pointer_cast<Camera>(it);
                    camera->computeVertexVisibility();
                    camera->blendingTessellateForCurrentCamera();
                }

                for (auto& it : objects)
                    dynamic_pointer_cast<Object>(it)->resetBlendingAttribute();

                // Compute each camera contribution
                for (auto& it : cameras)
                {
                    auto camera = dynamic_pointer_cast<Camera>(it);
                    camera->computeVertexVisibility();
                    camera->computeBlendingContribution();
                }
            }
            else
            {
                return;
            }

            setObjectsOfType("object", "activateVertexBlending", {1});

            // If there are some other scenes, send them the blending
            auto geometries = getObjectsOfType("geometry");
            for (auto& geometry : geometries)
            {
                auto serializedGeometry = dynamic_pointer_cast<Geometry>(geometry)->serialize();
                sendBuffer(geometry->getName(), std::move(serializedGeometry));
            }

            setObject(_name, "blendingUpdated", {});
        }
        // The non-master scenes only need to activate blending
        else
        {
            // Wait for the master scene to notify us that the blending was updated
            unique_lock<mutex> updateBlendingLock(_vertexBlendingMutex);
            while (!_vertexBlendingReceptionStatus)
                _vertexBlendingCondition.wait_for(updateBlendingLock, chrono::seconds(1));
            _vertexBlendingReceptionStatus = false;

            auto objects = getObjectsOfType("object");
            for (auto& object : objects)
                object->setAttribute("activateVertexBlending", {1});

            auto geometries = getObjectsOfType("geometry");
            for (auto& it : geometries)
                dynamic_pointer_cast<Geometry>(it)->useAlternativeBuffers(true);
        }
    }
    // This deactivates the blending
    else if (_blendingComputed && !_computeBlending)
    {
        _blendingComputed = false;

        auto cameras = getObjectsOfType("camera");
        auto objects = getObjectsOfType("object");

        if (isMaster && cameras.size() != 0)
        {
            for (auto& it : objects)
            {
                auto object = dynamic_pointer_cast<Object>(it);
                object->resetTessellation();
                object->resetVisibility();
            }
        }

        for (auto& object : objects)
            object->setAttribute("activateVertexBlending", {0});

        auto geometries = getObjectsOfType("geometry");
        for (auto& it : geometries)
            dynamic_pointer_cast<Geometry>(it)->useAlternativeBuffers(false);
    }
}

/*************/
void Blender::registerAttributes()
{
    ControllerObject::registerAttributes();

    addAttribute("mode",
        [&](const Values& args) {
            auto mode = args[0].as<string>();
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

    addAttribute("blendingUpdated", [&](const Values& args) {
        _vertexBlendingReceptionStatus = true;
        _vertexBlendingCondition.notify_one();
        return true;
    });
    setAttributeDescription("blendingUpdated", "Message sent by the master Scene to notify that a new blending has been computed");
    setAttributeSyncMethod("blendingUpdated", AttributeFunctor::Sync::force_sync);
}

} // end of namespace
