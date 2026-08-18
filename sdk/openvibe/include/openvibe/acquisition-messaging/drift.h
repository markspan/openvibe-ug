#pragma once
/**
 * @file drift.h
 * @author Florent LERAY 
 * 
 */

#include <cstdint> // for uint64_t

struct drift_st
{
    int64_t startLatency; ///< Start latency in nanoseconds.
    int64_t globalDrift; ///< Global drift in nanoseconds.
    int64_t instantDrift; ///< Instant drift in nanoseconds.
    int64_t endLatency; ///< End latency in nanoseconds.
};
