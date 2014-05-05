#include <bandit/bandit.h>

#include "scene.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Moving the object around", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
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

            scene.set("obj", "position", vector<Value>({0.5, 0.5, 0.0}));

            scene.set("check", "start", {});
            nanosleep(&nap, 0);
            scene.set("check", "quit", {});
            nanosleep(&nap, 0);
            AssertThat(scene.getStatus(), Equals(true));
        });
    });

    /*********/
    describe("Loading an object from a file", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should work with an obj file", [&]() {
            auto camera = scene.add("camera", "camera");
            auto mesh = static_pointer_cast<Splash::Mesh>(scene.add("mesh"));
            auto geometry = scene.add("geometry");
            auto object = scene.add("object", "obj");
            auto window = scene.add("window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            mesh->read("..//data//sphere.obj");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, camera);
            scene.link(camera, window);
            scene.link(image, texture);
            scene.link(texture, object);

            scene.set("camera", "eye", vector<Value>({-4.f, 2.f, 2.f}));
            scene.set("obj", "sideness", vector<Value>({2}));

            scene.set("check", "start", {});
            nanosleep(&nap, 0);
            scene.set("check", "quit", {});
            nanosleep(&nap, 0);
            AssertThat(scene.getStatus(), Equals(true));
        });
    });
});

/*************/
int main(int argc, char** argv)
{
    SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
