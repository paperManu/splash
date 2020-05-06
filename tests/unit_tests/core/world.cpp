#include <doctest.h>

#include <thread>

#include "./core/world.h"

using namespace Splash;
using namespace std::chrono;

/*************/
TEST_CASE("Testing creating a World and running it")
{
    auto context = RootObject::Context();
    context.spawnSubprocesses = false;
    context.configurationFile = "";
    context.unitTest = true;
    World world(context);
    auto worldThread = std::thread([&]() { world.run(); });
    std::this_thread::sleep_for(2s);
    world.setAttribute("quit", {1});
    worldThread.join();
}
