#include <bandit/bandit.h>

#include "mesh.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Mesh class", []() {
        Mesh mesh;
        before_each([&]() {
            mesh = Mesh();
        });

        it("should get the same vertex coordinates", [&]() {
            vector<float> srcVertices = mesh.getVertCoords();
            Mesh::SerializedObject obj = mesh.serialize();
            mesh.deserialize(obj);
            vector<float> dstVertices = mesh.getVertCoords();

            AssertThat(srcVertices.size(), Equals(dstVertices.size()));

            bool isEqual = true;
            for (int i = 0; i < srcVertices.size(); ++i)
                if (srcVertices[i] != dstVertices[i])
                    isEqual &= false;

            AssertThat(isEqual, Equals(true));
        });

        it("should get the same texture coordinates", [&]() {
            vector<float> srcTexcoords = mesh.getUVCoords();
            Mesh::SerializedObject obj = mesh.serialize();
            mesh.deserialize(obj);
            vector<float> dstTexcoords = mesh.getUVCoords();

            AssertThat(srcTexcoords.size(), Equals(dstTexcoords.size()));

            bool isEqual = true;
            for (int i = 0; i < srcTexcoords.size(); ++i)
                if (srcTexcoords[i] != dstTexcoords[i])
                    isEqual &= false;

            AssertThat(isEqual, Equals(true));
        });

        it("should get the same normal coordinates", [&]() {
            vector<float> src = mesh.getNormals();
            Mesh::SerializedObject obj = mesh.serialize();
            mesh.deserialize(obj);
            vector<float> dst = mesh.getNormals();

            AssertThat(src.size(), Equals(dst.size()));

            bool isEqual = true;
            for (int i = 0; i < src.size(); ++i)
                if (src[i] != dst[i])
                    isEqual &= false;

            AssertThat(isEqual, Equals(true));
        });
    });
});

/*************/
int main(int argc, char* argv[])
{
    return bandit::run(argc, argv);
}
