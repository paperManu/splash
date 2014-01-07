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
            scene.render();
        });
    });

    /*********/
    describe("A setup with a single camera, no window", [&]() {
        Scene scene;
        it("should render the camera once", [&]() {
            auto camera = scene.add("camera", "camera");
            auto mesh = scene.add("mesh");
            auto geometry = scene.add("geometry");
            auto object = scene.add("object");

            scene.link(mesh, geometry);
            scene.link(geometry, object);
            scene.link(object, camera);
            scene.render();
        });
    });
});

/*************/
int main(int argc, char** argv)
{
    //SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
