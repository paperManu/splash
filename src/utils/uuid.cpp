#include "./utils/uuid.h"

#include <algorithm>
#include <array>
#include <random>

namespace Splash
{

void UUID::init(bool generate)
{
    if (generate)
    {
        static std::random_device rd;
        static auto seedData = std::array<int, std::mt19937::state_size>{};
        std::generate(seedData.begin(), seedData.end(), std::ref(rd));
        static std::seed_seq seq(seedData.begin(), seedData.end());
        static std::mt19937 generator(seq);
        static uuids::uuid_random_generator gen{generator};
        _uuid = gen();
    }
    else
        _uuid = uuids::uuid();
}

} // namespace Splash
