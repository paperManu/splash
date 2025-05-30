#include "./graphics/object.h"

#include <algorithm>

#include "./graphics/filter.h"
#include "./graphics/geometry.h"
#include "./graphics/shader.h"
#include "./graphics/texture.h"
#include "./graphics/texture_image.h"
#include "./image/image.h"
#include "./mesh/mesh.h"
#include "./utils/log.h"
#include "./utils/timer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>
#include <limits>

namespace Splash
{

/*************/
Object::Object(RootObject* root, TreeRegisterStatus registerToTree)
    : GraphObject(root, registerToTree)
{
    init();
}

/*************/
void Object::init()
{
    _type = "object";
    registerAttributes();

    // This is used for getting documentation "offline"
    if (!_root)
        return;

    _modelMatrix = glm::dmat4(0.0);
}

/*************/
Object::~Object()
{
#ifdef DEBUG
    Log::get() << Log::DEBUGGING << "Object::~Object - Destructor" << Log::endl;
#endif
}

/*************/
void Object::activate()
{
    DebugGraphicsScope;
    _mutex.lock();

    // Create and store the shader depending on its type
    auto shaderIt = _graphicsShaders.find(_fill);
    if (shaderIt == _graphicsShaders.end())
    {
        _shader = std::make_shared<Shader>(_renderer);
        _graphicsShaders[_fill] = _shader;
    }
    else
    {
        _shader = shaderIt->second;
    }

    // Set the shader depending on a few other parameters
    std::vector<std::string> shaderParameters{};
    for (uint32_t i = 0; i < _textures.size(); ++i)
        shaderParameters.push_back("TEX_" + std::to_string(i + 1));
    shaderParameters.push_back("TEXCOUNT " + std::to_string(_textures.size()));

    for (auto& p : _fillParameters)
        shaderParameters.push_back(p);

    if (_fill == "texture")
    {
        if (_vertexBlendingActive)
        {
            shaderParameters.push_back("VERTEXBLENDING");
            _shader->setUniform("_farthestVertex", _farthestVisibleVertexDistance);
        }

        if (_textures.size() > 0 && _textures[0]->getType() == "texture_syphon")
            shaderParameters.push_back("TEXTURE_RECT");

        _shader->selectFillMode("texture", shaderParameters);
    }
    else if (_fill == "filter")
    {
        if (_textures.size() > 0 && _textures[0]->getType() == "texture_syphon")
            shaderParameters.push_back("TEXTURE_RECT");

        _shader->selectFillMode("filter", shaderParameters);
    }
    else if (_fill == "window")
    {
        _shader->selectFillMode("window", shaderParameters);
    }
    else
    {
        _shader->selectFillMode(_fill, shaderParameters);
    }

    // Set some uniforms
    _shader->setCulling(_culling);
    _shader->setUniform("_normalExp", _normalExponent);
    _shader->setUniform("_color", {_color.r, _color.g, _color.b, _color.a});

    if (_geometry)
    {
        _geometry->update();
        _geometry->activate();
    }
    _shader->activate();

    GLuint texUnit = 0;
    for (auto& t : _textures)
    {
        _shader->setTexture(t, texUnit, t->getPrefix() + std::to_string(texUnit));

        // Get texture specific uniforms and send them to the shader
        auto texUniforms = t->getShaderUniforms();
        for (const auto& [name, value] : texUniforms)
        {
            const auto uniformName = t->getPrefix() + std::to_string(texUnit) + "_" + name;
            _shader->setUniform(uniformName, value);
        }

        texUnit++;
    }
}

/*************/
glm::dmat4 Object::computeModelMatrix() const
{
    if (_modelMatrix != glm::dmat4(0.0))
        return _modelMatrix;
    else
        return glm::translate(glm::dmat4(1.f), _position) * glm::rotate(glm::dmat4(1.f), _rotation.z, glm::dvec3(0.0, 0.0, 1.0)) *
               glm::rotate(glm::dmat4(1.f), _rotation.y, glm::dvec3(0.0, 1.0, 0.0)) * glm::rotate(glm::dmat4(1.f), _rotation.x, glm::dvec3(1.0, 0.0, 0.0)) *
               glm::scale(glm::dmat4(1.f), _scale);
}

/*************/
void Object::deactivate()
{
    DebugGraphicsScope;
    _shader->deactivate();
    if (_geometry)
        _geometry->deactivate();
    _mutex.unlock();
}

/**************/
void Object::addCalibrationPoint(const glm::dvec3& point)
{
    for (auto& p : _calibrationPoints)
        if (p == point)
            return;

    _calibrationPoints.push_back(point);
}

/**************/
int64_t Object::getTimestamp() const
{
    int64_t timestamp = 0;
    for (const auto& texture : _textures)
        timestamp = std::max(timestamp, texture->getTimestamp());
    return timestamp;
}

/**************/
void Object::removeCalibrationPoint(const glm::dvec3& point)
{
    for (auto it = _calibrationPoints.begin(), itEnd = _calibrationPoints.end(); it != itEnd; ++it)
    {
        if (point == *it)
        {
            _calibrationPoints.erase(it);
            return;
        }
    }
}

/*************/
void Object::draw()
{
    if (_geometry)
        _geometry->draw();
    return;
}

/*************/
int Object::getVerticesNumber() const
{
    return _geometry->getVerticesNumber();
}

/*************/
bool Object::linkIt(const std::shared_ptr<GraphObject>& obj)
{
    if (obj->getType().find("texture") != std::string::npos)
    {
        auto filter = std::dynamic_pointer_cast<Filter>(_root->createObject("filter", getName() + "_" + obj->getName() + "_filter").lock());
        filter->setSavable(_savable); // We always save the filters as they hold user-specified values, if this is savable
        if (filter->linkTo(obj))
            return linkTo(filter);
        else
            return false;
    }
    else if (obj->getType().find("filter") != std::string::npos)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("virtual_probe") != std::string::npos)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("queue") != std::string::npos)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("image") != std::string::npos)
    {
        auto filter = std::dynamic_pointer_cast<Filter>(_root->createObject("filter", getName() + "_" + obj->getName() + "_filter").lock());
        filter->setSavable(_savable); // We always save the filters as they hold user-specified values, if this is savable
        if (filter->linkTo(obj))
            return linkTo(filter);
        else
            return false;
    }
    else if (obj->getType().find("mesh") != std::string::npos)
    {
        auto geom = std::dynamic_pointer_cast<Geometry>(_root->createObject("geometry", getName() + "_" + obj->getName() + "_geom").lock());
        if (geom->linkTo(obj))
            return linkTo(geom);
        else
            return false;
    }
    else if (obj->getType().find("geometry") != std::string::npos)
    {
        if (_geometry)
            return false;
        auto geom = std::dynamic_pointer_cast<Geometry>(obj);
        setGeometry(geom);
        return true;
    }

    return false;
}

