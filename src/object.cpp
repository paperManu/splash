#include "object.h"

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
}

/*************/
void Object::activate()
{
    if (_geometries.size() == 0)
        return;

    _geometries[0]->update();
    _geometries[0]->activate();
    _shader->activate(_geometries[0]);

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
void Object::setViewProjectionMatrix(const glm::mat4& mvp)
{
    _shader->setViewProjectionMatrix(mvp * computeModelMatrix());
}

/*************/
void Object::registerAttributes()
{
    _attribFunctions["position"] = AttributeFunctor([&](vector<float> args) {
        if (args.size() < 3)
            return false;
        _position = vec3(args[0], args[1], args[2]);
    });

    _attribFunctions["sideness"] = AttributeFunctor([&](vector<float> args) {
        if (args.size() < 1)
            return false;
        switch ((int)args[0])
        {
        case 0:
            _shader->setSideness(Shader::doubleSided);
            break;
        case 1:
            _shader->setSideness(Shader::singleSided);
            break;
        case 2:
            _shader->setSideness(Shader::inverted);
        }
    });
}

} // end of namespace
