#include "object.h"

#include "image.h"
#include "geometry.h"
#include "log.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "texture_image.h"
#include "timer.h"


#include <limits>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

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

    bool withBlend = false;
    if (_blendMaps.size() != 0)
        for (int i = 0; i < _textures.size(); ++i)
            if (_blendMaps[0] == _textures[i])
            {
                _shader->setAttribute("blending", {1});
                withBlend = true;
            }

    if (_fill == "texture")
    {
        if (_textures.size() > 0 && _textures[0]->getType() == "texture_syphon")
        {
            if (withBlend)
                _shader->setAttribute("fill", {"texture", "BLENDING", "TEXTURE_RECT"});
            else
                _shader->setAttribute("fill", {"texture", "TEXTURE_RECT"});
        }
        else
        {
            if (withBlend)
                _shader->setAttribute("fill", {"texture", "BLENDING"});
            else
                _shader->setAttribute("fill", {"texture"});
        }
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
        _shader->setAttribute("fill", {_fill});

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
dmat4 Object::computeModelMatrix() const
{
    return translate(dmat4(1.f), _position);
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

    if (_blendMaps.size() != 0)
        _shader->setAttribute("blending", {0});

    _shader->deactivate();
    _geometries[0]->deactivate();
    _mutex.unlock();
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
bool Object::linkTo(BaseObjectPtr obj)
{
    // Mandatory before trying to link
    BaseObject::linkTo(obj);

    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        TexturePtr tex = dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        Texture_ImagePtr tex = make_shared<Texture_Image>(_root);
        tex->setName(getName() + "_" + obj->getName() + "_tex");
        if (tex->linkTo(obj))
        {
            _root.lock()->registerObject(tex);
            return linkTo(tex);
        }
        else
            return false;
    }
    else if (dynamic_pointer_cast<Mesh>(obj).get() != nullptr)
    {
        GeometryPtr geom = make_shared<Geometry>();
        if (geom->linkTo(obj))
            return linkTo(geom);
        else
            return false;
    }
    else if (dynamic_pointer_cast<Geometry>(obj).get() != nullptr)
    {
        GeometryPtr geom = dynamic_pointer_cast<Geometry>(obj);
        addGeometry(geom);
        return true;
    }

    return false;
}

/*************/
float Object::pickVertex(dvec3 p, dvec3& v)
{
    float distance = numeric_limits<float>::max();
    dvec3 closestVertex;
    float tmpDist;
    for (auto& geom : _geometries)
    {
        dvec3 vertex;
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
void Object::removeTexture(TexturePtr tex)
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
    _attribFunctions["position"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 3)
            return false;
        _position = dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    }, [&]() -> Values {
        return {_position.x, _position.y, _position.z};
    });

    _attribFunctions["scale"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;

        if (args.size() < 3)
            _scale = dvec3(args[0].asFloat(), args[0].asFloat(), args[0].asFloat());
        else
            _scale = dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());

        _shader->setAttribute("scale", args);
        return true;
    }, [&]() -> Values {
        return {_scale.x, _scale.y, _scale.z};
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        switch (args[0].asInt())
        {
        default:
            return false;
        case 0:
            _shader->setAttribute("sideness", {Shader::doubleSided});
            break;
        case 1:
            _shader->setAttribute("sideness", {Shader::singleSided});
            break;
        case 2:
            _shader->setAttribute("sideness", {Shader::inverted});
        }
        return true;
    }, [&]() {
        Values sideness;
        _shader->getAttribute("sideness", sideness);
        return sideness;
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
        _shader->setAttribute("color", args);
        return true;
    });

    _attribFunctions["name"] = AttributeFunctor([&](const Values& args) {
        if (args.size() < 1)
            return false;
        _name = args[0].asString();
        return true;
    });
}

} // end of namespace
