#pragma once

#include "defines.h"

#include <cmath>
#include <cstdlib>  // fix Unix compatibility

namespace System {
class System_API Math
{
public:

    static bool initializeRandomMachine(size_t randomSeed);

    static size_t randomI();
    // returns a value in [0,upperLimit( -- i.e. upperLimit not included in range
    static size_t randomWithCeiling(const size_t upperLimit);
    static double random0To1();
    static uint64_t random();

private:
    Math() = delete;
};
}  // namespace System
