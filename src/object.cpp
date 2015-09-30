#include "object.h"

#include "image.h"
#include "filter.h"
#include "geometry.h"
#include "log.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "texture_image.h"
#include "timer.h"


#include <limits>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace std;

namespace Splash {

/*************/
Object::Object()
{
    init();
}

/*************/
Object::Object(RootObjectWeakPtr root)
       : BaseObject(root)
{
    init();
}

/*************/
void Object::init()
{
    _type = "object";

    _shader = make_shared<Shader>();

    registerAttributes();
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

    for (auto& m : _blendMaps)
        m->update();

    bool withTextureBlend = false;
    if (_blendMaps.size() != 0)
    {
        for (int i = 0; i < _textures.size(); ++i)
            if (_blendMaps[0] == _textures[i])
                withTextureBlend = true;
    }

    // Create and store the shader depending on its type
    auto shaderIt = _graphicsShaders.find(_fill);
    if (shaderIt == _graphicsShaders.end())
    {
        _shader = make_shared<Shader>();
        _graphicsShaders[_fill] = _shader;
    }
    else
    {
        _shader = shaderIt->second;
    }

    // Set the shader depending on a few other parameters
    if (_fill == "texture")
    {
        if (_textures.size() > 0 && _textures[0]->getType() == "texture_syphon")
        {
            if (_vertexBlendingActive)
                _shader->setAttribute("fill", {"texture", "VERTEXBLENDING", "TEXTURE_RECT"});
            else if (withTextureBlend)
                _shader->setAttribute("fill", {"texture", "BLENDING", "TEXTURE_RECT"});
            else
                _shader->setAttribute("fill", {"texture", "TEXTURE_RECT"});
        }
        else
        {
            if (_vertexBlendingActive)
                _shader->setAttribute("fill", {"texture", "VERTEXBLENDING"});
            else if (withTextureBlend)
                _shader->setAttribute("fill", {"texture", "BLENDING"});
            else
                _shader->setAttribute("fill", {"texture"});
        }
    }
    else if (_fill == "filter")
    {
        _shader->setAttribute("fill", {"filter"});
    }
    else if (_fill == "window")
    {
        if (_textures.size() == 1)
            _shader->setAttribute("fill", {"window", "TEX_1"});
        else if (_textures.size() == 2)
            _shader->setAttribute("fill", {"window", "TEX_1", "TEX_2"});
        else if (_textures.size() == 3)
            _shader->setAttribute("fill", {"window", "TEX_1", "TEX_2", "TEX_3"});
        else if (_textures.size() == 4)
            _shader->setAttribute("fill", {"window", "TEX_1", "TEX_2", "TEX_3", "TEX_4"});
    }
    else
    {
        _shader->setAttribute("fill", {_fill});
        _shader->setAttribute("uniform", {"_color", _color.r, _color.g, _color.b, _color.a});
    }

    // Set some uniforms
    _shader->setAttribute("sideness", {_sideness});
    _shader->setAttribute("scale", {_scale.x, _scale.y, _scale.z});

    _geometries[0]->update();
    _geometries[0]->activate();
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
    return glm::translate(glm::dmat4(1.f), _position);
}

/*************/
void Object::deactivate()
{
    for (auto& m : _blendMaps)
    {
        auto m_asTexImage = dynamic_pointer_cast<Texture_Image>(m);
        if (m_asTexImage)
            m_asTexImage->flushPbo();
    }

    for (auto& t : _textures)
    {
        //t->flushPbo();
        t->unlock();
    }

    _shader->deactivate();
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
bool Object::linkTo(shared_ptr<BaseObject> obj)
{
    // Mandatory before trying to link
    if (!BaseObject::linkTo(obj))
        return false;

    if (obj->getType().find("texture") != string::npos)
    {
        auto filter = make_shared<Filter>(_root);
        filter->setName(getName() + "_" + obj->getName() + "_tex");
        if (filter->linkTo(obj))
        {
            _root.lock()->registerObject(filter);
            return linkTo(filter);
        }
        else
        {
            return false;
        }
    }
    else if (obj->getType().find("filter") != string::npos)
    {
        auto tex = dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (obj->getType().find("image") != string::npos)
    {
        auto filter = make_shared<Filter>(_root);
        filter->setName(getName() + "_" + obj->getName() + "_filter");
        if (filter->linkTo(obj))
        {
            _root.lock()->registerObject(filter);
            return linkTo(filter);
        }
        else
        {
            return false;
        }
    }
    else if (obj->getType().find("mesh") != string::npos)
    {
        auto geom = make_shared<Geometry>(_root);
        geom->setName(getName() + "_" + obj->getName() + "_geom");
        if (geom->linkTo(obj))
        {
            _root.lock()->registerObject(geom);
            return linkTo(geom);
        }
        else
        {
            return false;
        }
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
bool Object::unlinkFrom(shared_ptr<BaseObject> obj)
{
    auto type = obj->getType();
    if (type.find("texture") != string::npos)
    {
        TexturePtr tex = dynamic_pointer_cast<Texture>(obj);
        removeTexture(tex);
    }
    else if (type.find("image") != string::npos)
    {
        auto textureName = getName() + "_" + obj->getName() + "_tex";
        auto tex = _root.lock()->unregisterObject(textureName);

        if (!tex)
            return false;
        else if (tex->unlinkFrom(obj))
            unlinkFrom(tex);
        else
            return false;
    }
    else if (type.find("mesh") != string::npos)
    {
        auto geomName = getName() + "_" + obj->getName() + "_geom";
        auto geom = _root.lock()->unregisterObject(geomName);

        if (!geom)
            return false;
        if (geom->unlinkFrom(obj))
            unlinkFrom(geom);
        else
            return false;
    }
    else if (type.find("geometry") != string::npos)
    {
        auto geom = dynamic_pointer_cast<Geometry>(obj);
        removeGeometry(geom);
        return true;
    }

    return BaseObject::unlinkFrom(obj);
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
void Object::resetBlendingMap()
{
    for (vector<TexturePtr>::iterator textureIt = _textures.begin(); textureIt != _textures.end();)
    {
        bool hasErased {false};
        for (auto& m : _blendMaps)
            if (*textureIt == m)
            {
                textureIt = _textures.erase(textureIt);
                hasErased = true;
            }
        if (!hasErased)
            textureIt++;
    }

    _blendMaps.clear();
    _updatedParams = true;
}

/*************/
void Object::resetVisibility()
{
    unique_lock<mutex> lock(_mutex);

    if (!_computeShaderResetBlending)
    {
        _computeShaderResetBlending = make_shared<Shader>(Shader::prgCompute);
        _computeShaderResetBlending->setAttribute("computePhase", {"resetVisibility"});
    }

    if (_computeShaderResetBlending)
    {
        for (auto& geom : _geometries)
        {
            geom->update();
            geom->activateAsSharedBuffer();
            auto verticesNbr = geom->getVerticesNumber();
            _computeShaderResetBlending->setAttribute("uniform", {"_vertexNbr", verticesNbr});
            unsigned int groupCountX = verticesNbr / 3 / 128;
            _computeShaderResetBlending->doCompute(groupCountX, 128);
            geom->deactivate();
        }
    }
}

/*************/
void Object::resetTessellation()
{
    unique_lock<mutex> lock(_mutex);

    for (auto& geom : _geometries)
    {
        geom->useAlternativeBuffers(false);
    }
}

/*************/
void Object::tessellateForThisCamera(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth, float blendPrecision)
{
    unique_lock<mutex> lock(_mutex);

    if (!_feedbackShaderSubdivideCamera)
    {
        _feedbackShaderSubdivideCamera = make_shared<Shader>(Shader::prgFeedback);
        _feedbackShaderSubdivideCamera->setAttribute("feedbackPhase", {"tessellateFromCamera"});
        _feedbackShaderSubdivideCamera->setAttribute("feedbackVaryings", {"GEOM_OUT.vertex",
                                                                          "GEOM_OUT.texcoord",
                                                                          "GEOM_OUT.normal",
                                                                          "GEOM_OUT.annexe"});
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

                auto mvp = projectionMatrix * viewMatrix * computeModelMatrix();
                auto mvpAsValues = Values(glm::value_ptr(mvp), glm::value_ptr(mvp) + 16);
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_mvp", mvpAsValues});

                auto mNormal = projectionMatrix * glm::transpose(glm::inverse(viewMatrix * computeModelMatrix()));
                auto mNormalAsValues = Values(glm::value_ptr(mNormal), glm::value_ptr(mNormal) + 16);
                _feedbackShaderSubdivideCamera->setAttribute("uniform", {"_mNormal", mNormalAsValues});

                geom->activateForFeedback();
                _feedbackShaderSubdivideCamera->activate();
                glDrawArrays(GL_PATCHES, 0, geom->getVerticesNumber());
                _feedbackShaderSubdivideCamera->deactivate();

                geom->deactivateFeedback();
                geom->deactivate();
            } while (geom->hasBeenResized());

            geom->swapBuffers();
            geom->useAlternativeBuffers(true);
        }
    }
}

/*************/
void Object::transferVisibilityFromTexToAttr(int width, int height)
{
    unique_lock<mutex> lock(_mutex);

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
        _computeShaderTransferVisibilityToAttr->doCompute(width / 32 + 1, height / 32 + 1);
        geom->deactivate();
    }
}

/*************/
void Object::computeVisibility(glm::dmat4 viewMatrix, glm::dmat4 projectionMatrix, float blendWidth)
{
    unique_lock<mutex> lock(_mutex);

    if (!_computeShaderComputeBlending)
    {
        _computeShaderComputeBlending = make_shared<Shader>(Shader::prgCompute);
        _computeShaderComputeBlending->setAttribute("computePhase", {"computeVisibility"});
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

            unsigned int groupCountX = verticesNbr / 3 / (32 * 32) + 1;
            _computeShaderComputeBlending->doCompute(groupCountX, 32);
            geom->deactivate();
        }
    }
}

/*************/
void Object::setBlendingMap(TexturePtr map)
{
    _blendMaps.push_back(map);
    _textures.push_back(map);
}

/*************/
void Object::setViewProjectionMatrix(const glm::dmat4& mv, const glm::dmat4& mp)
{
    _shader->setModelViewProjectionMatrix(mv * computeModelMatrix(), mp);
}

/*************/
void Object::registerAttributes()
{
    _attribFunctions["activateVertexBlending"] = AttributeFunctor([&](const Values& args) {
        if (args.size() != 1)
            return false;
        _vertexBlendingActive = args[0].asInt();
        return true;
    });
    
    _attribFunctions["position"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 3)
            return false;
        _position = glm::dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    }, [&]() -> Values {
        return {_position.x, _position.y, _position.z};
    });

    _attribFunctions["scale"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;

        if (args.size() < 3)
            _scale = glm::dvec3(args[0].asFloat(), args[0].asFloat(), args[0].asFloat());
        else
            _scale = glm::dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());

        return true;
    }, [&]() -> Values {
        return {_scale.x, _scale.y, _scale.z};
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;

        _sideness = args[0].asInt();
        return true;
    }, [&]() -> Values {
        return {_sideness};
    });

    _attribFunctions["fill"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _fill = args[0].asString();
        return true;
    }, [&]() -> Values {
        return {_fill};
    });

    _attribFunctions["color"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 4)
            return false;
        _color = glm::dvec4(args[0].asFloat(), args[1].asFloat(), args[2].asFloat(), args[3].asFloat());
        return true;
    });
}

} // end of namespace
