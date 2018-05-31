#include "./graphics/object.h"

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

using namespace std;

namespace Splash
{

/*************/
Object::Object(RootObject* root)
    : GraphObject(root)
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

    _shader = make_shared<Shader>();
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
    if (_geometries.size() == 0)
        return;

    _mutex.lock();

    // Create and store the shader depending on its type
    auto shaderIt = _graphicsShaders.find(_fill);
    if (shaderIt == _graphicsShaders.end() && _fill == "userDefined")
    {
        _graphicsShaders["userDefined"] = _shader;
    }
    else if (shaderIt == _graphicsShaders.end())
    {
        _shader = make_shared<Shader>();
        _graphicsShaders[_fill] = _shader;
    }
    else
    {
        _shader = shaderIt->second;
    }

    // Set the shader depending on a few other parameters
    Values shaderParameters{};
    for (uint32_t i = 0; i < _textures.size(); ++i)
        shaderParameters.push_back("TEX_" + to_string(i + 1));

    for (auto& p : _fillParameters)
        shaderParameters.push_back(p);

    if (_fill == "texture")
    {
        if (_vertexBlendingActive)
            shaderParameters.push_back("VERTEXBLENDING");
        if (_textures.size() > 0 && _textures[0]->getType() == "texture_syphon")
            shaderParameters.push_back("TEXTURE_RECT");

        shaderParameters.push_front("texture");
        _shader->setAttribute("fill", shaderParameters);
    }
    else if (_fill == "filter")
    {
        if (_textures.size() > 0 && _textures[0]->getType() == "texture_syphon")
            shaderParameters.push_back("TEXTURE_RECT");

        shaderParameters.push_front("filter");
        _shader->setAttribute("fill", shaderParameters);
    }
    else if (_fill == "window")
    {
        shaderParameters.push_front("window");
        _shader->setAttribute("fill", shaderParameters);
    }
    else
    {
        shaderParameters.push_front(_fill);
        _shader->setAttribute("fill", shaderParameters);
    }

    // Set some uniforms
    _shader->setAttribute("sideness", {_sideness});
    _shader->setAttribute("uniform", {"_normalExp", _normalExponent});
    _shader->setAttribute("uniform", {"_color", _color.r, _color.g, _color.b, _color.a});

    if (_geometries.size() > 0)
    {
        _geometries[0]->update();
        _geometries[0]->activate();
    }
    _shader->activate();

