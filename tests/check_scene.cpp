#include <bandit/bandit.h>

#include "scene.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Scene class", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should initialize smoothly", [&]() {
            AssertThat(scene.isInitialized(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("Camera class", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should initialize smoothly", [&]() {
            BaseObjectPtr obj = scene.add("camera");
            CameraPtr camera = static_pointer_cast<Camera>(obj);
            AssertThat(camera->isInitialized(), Equals(true));

            vector<TexturePtr> textures = camera->getTextures();
            AssertThat(textures.size(), Equals(1));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("Window class", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should initialize smoothly", [&]() {
            BaseObjectPtr obj = scene.add("window");
            WindowPtr window = static_pointer_cast<Window>(obj);
            AssertThat(window->isInitialized(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("A setup with a single window", [&]() {
        Scene scene("scene");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should display and update the window", [&]() {
            scene.add("window", "window");
            scene.set("scene", "start", {});
            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("A single window with a texture", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should display and update the window", [&]() {
            auto window = scene.add("window", "window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(image, texture);
            scene.link(texture, window);
            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("A setup with a single camera, and a window linked to it", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should render the camera once", [&]() {
            auto camera = scene.add("camera", "camera");
            auto mesh = scene.add("mesh");
            auto geometry = scene.add("geometry");
            auto object = scene.add("object");
            auto window = scene.add("window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, camera);
            scene.link(camera, window);
            scene.link(image, texture);
            scene.link(texture, object);

            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("A setup with a single camera, multiple windows", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should render the camera once", [&]() {
            auto camera = scene.add("camera", "camera");
            auto mesh = scene.add("mesh");
            auto geometry = scene.add("geometry");
            auto object = scene.add("object");
            auto win1 = scene.add("window");
            auto win2 = scene.add("window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, camera);
            scene.link(camera, win1);
            scene.link(camera, win2);
            scene.link(image, texture);
            scene.link(texture, object);

            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("A setup with multiple cameras, multiple windows", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should render the camera once", [&]() {
            auto cam1 = scene.add("camera", "cam1");
            auto cam2 = scene.add("camera", "cam2");
            auto mesh = scene.add("mesh");
            auto geometry = scene.add("geometry");
            auto object = scene.add("object");
            auto win1 = scene.add("window");
            auto win2 = scene.add("window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, cam1);
            scene.link(object, cam2);
            scene.link(cam1, win1);
            scene.link(cam2, win2);
            scene.link(image, texture);
            scene.link(texture, object);

            scene.set("cam2", "eye", vector<Value>({-1.f, 1.f, -5.f}));
            scene.set("cam2", "size", vector<Value>({2048, 2048}));

            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("A single window with a texture, fullscreen", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should display and update the window", [&]() {
            auto window = scene.add("window", "window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(image, texture);
            scene.link(texture, window);

            scene.set("window", "fullscreen", vector<Value>({0}));

            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });

    /*********/
    describe("Two windows with a texture, fullscreen", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should display and update the window", [&]() {
            auto win1 = scene.add("window", "win1");
            auto win2 = scene.add("window", "win2");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(image, texture);
            scene.link(texture, win1);
            scene.link(texture, win2);

            scene.set("win1", "fullscreen", vector<Value>({0}));
            scene.set("win2", "fullscreen", vector<Value>({1}));

            AssertThat(scene.getStatus(), Equals(true));
        });
        scene.set("check", "quit", {});
    });
});

/*************/
int main(int argc, char** argv)
{
    SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
