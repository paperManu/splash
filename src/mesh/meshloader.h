/*
 * Copyright (C) 2015 Splash authors
 *
 * This file is part of Splash.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Splash is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Splash.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @meshloader.h
 * A simple and Splash-oriented Wavefront OBJ loader
 */

#ifndef SPLASH_MESHLOADER_H
#define SPLASH_MESHLOADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../utils/log.h"

namespace Splash
{
namespace Loader
{

/**********/
class Base
{
  public:
    virtual ~Base(){};

    virtual bool load(const std::string& filename) = 0;
    virtual std::vector<glm::vec4> getVertices() const = 0;
    virtual std::vector<glm::vec2> getUVs() const = 0;
    virtual std::vector<glm::vec4> getNormals() const = 0;
    virtual std::vector<std::vector<int>> getFaces() const = 0;
};

/**********/
class Obj final : public Base
{
  public:
    ~Obj() final = default;

    /**
     * Remove trailing whitespaces
     * \param str Input string
     * \return Return the input string without trailing whitespaces
     */
    std::string removeTrailingSpaces(const std::string& str)
    {
        auto output = str;
        while (output.back() == ' ')
            output.pop_back();
        return output;
    }

    /**
     * Replace multiple consecutive spaces with a single one
     * \param str Input string
     * \return Return the input string without multiple consecutive spaces
     */
    std::string compressSpaces(const std::string& str)
    {
        auto output = str;
        size_t pos{0};
        while ((pos = output.find("  ")) != std::string::npos)
            output.replace(pos, 2, " ");
        return output;
    }

    /**
     * Remove trailing slashes at the end of a face vertex definition
     * i.e: f 1/1/ 2/2/ 3/3/
     * \param str Input string
     * \return Return the input string without the trailing slashes
     */
    std::string removeTrailingSlashes(const std::string& str)
    {
        auto output = str;
        size_t pos{0};
        while ((pos = output.find("/ ")) != std::string::npos)
            output.replace(pos, 2, " ");
        while (output.back() == '/')
            output.pop_back();
        return output;
    }

    /**
     * Load the obj file given its filename
     * \param filename Filename
     * \return Return true if the file has been loaded correctly
     */
    bool load(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::in);
        if (!file.is_open())
            return false;

        _vertices.clear();
        _uvs.clear();
        _normals.clear();
        _faces.clear();

        for (std::string line; std::getline(file, line);)
        {
            auto fullLine = line;
            line = removeTrailingSpaces(line);
            line = compressSpaces(line);
            line = removeTrailingSlashes(line);

            std::string::size_type pos;
            if (line.find("o ") == 0)
            {
                continue;
            }
            else if ((pos = line.find("v ")) == 0)
            {
                pos += 1;
                glm::vec4 vertex;
                int index = 0;
                do
                {
                    pos++;
                    line = line.substr(pos);
                    vertex[index] = std::stof(line);
                    index++;
                    pos = line.find(' ');
                } while (pos != std::string::npos && index < 4);

                if (index < 3)
                    vertex[3] = 1.f;

                _vertices.push_back(vertex);
            }
            else if ((pos = line.find("vt ")) == 0)
            {
                pos += 2;
                glm::vec2 uv;
                int index = 0;
                do
                {
                    pos++;
                    line = line.substr(pos);
                    uv[index] = std::stof(line);
                    index++;
                    pos = line.find(' ');
                } while (pos != std::string::npos && index < 2);

                _uvs.push_back(uv);
            }
            else if ((pos = line.find("vn ")) == 0)
            {
                pos += 2;
                glm::vec4 normal;
                int index = 0;
                do
                {
                    pos++;
                    line = line.substr(pos);
                    normal[index] = std::stof(line);
                    index++;
                    pos = line.find(' ');
                } while (pos != std::string::npos && index < 3);

                normal[3] = 0.f;
                _normals.push_back(normal);
            }
            else if ((pos = line.find("f ")) == 0)
            {
                pos += 1;

                std::vector<FaceVertex> face;
                std::string::size_type nextSlash, nextSpace;
                do
                {
                    pos++;
                    line = line.substr(pos);
                    nextSpace = line.find(' ');

                    FaceVertex faceVertex;
                    faceVertex.vertexId = std::stoi(line) - 1;

                    nextSlash = line.find('/');
                    if (nextSlash != std::string::npos && (nextSpace == std::string::npos || nextSlash < nextSpace))
                    {
                        line = line.substr(nextSlash + 1);
                        nextSlash = line.find('/');
                        if (nextSlash != 0)
                        {
                            nextSpace = line.find(' ');
                            faceVertex.uvId = std::stoi(line) - 1;
                        }
                    }
                    else
                    {
                        nextSlash = line.find('/');
                    }

                    if (nextSlash != std::string::npos && (nextSpace == std::string::npos || nextSlash < nextSpace))
                    {
                        line = line.substr(nextSlash + 1);
                        nextSpace = line.find(' ');
                        faceVertex.normalId = std::stoi(line) - 1;
                    }

                    face.push_back(faceVertex);

                    pos = nextSpace;
                } while (pos != std::string::npos);

                // We triangulate faces right away if needed
                // Only tris and quads are supported
                if (face.size() == 3)
                {
                    _faces.push_back(face);
                }
                else if (face.size() >= 4)
                {
                    std::vector<FaceVertex> newFace;
                    newFace.push_back(face[0]);
                    newFace.push_back(face[1]);
                    newFace.push_back(face[2]);
                    _faces.push_back(newFace);

                    newFace.clear();
                    newFace.push_back(face[2]);
                    newFace.push_back(face[3]);
                    newFace.push_back(face[0]);
                    _faces.push_back(newFace);
                }
            }
        }

