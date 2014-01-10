#include <bandit/bandit.h>

#include "scene.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Scene class", [&]() {
        Scene scene;
        it("should initialize smoothly", [&]() {
            AssertThat(scene.isInitialized(), Equals(true));
        });
    });

    /*********/
    describe("Camera class", [&]() {
        Scene scene;
        it("should initialize smoothly", [&]() {
            BaseObjectPtr obj = scene.add("camera");
            CameraPtr camera = static_pointer_cast<Camera>(obj);
            AssertThat(camera->isInitialized(), Equals(true));

            vector<TexturePtr> textures = camera->getTextures();
            AssertThat(textures.size(), Equals(1));
        });
    });

    /*********/
    describe("Window class", [&]() {
        Scene scene;
        it("should initialize smoothly", [&]() {
            BaseObjectPtr obj = scene.add("window");
            WindowPtr window = static_pointer_cast<Window>(obj);
            AssertThat(window->isInitialized(), Equals(true));
        });
    });

    /*********/
    describe("A setup with a single window", [&]() {
        Scene scene;
        it("should display and update the window", [&]() {
            scene.add("window", "window");
            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("A single window with a texture", [&]() {
        Scene scene;
        it("should display and update the window", [&]() {
            auto window = scene.add("window", "window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(image, texture);
            scene.link(texture, window);
            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("A setup with a single camera, and a window linked to it", [&]() {
        Scene scene;
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

            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("A setup with a single camera, multiple windows", [&]() {
        Scene scene;
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

            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("A setup with multiple cameras, multiple windows", [&]() {
        Scene scene;
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

            scene.setAttribute("cam2", "eye", vector<float>({-1.f, 1.f, -5.f}));
            scene.setAttribute("cam2", "size", vector<float>({128, 128}));

            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("A single window with a texture, fullscreen", [&]() {
        Scene scene;
        it("should display and update the window", [&]() {
            auto window = scene.add("window", "window");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(image, texture);
            scene.link(texture, window);

            scene.setAttribute("window", "fullscreen", vector<float>({0}));

            while(true)
                if (scene.render())
                    break;
        });
    });

    /*********/
    describe("Two windows with a texture, fullscreen", [&]() {
        Scene scene;
        it("should display and update the window", [&]() {
            auto win1 = scene.add("window", "win1");
            auto win2 = scene.add("window", "win2");
            auto image = scene.add("image");
            auto texture = scene.add("texture");

            scene.link(image, texture);
            scene.link(texture, win1);
            scene.link(texture, win2);

            scene.setAttribute("win1", "fullscreen", vector<float>({0}));
            scene.setAttribute("win2", "fullscreen", vector<float>({1}));

            while(true)
                if (scene.render())
                    break;
        });
    });
    
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
});

/*************/
int main(int argc, char** argv)
{
    SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