    GLuint texUnit = 0;
    for (auto& t : _textures)
    {
        t->lock();
        _shader->setTexture(t, texUnit, t->getPrefix() + to_string(texUnit));

        // Get texture specific uniforms and send them to the shader
        auto texUniforms = t->getShaderUniforms();
        for (auto u : texUniforms)
        {
            Values parameters;
            parameters.push_back(Value(t->getPrefix() + to_string(texUnit) + "_" + u.first));
            for (auto value : u.second)
                parameters.push_back(value);
            _shader->setAttribute("uniform", parameters);
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
    for (auto& t : _textures)
    {
        // t->flushPbo();
        t->unlock();
    }

    _shader->deactivate();
    if (_geometries.size() > 0)
        _geometries[0]->deactivate();
    _mutex.unlock();
}

/**************/
void Object::addCalibrationPoint(glm::dvec3 point)
{
    for (auto& p : _calibrationPoints)
        if (p == point)
            return;

    _calibrationPoints.push_back(point);
}

/**************/
void Object::removeCalibrationPoint(glm::dvec3 point)
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
    if (_geometries.size() == 0)
        return;

    _shader->updateUniforms();
    glDrawArrays(GL_TRIANGLES, 0, _geometries[0]->getVerticesNumber());
}

/*************/
int Object::getVerticesNumber() const
{
    int nbr = 0;
    for (auto& g : _geometries)
        nbr += g->getVerticesNumber();
    return nbr;
}

/*************/
bool Object::linkTo(const shared_ptr<GraphObject>& obj)
{
    // Mandatory before trying to link
    if (!GraphObject::linkTo(obj))
        return false;

    if (obj->getType().find("texture") != string::npos)
    {
        auto filter = dynamic_pointer_cast<Filter>(_root->createObject("filter", getName() + "_" + obj->getName() + "_filter").lock());
        if (filter->linkTo(obj))
            return linkTo(filter);
        else
            return false;
    }
    else if (obj->getType().find("filter") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("virtual_probe") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("queue") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("image") != string::npos)
    {
        auto filter = dynamic_pointer_cast<Filter>(_root->createObject("filter", getName() + "_" + obj->getName() + "_filter").lock());
        if (filter->linkTo(obj))
            return linkTo(filter);
        else
            return false;
    }
    else if (obj->getType().find("mesh") != string::npos)
    {
        auto geom = dynamic_pointer_cast<Geometry>(_root->createObject("geometry", getName() + "_" + obj->getName() + "_geom").lock());
        if (geom->linkTo(obj))
            return linkTo(geom);
        else
            return false;
    }
    else if (obj->getType().find("geometry") != string::npos)
    {
        auto geom = dynamic_pointer_cast<Geometry>(obj);
        addGeometry(geom);
        return true;
    }

    return false;
}

/*************/
void Object::unlinkFrom(const shared_ptr<GraphObject>& obj)
{
    auto type = obj->getType();
    if (type.find("texture") != string::npos)
    {
        auto filterName = getName() + "_" + obj->getName() + "_filter";

        if (auto filter = _root->getObject(filterName))
        {
            filter->unlinkFrom(obj);
            unlinkFrom(filter);
        }

        _root->disposeObject(filterName);
    }
    else if (type.find("image") != string::npos)
    {
        auto filterName = getName() + "_" + obj->getName() + "_filter";

        if (auto filter = _root->getObject(filterName))
        {
            filter->unlinkFrom(obj);
            unlinkFrom(filter);
        }

        _root->disposeObject(filterName);
    }
    else if (type.find("filter") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }
    else if (obj->getType().find("virtual_screen") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }
    else if (type.find("mesh") != string::npos)
    {
        auto geomName = getName() + "_" + obj->getName() + "_geom";

        if (auto geom = _root->getObject(geomName))
        {
            geom->unlinkFrom(obj);
            unlinkFrom(geom);
        }

        _root->disposeObject(geomName);
    }
    else if (type.find("geometry") != string::npos)
    {
        auto geom = dynamic_pointer_cast<Geometry>(obj);
        removeGeometry(geom);
    }
    else if (obj->getType().find("queue") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }

    GraphObject::unlinkFrom(obj);
}

/*************/
float Object::pickVertex(glm::dvec3 p, glm::dvec3& v)
{
    float distance = numeric_limits<float>::max();
    glm::dvec3 closestVertex;
    float tmpDist;
    for (auto& geom : _geometries)
    {
        glm::dvec3 vertex;
        if ((tmpDist = geom->pickVertex(p, vertex)) < distance)
        {
            distance = tmpDist;
            closestVertex = vertex;
        }
    }

    v = closestVertex;
    return distance;
}

/*************/
void Object::removeGeometry(const shared_ptr<Geometry>& geometry)
{
    auto geomIt = find(_geometries.begin(), _geometries.end(), geometry);
    if (geomIt != _geometries.end())
        _geometries.erase(geomIt);
}

/*************/
void Object::removeTexture(const shared_ptr<Texture>& tex)
{
    auto texIterator = find(_textures.begin(), _textures.end(), tex);
    if (texIterator != _textures.end())
        _textures.erase(texIterator);
}

/*************/
void Object::resetVisibility(int primitiveIdShift)
{
    lock_guard<mutex> lock(_mutex);

    if (!_computeShaderResetVisibility)
    {
        _computeShaderResetVisibility = make_shared<Shader>(Shader::prgCompute);
        _computeShaderResetVisibility->setAttribute("computePhase", {"resetVisibility"});
    }

    if (_computeShaderResetVisibility)
    {
        for (auto& geom : _geometries)
        {
            geom->update();
            geom->activateAsSharedBuffer();
            auto verticesNbr = geom->getVerticesNumber();
            _computeShaderResetVisibility->setAttribute("uniform", {"_vertexNbr", verticesNbr});
            _computeShaderResetVisibility->setAttribute("uniform", {"_primitiveIdShift", primitiveIdShift});
            _computeShaderResetVisibility->doCompute(verticesNbr / 3 / 128 + 1);
            geom->deactivate();
        }
    }
}

/*************/
void Object::resetBlendingAttribute()
{
    lock_guard<mutex> lock(_mutex);

    if (!_computeShaderResetBlendingAttributes)
    {
        _computeShaderResetBlendingAttributes = make_shared<Shader>(Shader::prgCompute);
        _computeShaderResetBlendingAttributes->setAttribute("computePhase", {"resetBlending"});
    }

    if (_computeShaderResetBlendingAttributes)
    {
        for (auto& geom : _geometries)
        {
            geom->update();
            geom->activateAsSharedBuffer();
            auto verticesNbr = geom->getVerticesNumber();
            _computeShaderResetBlendingAttributes->setAttribute("uniform", {"_vertexNbr", verticesNbr});
            _computeShaderResetBlendingAttributes->doCompute(verticesNbr / 3 / 128 + 1);
            geom->deactivate();
        }
    }
}

/*************/
void Object::resetTessellation()
{
    lock_guard<mutex> lock(_mutex);

    for (auto& geom : _geometries)
    {
        geom->useAlternativeBuffers(false);
    }
}

/*************/
void Object::tessellateForThisCamera(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float fovX, float fovY, float blendWidth, float blendPrecision)
{
    lock_guard<mutex> lock(_mutex);

    if (!_feedbackShaderSubdivideCamera)
    {
        _feedbackShaderSubdivideCamera = make_shared<Shader>(Shader::prgFeedback);
        _feedbackShaderSubdivideCamera->setAttribute("feedbackPhase", {"tessellateFromCamera"});
        _feedbackShaderSubdivideCamera->setAttribute("feedbackVaryings", {"GEOM_OUT.vertex", "GEOM_OUT.texcoord", "GEOM_OUT.normal", "GEOM_OUT.annexe"});
    }

    if (_feedbackShaderSubdivideCamera)
    {
        for (auto& geom : _geometries)
        {
            do
            {
                geom->update();
                geom->activate();

                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_blendWidth", blendWidth});
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_blendPrecision", blendPrecision});
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_sideness", _sideness});
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_fov", fovX, fovY});

                auto mv = viewMatrix * computeModelMatrix();
                auto mvAsValues = Values(glm::value_ptr(mv), glm::value_ptr(mv) + 16);
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_mv", mvAsValues});

                auto mvp = projectionMatrix * viewMatrix * computeModelMatrix();
                auto mvpAsValues = Values(glm::value_ptr(mvp), glm::value_ptr(mvp) + 16);
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_mvp", mvpAsValues});

                auto ip = glm::inverse(projectionMatrix);
                auto ipAsValues = Values(glm::value_ptr(ip), glm::value_ptr(ip) + 16);
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_ip", ipAsValues});

                auto mNormal = projectionMatrix * glm::transpose(glm::inverse(viewMatrix * computeModelMatrix()));
                auto mNormalAsValues = Values(glm::value_ptr(mNormal), glm::value_ptr(mNormal) + 16);
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_mNormal", mNormalAsValues});

                geom->activateForFeedback();
                _feedbackShaderSubdivideCamera->activate();
                glDrawArrays(GL_PATCHES, 0, geom->getVerticesNumber());
                _feedbackShaderSubdivideCamera->deactivate();

                geom->deactivateFeedback();
                geom->deactivate();

                glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
            } while (geom->hasBeenResized());

            geom->swapBuffers();
            geom->useAlternativeBuffers(true);
        }
    }
}

/*************/
void Object::transferVisibilityFromTexToAttr(int width, int height, int primitiveIdShift)
{
    lock_guard<mutex> lock(_mutex);

    if (!_computeShaderTransferVisibilityToAttr)
    {
        _computeShaderTransferVisibilityToAttr = make_shared<Shader>(Shader::prgCompute);
        _computeShaderTransferVisibilityToAttr->setAttribute("computePhase", {"transferVisibilityToAttr"});
    }

    for (auto& geom : _geometries)
    {
        geom->update();
        geom->activateAsSharedBuffer();
        _computeShaderTransferVisibilityToAttr->setAttribute("uniform", {"_texSize", (float)width, (float)height});
        _computeShaderTransferVisibilityToAttr->setAttribute("uniform", {"_idShift", primitiveIdShift});
        _computeShaderTransferVisibilityToAttr->doCompute(width / 32 + 1, height / 32 + 1);
        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        geom->deactivate();
    }
}

/*************/
void Object::computeCameraContribution(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth)
{
    lock_guard<mutex> lock(_mutex);

    if (!_computeShaderComputeBlending)
    {
        _computeShaderComputeBlending = make_shared<Shader>(Shader::prgCompute);
        _computeShaderComputeBlending->setAttribute("computePhase", {"computeCameraContribution"});
    }

    if (_computeShaderComputeBlending)
    {
        for (auto& geom : _geometries)
        {
            geom->update();
            geom->activateAsSharedBuffer();

            // Set uniforms
            auto verticesNbr = geom->getVerticesNumber();
            _computeShaderComputeBlending->setAttribute("uniform", {"_vertexNbr", verticesNbr});
            _computeShaderComputeBlending->setAttribute("uniform", {"_sideness", _sideness});
            _computeShaderComputeBlending->setAttribute("uniform", {"_blendWidth", blendWidth});

            auto mvp = projectionMatrix * viewMatrix * computeModelMatrix();
            auto mvpAsValues = Values(glm::value_ptr(mvp), glm::value_ptr(mvp) + 16);
            _computeShaderComputeBlending->setAttribute("uniform", {"_mvp", mvpAsValues});

            auto mNormal = projectionMatrix * glm::transpose(glm::inverse(viewMatrix * computeModelMatrix()));
            auto mNormalAsValues = Values(glm::value_ptr(mNormal), glm::value_ptr(mNormal) + 16);
            _computeShaderComputeBlending->setAttribute("uniform", {"_mNormal", mNormalAsValues});

            _computeShaderComputeBlending->doCompute(verticesNbr / 3);

            geom->deactivate();

            glMemoryBarrier(GL_TRANSFORM_FEEDBACK_BARRIER_BIT);
        }
    }
}

/*************/
void Object::setViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp)
{
    _shader->setModelViewProjectionMatrix(mv * computeModelMatrix(), mp);
}

