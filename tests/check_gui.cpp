#include <bandit/bandit.h>

#include "world.h"
#include "gui.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Gui class", [&]() {
        Scene scene("check");
        timespec nap({1, (long int)0});
        nanosleep(&nap, 0);
        it("should be visible", [&]() {
            auto window = scene.add("window", "window");
            auto camera = scene.add("camera", "camera");
            auto gui = scene.add("gui", "gui");

            scene.link("camera", "gui");
            scene.link("gui", "window");

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
