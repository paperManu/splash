/*
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

#include <vector>

#include <doctest.h>

#include "./core/serialize/serialize_mesh.h"
#include "./core/serializer.h"
#include "./mesh/mesh.h"
#include "./utils/osutils.h"

using namespace Splash;

/*************/
class MeshMockup : public Mesh
{
  public:
    MeshMockup()
        : Mesh(nullptr)
    {
    }

    MeshContainer getContainer() const { return _mesh; }
};

/*************/
TEST_CASE("Testing Mesh serialization")
{
    MeshMockup mesh;
    mesh.read(Utils::getCurrentWorkingDirectory() + "/data/cubes.obj");
    auto container = mesh.getContainer();
    CHECK(container.vertices.size() != 0);

    std::vector<uint8_t> serializedMesh;
    Serial::serialize(container, serializedMesh);
    const auto deserializedMesh = Serial::deserialize<Mesh::MeshContainer>(serializedMesh);
    CHECK_EQ(container.name, deserializedMesh.name);
    CHECK_EQ(container.vertices, deserializedMesh.vertices);
    CHECK_EQ(container.uvs, deserializedMesh.uvs);
    CHECK_EQ(container.normals, deserializedMesh.normals);
    CHECK_EQ(container.annexe, deserializedMesh.annexe);
}
