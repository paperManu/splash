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
            for (int i = 0; i < 2000; ++i)
                scene.render();
        });
    });
});

/*************/
int main(int argc, char** argv)
{
    return bandit::run(argc, argv);
}
