#include <bandit/bandit.h>

#include "world.h"
#include "gui.h"

using namespace std;
using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    describe("Gui class", [&]() {
        Scene scene;
        it("should be visible", [&]() {
            auto window = scene.add("window", "window");
            auto camera = scene.add("camera", "camera");
            auto gui = scene.add("gui", "gui");

            scene.link("camera", "gui");
            scene.link("gui", "window");

            for (int i = 0; i < 500; ++i)
                scene.render();
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
