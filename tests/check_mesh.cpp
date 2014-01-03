#include <bandit/bandit.h>

#include "mesh.h"

using namespace bandit;
using namespace Splash;

go_bandit([]() {
    /*********/
    Mesh mesh;
    describe("Mesh class", []() {
        before_each([&]() {
            
        });

        it("should get the same number of vertices", [&]() {
            AssertThat(5, Equals(6));
        });
    });
});

/*************/
int main(int argc, char* argv[])
{
    return bandit::run(argc, argv);
}
