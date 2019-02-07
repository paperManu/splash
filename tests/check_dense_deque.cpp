#include <doctest.h>

#include "./utils/dense_deque.h"

using namespace std;
using namespace Splash;

/*************/
TEST_CASE("Testing Splash::DenseDeque")
{
    auto ddeque = DenseDeque<float>();
    CHECK(ddeque.size() == 0);

    ddeque = DenseDeque<float>((size_t)8);
    CHECK(ddeque.size() == 8);

    float integers[] = {1.f, 2.f, 3.f, 4.f};
    ddeque = DenseDeque<float>(integers, integers + 4);
    CHECK(ddeque.size() == 4);
    ddeque = DenseDeque({1.f, 2.f, 3.f, 4.f});
    CHECK(ddeque[3] == 4.f);
    CHECK(ddeque.front() == 1.f);
    CHECK(ddeque.back() == 4.f);

    ddeque.push_back(5.f);
    CHECK(ddeque.back() == 5.f);
    ddeque.emplace_back(6.f);
    CHECK(ddeque.back() == 6.f);
    ddeque.pop_back();
    CHECK(ddeque.back() == 5.f);

    ddeque.push_front(0.f);
    CHECK(ddeque.front() == 0.f);
    ddeque.emplace_front(-1.f);
    CHECK(ddeque.front() == -1.f);
    ddeque.pop_front();
    CHECK(ddeque.front() == 0.f);
}