/*************/
void Object::unlinkIt(const std::shared_ptr<GraphObject>& obj)
{
    auto type = obj->getType();
    if (type.find("texture") != std::string::npos)
    {
        auto filterName = getName() + "_" + obj->getName() + "_filter";

        if (auto filter = _root->getObject(filterName))
        {
            filter->unlinkFrom(obj);
            unlinkFrom(filter);
        }

        _root->disposeObject(filterName);
    }
    else if (type.find("image") != std::string::npos)
    {
        auto filterName = getName() + "_" + obj->getName() + "_filter";

        if (auto filter = _root->getObject(filterName))
        {
            filter->unlinkFrom(obj);
            unlinkFrom(filter);
        }

        _root->disposeObject(filterName);
    }
    else if (type.find("filter") != std::string::npos)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }
    else if (obj->getType().find("virtual_screen") != std::string::npos)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }
    else if (type.find("mesh") != std::string::npos)
    {
        auto geomName = getName() + "_" + obj->getName() + "_geom";

        if (auto geom = _root->getObject(geomName))
        {
            geom->unlinkFrom(obj);
            unlinkFrom(geom);
        }

        _root->disposeObject(geomName);
    }
    else if (type.find("geometry") != std::string::npos)
    {
        auto geom = std::dynamic_pointer_cast<Geometry>(obj);
        if (geom == _geometry)
            resetGeometry();
    }
    else if (obj->getType().find("queue") != std::string::npos)
    {
        auto tex = std::dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }
}

/*************/
float Object::pickVertex(glm::dvec3 p, glm::dvec3& v)
{
    float distance = std::numeric_limits<float>::max();
    glm::dvec3 closestVertex;
    float tmpDist;
        glm::dvec3 vertex;
        if ((tmpDist = _geometry->pickVertex(p, vertex)) < distance)
        {
            distance = tmpDist;
            closestVertex = vertex;
        }

    v = closestVertex;
    return distance;
}

