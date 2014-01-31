#include "object.h"
#include "timer.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

namespace Splash {

/*************/
Object::Object()
{
    _type = "object";

    _shader.reset(new Shader());
    _position = vec3(0.f, 0.f, 0.f);

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
mat4x4 Object::computeModelMatrix()
{
    return translate(mat4x4(1.f), _position);
}

/*************/
void Object::deactivate()
{
    _shader->deactivate();
    _geometries[0]->deactivate();
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
void Object::setViewProjectionMatrix(const glm::mat4& mvp)
{
    _shader->setViewProjectionMatrix(mvp * computeModelMatrix());
}

/*************/
void Object::registerAttributes()
{
    _attribFunctions["position"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 3)
            return false;
        _position = vec3(args[0].asFloat(), args[1].asFloat(), args[2].asFloat());
        return true;
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
    });
}

} // end of namespace
