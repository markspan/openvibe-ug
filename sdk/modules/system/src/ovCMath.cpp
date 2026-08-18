/**
 *
 * @fixme This class could benefit from a serious overhaul, e.g. using randomness from some established library or C11.
 *
 * - Here Linear Congruential Generator is re-implemented to avoid third-party dependencies messing the up the rand() state.
 *   This happened before when we used the global srand() / rand(). It made hard to make repeatable experiments on some
 *   platforms. The generated randomness from the introduced home-made class is not super but it should be sufficient for 
 *   OpenViBE's present use-cases.
 *
 * Other notes
 *
 * - Due to generative process, values generated above L_RAND_MAX may not be dense (verify) 
 * - randomIWithCeiling() may not be dense either
 *
 */
#include "system/ovCMath.h"
#include <cstdlib>
#include <cstring>

namespace System {

class RandomGenerator
{
    size_t m_nextValue = 0;

public:
    static const size_t L_RAND_MAX = 2147483647; // (2^32)/2-1 == 2147483647 (0x7FFFFFFF)

    explicit RandomGenerator(const size_t seed = 1) : m_nextValue(seed) {}

    int rand()
    {
        // Pretty much C99 convention and parameters for a Linear Congruential Generator
        m_nextValue = (m_nextValue * 1103515245 + 12345) & L_RAND_MAX;
        return int(m_nextValue);
    }

    void setSeed(const size_t seed) { m_nextValue = seed; }

    size_t getSeed() const { return m_nextValue; }
};

// Should be only accessed via Math:: calls defined below
static RandomGenerator randomGenerator;

bool Math::initializeRandomMachine(const size_t randomSeed)
{
    randomGenerator.setSeed(size_t(randomSeed));

    // For safety, we install also the C++ basic random engine (it might be useg by dependencies, old code, etc)
    srand(uint32_t(randomSeed));

    return true;
}

size_t Math::randomI() { return size_t(random()); }
size_t Math::randomWithCeiling(const size_t upperLimit) { return size_t(random0To1() * double(upperLimit)); }
double Math::random0To1() { return double(randomGenerator.rand()) / double(RandomGenerator::L_RAND_MAX); }

uint64_t Math::random()
{
    const uint64_t r1 = randomGenerator.rand();
    const uint64_t r2 = randomGenerator.rand();
    const uint64_t r3 = randomGenerator.rand();
    const uint64_t r4 = randomGenerator.rand();
    return (r1 << 24) ^ (r2 << 16) ^ (r3 << 8) ^ (r4);
}

}  // namespace System