/*************/
void Object::resetGeometry()
{
    _geometry.reset();
}

/*************/
void Object::removeTexture(const std::shared_ptr<Texture>& tex)
{
    auto texIterator = find(_textures.begin(), _textures.end(), tex);
    if (texIterator != _textures.end())
        _textures.erase(texIterator);
}

/*************/
void Object::resetVisibility(int primitiveIdShift)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_computeShaderResetVisibility)
    {
        _computeShaderResetVisibility = std::make_shared<Shader>(_renderer, Shader::prgCompute);
        _computeShaderResetVisibility->selectComputePhase(Shader::ComputePhase::ResetVisibility);
    }

    if (_computeShaderResetVisibility)
    {
        _geometry->update();
        _geometry->activateAsSharedBuffer();
        auto verticesNbr = _geometry->getVerticesNumber();
        _computeShaderResetVisibility->setUniform("_vertexNbr", verticesNbr);
        _computeShaderResetVisibility->setUniform("_primitiveIdShift", primitiveIdShift);
        if (!_computeShaderResetVisibility->doCompute(verticesNbr / (3 * 128) + 1))
            Log::get() << Log::WARNING << "Object::" << __FUNCTION__ << " - Error while computing the visibility" << Log::endl;
        _geometry->deactivate();
    }
}

/*************/
void Object::resetBlendingAttribute()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_computeShaderResetBlendingAttributes)
    {
        _computeShaderResetBlendingAttributes = std::make_shared<Shader>(_renderer, Shader::prgCompute);
        _computeShaderResetBlendingAttributes->selectComputePhase(Shader::ComputePhase::ResetBlending);
    }

    if (_computeShaderResetBlendingAttributes)
    {
        _geometry->update();
        _geometry->activateAsSharedBuffer();
        auto verticesNbr = _geometry->getVerticesNumber();
        _computeShaderResetBlendingAttributes->setUniform("_vertexNbr", verticesNbr);
        _computeShaderResetBlendingAttributes->doCompute(verticesNbr / 3 / 128 + 1);
        _geometry->deactivate();
    }
}

/*************/
void Object::resetTessellation()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _geometry->useAlternativeBuffers(false);
}

/*************/
void Object::tessellateForThisCamera(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float fovX, float fovY, float blendWidth, float blendPrecision)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_feedbackShaderSubdivideCamera)
    {
        _feedbackShaderSubdivideCamera = std::make_shared<Shader>(_renderer, Shader::prgFeedback);
        _feedbackShaderSubdivideCamera->selectFeedbackPhase(
            Shader::FeedbackPhase::TessellateFromCamera, {"GEOM_OUT.vertex", "GEOM_OUT.texCoord", "GEOM_OUT.normal", "GEOM_OUT.annexe"});
    }

    assert(_feedbackShaderSubdivideCamera != nullptr);

    _feedbackShaderSubdivideCamera->setUniform("_blendWidth", blendWidth);
    _feedbackShaderSubdivideCamera->setUniform("_blendPrecision", blendPrecision);
    _feedbackShaderSubdivideCamera->setCulling(_culling);
    _feedbackShaderSubdivideCamera->setUniform("_fov", {fovX, fovY});

        do
        {
            _geometry->update();
            _geometry->activate();

            auto mv = viewMatrix * computeModelMatrix();
            auto mvAsValues = Values(glm::value_ptr(mv), glm::value_ptr(mv) + 16);
            _feedbackShaderSubdivideCamera->setUniform("_mv", mvAsValues);

            auto mvp = projectionMatrix * viewMatrix * computeModelMatrix();
            auto mvpAsValues = Values(glm::value_ptr(mvp), glm::value_ptr(mvp) + 16);
            _feedbackShaderSubdivideCamera->setUniform("_mvp", mvpAsValues);

            auto ip = glm::inverse(projectionMatrix);
            auto ipAsValues = Values(glm::value_ptr(ip), glm::value_ptr(ip) + 16);
            _feedbackShaderSubdivideCamera->setUniform("_ip", ipAsValues);

            auto mNormal = projectionMatrix * glm::transpose(glm::inverse(viewMatrix * computeModelMatrix()));
            auto mNormalAsValues = Values(glm::value_ptr(mNormal), glm::value_ptr(mNormal) + 16);
            _feedbackShaderSubdivideCamera->setUniform("_mNormal", mNormalAsValues);

            _geometry->activateForFeedback();
            _feedbackShaderSubdivideCamera->activate();
            _geometry->draw();
            _feedbackShaderSubdivideCamera->deactivate();

            _geometry->deactivateFeedback();
            _geometry->deactivate();

        } while (_geometry->hasBeenResized());

        _geometry->swapBuffers();
        _geometry->useAlternativeBuffers(true);
}

