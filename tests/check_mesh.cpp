#include <bandit/bandit.h>

using namespace bandit;

go_bandit([](){
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
