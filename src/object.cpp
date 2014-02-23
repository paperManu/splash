#include "object.h"
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

    _shader.reset(new Shader());

    registerAttributes();
}

/*************/
Object::~Object()
{
    SLog::log << Log::DEBUG << "Object::~Object - Destructor" << Log::endl;
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
                _shader->activateBlending(i);

    for (auto& t : _textures)
        t->update();
    _geometries[0]->update();
    _geometries[0]->activate();
    _shader->activate();

    GLuint texUnit = 0;
    for (auto t : _textures)
    {
        _shader->setTexture(t, texUnit, string("_tex") + to_string(texUnit));
        texUnit++;
    }
}

/*************/
mat4x4 Object::computeModelMatrix() const
{
    return translate(mat4x4(1.f), _position);
}

/*************/
void Object::deactivate()
{
    if (_blendMaps.size() != 0)
        _shader->deactivateBlending();
    _shader->deactivate();
    _geometries[0]->deactivate();
    _mutex.unlock();
}

/*************/
void Object::draw()
{
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
        TexturePtr tex(new Texture());
        if (tex->linkTo(obj))
            return linkTo(tex);
        else
            return false;
    }
    else if (dynamic_pointer_cast<Mesh>(obj).get() != nullptr)
    {
        GeometryPtr geom(new Geometry());
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
float Object::pickVertex(vec3 p, vec3& v)
{
    float distance = numeric_limits<float>::max();
    vec3 closestVertex;
    float tmpDist;
    for (auto& geom : _geometries)
    {
        vec3 vertex;
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
void Object::setViewProjectionMatrix(const glm::mat4& vp)
{
    _shader->setModelViewProjectionMatrix(vp * computeModelMatrix());
}

/*************/
void Object::registerAttributes()
{
    _attribFunctions["position"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _position = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
    }, [&]() {
        return vector<Value>({_position.x, _position.y, _position.z});
    });

    _attribFunctions["scale"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;

        if (args.size() < 3)
            _scale = vec3(args[0].asFloat(), args[0].asFloat(), args[0].asFloat());
        else
            _scale = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());

        _shader->setAttribute("scale", args);
        return true;
    }, [&]() {
        return vector<Value>({_scale.x, _scale.y, _scale.z});
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        switch (args[0].asInt())
        {
        default:
            return false;
        case 0:
            _shader->setSideness(Shader::doubleSided);
            break;
        case 1:
            _shader->setSideness(Shader::singleSided);
            break;
        case 2:
            _shader->setSideness(Shader::inverted);
        }
        return true;
    }, [&]() {
        return vector<Value>({_shader->getSideness()});
    });

    _attribFunctions["fill"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        _fill = args[0].asString();
        return true;
    }, [&]() {
        return vector<Value>({_fill});
    });

    _attribFunctions["color"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 4)
            return false;
        _shader->setAttribute("color", args);
        return true;
    });
}

} // end of namespace
