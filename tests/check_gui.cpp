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
            auto gui = scene.add("gui", "gui");
        });
    });
});

/*************/
int main(int argc, char** argv)
{
    SLog::log.setVerbosity(Log::NONE);
    return bandit::run(argc, argv);
}
