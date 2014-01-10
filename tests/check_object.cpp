#include <bandit/bandit.h>

#include "scene.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Moving the object around", [&]() {
        Scene scene;
        it("should be no problem", [&]() {
            auto camera = scene.add("camera");
            auto mesh = scene.add("mesh");
            auto geometry = scene.add("geometry");
            auto object = scene.add("object", "obj");
            auto window = scene.add("window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, camera);
            scene.link(camera, window);
            scene.link(image, texture);
            scene.link(texture, object);

            scene.setAttribute("obj", "position", vector<float>({0.5, 0.5, 0.0}));

            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("Loading an object from a file", [&]() {
        Scene scene;
        it("should work with an obj file", [&]() {
            auto camera = scene.add("camera");
            auto mesh = static_pointer_cast<Splash::Mesh>(scene.add("mesh"));
            auto geometry = scene.add("geometry");
            auto object = scene.add("object", "obj");
            auto window = scene.add("window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            mesh->read("//home//manu//sphere.obj");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, camera);
            scene.link(camera, window);
            scene.link(image, texture);
            scene.link(texture, object);

            while(true)
                if (scene.render())
                    break;
        });
    });
});

/*************/
int main(int argc, char** argv)
{
    //SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