/*************/
void Object::transferVisibilityFromTexToAttr(int width, int height, int primitiveIdShift)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_computeShaderTransferVisibilityToAttr)
    {
        _computeShaderTransferVisibilityToAttr = std::make_shared<Shader>(_renderer, Shader::prgCompute);
        _computeShaderTransferVisibilityToAttr->selectComputePhase(Shader::ComputePhase::TransferVisibilityToAttribute);
    }

    assert(_computeShaderTransferVisibilityToAttr != nullptr);

    _computeShaderTransferVisibilityToAttr->setUniform("_texSize", {static_cast<float>(width), static_cast<float>(height)});
    _computeShaderTransferVisibilityToAttr->setUniform("_idShift", primitiveIdShift);

    _geometry->update();
    _geometry->activateAsSharedBuffer();
    _computeShaderTransferVisibilityToAttr->doCompute(width / 32 + 1, height / 32 + 1);
    _geometry->deactivate();
}

/*************/
void Object::computeCameraContribution(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_computeShaderComputeBlending)
    {
        _computeShaderComputeBlending = std::make_shared<Shader>(_renderer, Shader::prgCompute);
        _computeShaderComputeBlending->selectComputePhase(Shader::ComputePhase::ComputeCameraContribution);
    }

    assert(_computeShaderComputeBlending != nullptr);

    _computeShaderComputeBlending->setCulling(_culling);
    _computeShaderComputeBlending->setUniform("_blendWidth", blendWidth);

    _farthestVisibleVertexDistance = 0.f;

    _geometry->update();
    _geometry->activateAsSharedBuffer();

    // Set uniforms
    const auto verticesNbr = _geometry->getVerticesNumber();
    _computeShaderComputeBlending->setUniform("_vertexNbr", verticesNbr);

    const auto mv = viewMatrix * computeModelMatrix();
    const auto mvAsValues = Values(glm::value_ptr(mv), glm::value_ptr(mv) + 16);
    _computeShaderComputeBlending->setUniform("_mv", mvAsValues);

    const auto mvp = projectionMatrix * viewMatrix * computeModelMatrix();
    const auto mvpAsValues = Values(glm::value_ptr(mvp), glm::value_ptr(mvp) + 16);
    _computeShaderComputeBlending->setUniform("_mvp", mvpAsValues);

    const auto mNormal = projectionMatrix * glm::transpose(glm::inverse(viewMatrix * computeModelMatrix()));
    const auto mNormalAsValues = Values(glm::value_ptr(mNormal), glm::value_ptr(mNormal) + 16);
    _computeShaderComputeBlending->setUniform("_mNormal", mNormalAsValues);

    _computeShaderComputeBlending->doCompute(verticesNbr / 3);
    _geometry->deactivate();

    if (_computeFarthestVisibleVertexDistance)
    {
        // Get the annexe buffer from the geometry, and get the farthest projected vertex
        // Note that the annexe buffer holds float32 values
        const auto annexeBufferAsChar = _geometry->getGpuBufferAsVector(Geometry::BufferType::Annexe);
        if (!annexeBufferAsChar.empty())
        {
            size_t annexeBufferSize = annexeBufferAsChar.size() / 4;
            const float* annexePtr = reinterpret_cast<const float*>(annexeBufferAsChar.data());
            std::vector<float> distanceBuffer((size_t)annexeBufferSize / 4);
            // Initialize the distanceBuffer using only the w component of the annexeBuffer
            for (size_t i = 3; i < annexeBufferSize / 4; i += 4)
            {
                // The vertices are farther away from the camera when the distance goes lower,
                // so we need to invert the values from the annexe buffer
                distanceBuffer[(i - 3) / 4] = -annexePtr[i];
            }
            const auto maxDistance = std::max_element(distanceBuffer.cbegin(), distanceBuffer.cend());
            _farthestVisibleVertexDistance = std::max(_farthestVisibleVertexDistance, *maxDistance);
        }
        }
        else
        {
            _farthestVisibleVertexDistance = 0.f;
        }
}

