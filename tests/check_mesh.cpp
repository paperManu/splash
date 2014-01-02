#include <bandit/bandit.h>

#include "mesh.h"

using namespace bandit;
using namespace Splash;

go_bandit([](){
    Mesh mesh;
    describe("sample test", []() {
        it("should fail", [&]() {
            AssertThat(5, Equals(6));
        });
    });
});

int main(int argc, char* argv[])
{
    return bandit::run(argc, argv);
}