        // Check that we have faces, vertices and UVs
        if (_vertices.size() == 0 || _faces.size() == 0)
        {
            _vertices.clear();
            _faces.clear();
            _uvs.clear();
            _normals.clear();

            return false;
        }

        return true;
    }

    /**
     * Get the vertices of the loaded obj file
     * \return Return a vector of vertices
     */
    std::vector<glm::vec4> getVertices() const
    {
        std::vector<glm::vec4> vertices;

        for (auto& face : _faces)
        {
            vertices.push_back(_vertices[face[0].vertexId]);
            vertices.push_back(_vertices[face[1].vertexId]);
            vertices.push_back(_vertices[face[2].vertexId]);
        }

        return vertices;
    }

    /**
     * Get the UV coordinates of the loaded obj file
     * \return Return a vector of UVs
     */
    std::vector<glm::vec2> getUVs() const
    {
        std::vector<glm::vec2> uvs;

        for (auto& face : _faces)
        {
            if (face[0].uvId == -1)
                for (uint32_t i = 0; i < face.size(); ++i)
                    uvs.push_back(glm::vec2(0.f, 0.f));
            else
            {
                uvs.push_back(_uvs[face[0].uvId]);
                uvs.push_back(_uvs[face[1].uvId]);
                uvs.push_back(_uvs[face[2].uvId]);
            }
        }

        return uvs;
    }

    /**
     * Get the normals of the loaded obj file
     * \return Return a vector of normals
     */
    std::vector<glm::vec4> getNormals() const
    {
        std::vector<glm::vec4> normals;

        for (auto& face : _faces)
        {
            if (face[0].normalId == -1)
            {
                auto edge1 = glm::vec3(_vertices[face[1].vertexId] - _vertices[face[0].vertexId]);
                auto edge2 = glm::vec3(_vertices[face[2].vertexId] - _vertices[face[0].vertexId]);
                auto normal = glm::vec4(glm::normalize(glm::cross(edge1, edge2)), 0.0);

                normals.push_back(normal);
                normals.push_back(normal);
                normals.push_back(normal);
            }
            else
            {
                normals.push_back(_normals[face[0].normalId]);
                normals.push_back(_normals[face[1].normalId]);
                normals.push_back(_normals[face[2].normalId]);
            }
        }

        return normals;
    }

    /**
     * Get the face indices of the loaded obj file
     * \return Return the face definitions
     */
    std::vector<std::vector<int>> getFaces() const { return std::vector<std::vector<int>>(); }

  private:
    std::vector<glm::vec4> _vertices;
    std::vector<glm::vec2> _uvs;
    std::vector<glm::vec4> _normals;

    struct FaceVertex
    {
        int vertexId{-1};
        int uvId{-1};
        int normalId{-1};
    };
    std::vector<std::vector<FaceVertex>> _faces;
};

} // namespace Loader
} // namespace Splash

#endif
