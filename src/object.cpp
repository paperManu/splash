#include "object.h"

#include "image.h"
#include "geometry.h"
#include "log.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "timer.h"


#include <limits>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

namespace Splash {

/*************/
Object::Object()
{
    _type = "object";

    _shader = make_shared<Shader>();

    registerAttributes();
}

/*************/
Object::~Object()
{
#ifdef DEBUG
    SLog::log << Log::DEBUGGING << "Object::~Object - Destructor" << Log::endl;
#endif
}

/*************/
void Object::activate()
{
    if (_geometries.size() == 0)
        return;

    _mutex.lock(); 
    _shader->setAttribute("fill", {_fill});

    if (_blendMaps.size() != 0)
        for (int i = 0; i < _textures.size(); ++i)
            if (_blendMaps[0] == _textures[i])
                _shader->setAttribute("blending", {1});

    _geometries[0]->update();
    _geometries[0]->activate();
    _shader->activate();

    GLuint texUnit = 0;
    for (auto& t : _textures)
    {
        t->update();
        t->lock();
        _shader->setTexture(t, texUnit, string("_tex") + to_string(texUnit));

        // Get texture specific uniforms and send them to the shader
        map<string, Values> texUniforms = t->getShaderUniforms();
        for (auto u : texUniforms)
        {
            Values parameters;
            parameters.push_back(Value(string("_tex") + to_string(texUnit) + "_" + u.first));
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
    for (auto& t : _textures)
    {
        t->flushPbo();
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
    if (dynamic_pointer_cast<Texture>(obj).get() != nullptr)
    {
        TexturePtr tex = dynamic_pointer_cast<Texture>(obj);
        addTexture(tex);
        return true;
    }
    else if (dynamic_pointer_cast<Image>(obj).get() != nullptr)
    {
        TexturePtr tex = make_shared<Texture>();
        if (tex->linkTo(obj))
            return linkTo(tex);
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
void Object::resetBlendingMap()
{
    for (int i = 0; i < _textures.size();)
    {
        bool hasErased {false};
        for (auto& m : _blendMaps)
            if (_textures[i] == m)
            {
                _textures.erase(_textures.begin() + i);
                hasErased = true;
            }
        if (!hasErased)
            ++i;
    }

    _blendMaps.clear();
}

/*************/
void Object::setBlendingMap(TexturePtr& map)
{
    _blendMaps.push_back(map);
    _textures.push_back(map);
}

/*************/
void Object::setViewProjectionMatrix(const glm::dmat4& vp)
{
    _shader->setModelViewProjectionMatrix(vp * computeModelMatrix());
}

/*************/
void Object::registerAttributes()
{
    _attribFunctions["position"] = AttributeFunctor([&](Values args) {
        if (args.size() < 3)
            return false;
        _position = dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    }, [&]() {
        return Values({_position.x, _position.y, _position.z});
    });

    _attribFunctions["scale"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;

        if (args.size() < 3)
            _scale = dvec3(args[0].asFloat(), args[0].asFloat(), args[0].asFloat());
        else
            _scale = dvec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());

        _shader->setAttribute("scale", args);
        return true;
    }, [&]() {
        return Values({_scale.x, _scale.y, _scale.z});
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](Values args) {
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
        return Values({_shader->getSideness()});
    });

    _attribFunctions["fill"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        _fill = args[0].asString();
        return true;
    }, [&]() {
        return Values({_fill});
    });

    _attribFunctions["color"] = AttributeFunctor([&](Values args) {
        if (args.size() < 4)
            return false;
        _shader->setAttribute("color", args);
        return true;
    });

    _attribFunctions["name"] = AttributeFunctor([&](Values args) {
        if (args.size() < 1)
            return false;
        _name = args[0].asString();
        return true;
    });
}

} // end of namespace
