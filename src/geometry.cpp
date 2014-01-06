#include "geometry.h"

using namespace std;

namespace Splash {

/*************/
Geometry::Geometry()
{
    _mesh.reset(new Mesh());
    _timestamp = _mesh->getTimestamp();
}

/*************/
Geometry::~Geometry()
{
}

/*************/
void Geometry::update()
{
    if (!_mesh)
        return; // No mesh, no update

    // Update the vertex buffers if mesh was updated
    if (_timestamp != _mesh->getTimestamp())
    {
        if (_vertexCoords == 0 || _texCoords == 0)
        {
            glDeleteBuffers(1, &_vertexCoords);
            glDeleteBuffers(1, &_texCoords);

            glGenBuffers(1, &_vertexCoords);
            glBindBuffer(GL_ARRAY_BUFFER, _vertexCoords);
            vector<float> vertices = _mesh->getVertCoords();
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(_vertexCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);
            
            glGenBuffers(1, &_texCoords);
            glBindBuffer(GL_ARRAY_BUFFER, _texCoords);
            vector<float> texcoords = _mesh->getUVCoords();
            glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), texcoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(_texCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, _vertexCoords);
            vector<float> vertices = _mesh->getVertCoords();
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(_vertexCoords, 4, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, _texCoords);
            vector<float> texcoords = _mesh->getUVCoords();
            glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(float), texcoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(_texCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

} // end of namespace
