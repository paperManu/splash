#include "object.h"

using namespace std;

namespace Splash {

/*************/
Object::Object()
{
    _type = "object";

    _shader.reset(new Shader());
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

    // Print the pixel values...
    if (false && _textures.size() > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textures[0]->getTexId());
        ImageBuf img(_textures[0]->getSpec());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, img.localpixels());
        for (ImageBuf::Iterator<unsigned char> p(img); !p.done(); ++p)
        {
            if (!p.exists())
                continue;
            for (int c = 0; c < img.nchannels(); ++c)
                cout << p[c] << " ";
        }
        cout << endl;
    }
    //

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
    _shader->setViewProjectionMatrix(mvp);
}

} // end of namespace
