#include <doctest.h>

#include <thread>

#include "./core/scene.h"

using namespace Splash;
using namespace std::chrono;

/*************/
TEST_CASE("Testing creating a Scene")
{
    auto context = RootObject::Context();
    context.unitTest = true;
    Scene scene(context);
    auto sceneThread = std::thread([&]() { scene.run(); });
    std::this_thread::sleep_for(2s);
    scene.setAttribute("quit", {});
    sceneThread.join();
}