/*************/
void Object::setViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp)
{
    _shader->setModelViewProjectionMatrix(mv * computeModelMatrix(), mp);
}

/*************/
void Object::setShader(const std::shared_ptr<Shader>& shader)
{
    _graphicsShaders["userDefined"] = shader;
    _fill = "userDefined";
    _fillParameters.clear();
}

/*************/
void Object::registerAttributes()
{
    GraphObject::registerAttributes();

    addAttribute("activateVertexBlending",
        [&](const Values& args) {
            _vertexBlendingActive = args[0].as<bool>();
            _geometry->useAlternativeBuffers(_vertexBlendingActive);
            return true;
        },
        {'b'});
    setAttributeDescription("activateVertexBlending", "If true, activate vertex blending");

    addAttribute("computeFarthestVisibleVertexDistance",
        [&](const Values& args) {
            _computeFarthestVisibleVertexDistance = args[0].as<bool>();
            return true;
        },
        {'b'});

    addAttribute("farthestVisibleVertexDistance",
        [&](const Values& args) {
            _farthestVisibleVertexDistance = args[0].as<float>();
            return true;
        },
        {'r'});

    addAttribute(
        "position",
        [&](const Values& args) {
            _position = glm::dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_position.x, _position.y, _position.z};
        },
        {'r', 'r', 'r'});
    setAttributeDescription("position", "Set the object position");

    addAttribute(
        "rotation",
        [&](const Values& args) {
            _rotation = glm::dvec3(args[0].as<float>() * M_PI / 180.0, args[1].as<float>() * M_PI / 180.0, args[2].as<float>() * M_PI / 180.0);
            return true;
        },
        [&]() -> Values {
            return {_rotation.x * 180.0 / M_PI, _rotation.y * 180.0 / M_PI, _rotation.z * 180.0 / M_PI};
        },
        {'r', 'r', 'r'});
    setAttributeDescription("rotation", "Set the object rotation");

    addAttribute(
        "scale",
        [&](const Values& args) {
            if (args.size() < 3)
                _scale = glm::dvec3(args[0].as<float>(), args[0].as<float>(), args[0].as<float>());
            else
                _scale = glm::dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());

            return true;
        },
        [&]() -> Values {
            return {_scale.x, _scale.y, _scale.z};
        },
        {'r'});
    setAttributeDescription("scale", "Set the object scale");

    addAttribute(
        "culling",
        [&](const Values& args) {
            _culling = static_cast<Shader::Culling>(args[0].as<int>());
            return true;
        },
        [&]() -> Values { return {_culling}; },
        {'i'});
    setAttributeDescription("culling", "Set the side culling for the object: 0 for double sided, 1 for front-face visible, 2 for back-face visible");

    addAttribute(
        "fill",
        [&](const Values& args) {
            _fill = args[0].as<std::string>();
            _fillParameters.clear();
            for (uint32_t i = 1; i < args.size(); ++i)
                _fillParameters.push_back(args[i].as<std::string>());
            return true;
        },
        [&]() -> Values { return {_fill}; },
        {'s'});
    setAttributeDescription("fill",
        "Set the fill type (texture, wireframe, or color). A fourth choice is available: userDefined. The fragment shader has to be defined "
        "manually then. Additional parameters are sent as #define directives to the shader compiler.");

    addAttribute(
        "color",
        [&](const Values& args) {
            _color = glm::dvec4(args[0].as<float>(), args[1].as<float>(), args[2].as<float>(), args[3].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_color.r, _color.g, _color.b, _color.a};
        },
        {'r', 'r', 'r', 'r'});
    setAttributeDescription("color", "Set the object color, used for the \"color\" fill mode, or when no texture is linked to the object.");

    addAttribute(
        "normalExponent",
        [&](const Values& args) {
            _normalExponent = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_normalExponent}; },
        {'r'});
    setAttributeDescription("normalExponent", "If set to anything but 0.0, set the exponent applied to the normal factor for blending computation");
}

} // namespace Splash