/*************/
void Object::registerAttributes()
{
    GraphObject::registerAttributes();

    addAttribute("activateVertexBlending",
        [&](const Values& args) {
            _vertexBlendingActive = args[0].as<int>();
            for (auto& geom : _geometries)
                geom->useAlternativeBuffers(_vertexBlendingActive);
            return true;
        },
        {'n'});
    setAttributeDescription("activateVertexBlending", "If set to 1, activate vertex blending");

    addAttribute("position",
        [&](const Values& args) {
            _position = glm::dvec3(args[0].as<float>(), args[1].as<float>(), args[2].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_position.x, _position.y, _position.z};
        },
        {'n', 'n', 'n'});
    setAttributeDescription("position", "Set the object position");

    addAttribute("rotation",
        [&](const Values& args) {
            _rotation = glm::dvec3(args[0].as<float>() * M_PI / 180.0, args[1].as<float>() * M_PI / 180.0, args[2].as<float>() * M_PI / 180.0);
            return true;
        },
        [&]() -> Values {
            return {_rotation.x * 180.0 / M_PI, _rotation.y * 180.0 / M_PI, _rotation.z * 180.0 / M_PI};
        },
        {'n', 'n', 'n'});
    setAttributeDescription("rotation", "Set the object rotation");

    addAttribute("scale",
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
        {'n'});
    setAttributeDescription("scale", "Set the object scale");

    addAttribute("sideness",
        [&](const Values& args) {
            _sideness = args[0].as<int>();
            return true;
        },
        [&]() -> Values { return {_sideness}; },
        {'n'});
    setAttributeDescription("sideness", "Set the side culling for the object: 0 for double sided, 1 for front-face visible, 2 for back-face visible");

    addAttribute("fill",
        [&](const Values& args) {
            _fill = args[0].as<string>();
            _fillParameters.clear();
            for (uint32_t i = 1; i < args.size(); ++i)
                _fillParameters.push_back(args[i].as<string>());
            return true;
        },
        [&]() -> Values { return {_fill}; },
        {'s'});
    setAttributeDescription("fill",
        "Set the fill type (texture, wireframe, or color). A fourth choice is available: userDefinedFilter. The fragment shader has to be defined "
        "manually then. Additional parameters are sent as #define directives to the shader compiler.");

    addAttribute("color",
        [&](const Values& args) {
            _color = glm::dvec4(args[0].as<float>(), args[1].as<float>(), args[2].as<float>(), args[3].as<float>());
            return true;
        },
        [&]() -> Values {
            return {_color.r, _color.g, _color.b, _color.a};
        },
        {'n', 'n', 'n', 'n'});
    setAttributeDescription("color", "Set the object color, used for the \"color\" fill mode, or when no texture is linked to the object.");

    addAttribute("normalExponent",
        [&](const Values& args) {
            _normalExponent = args[0].as<float>();
            return true;
        },
        [&]() -> Values { return {_normalExponent}; },
        {'n'});
    setAttributeDescription("normalExponent", "If set to anything but 0.0, set the exponent applied to the normal factor for blending computation");
}

} // namespace Splash
