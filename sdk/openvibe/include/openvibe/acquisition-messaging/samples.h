#pragma once

#include <cstddef> // for size_t  
#include <cstdint> // for uint64_t  

#define TIMESTAMP_INVALID 0xFFFFFFFFFFFFFFFF ///< Define a constant representing an invalid timestamp value.

using sampleType = float; ///< Define an alias for the data type used to store samples (single-precision floating-point).
using timestampType = uint64_t; ///< Define an alias for the data type used to store timestamps (64-bit unsigned integer).

struct ovStimulation
{
    uint64_t id;       ///< Stimulation identifier.
    uint64_t date;     ///< Date in fixed-point 32:32 seconds from start of acquisition.
    uint64_t duration; ///< Duration in fixed-point 32:32 seconds.
};

