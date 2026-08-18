///-------------------------------------------------------------------------------------------------
/// 
/// \file CTimeTest.hpp
/// \brief Test Definitions for OpenViBE CTime Class.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 11/05/2020.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include <array>
#include <gtest/gtest.h>
#include <openvibe/CTime.hpp>
#include <cmath>

#include "utils.hpp"

//---------------------------------------------------------------------------------------------------
namespace Dataset {

const double HOUR  = 60 * 60;
const double DAY   = 24 * HOUR;
const double WEEK  = 7 * DAY;
const double MONTH = 30 * DAY;
const double YEAR  = 365 * DAY;

/// <summary> time values to test in seconds </summary>
const std::array<double, 40> SECONDS =
{
    0, 0.001, 0.01, 0.1, 0.2, 0.25, 0.5, 1.0, 1.1, 1.5, 2,
    1.000001, 1.999, 1.999999,
    5, 10, 50, 100, 123.456789, 128.0, 500, 1000, 2500, 5000,
    DAY, DAY + 0.03, DAY + 0.999, DAY + 1,
    WEEK, WEEK + 0.03, WEEK + 0.999, WEEK + 1,
    MONTH, MONTH + 0.03, MONTH + 0.999, MONTH + 1,
    YEAR, YEAR + 0.03, YEAR + 0.999, YEAR + 1,
};

/// <summary> Time values to test in fixed point format. </summary>
const std::array<uint64_t, 16> FIXED_POINT = {
    1LL << 8, 1LL << 16, 1L << 19, 1LL << 22, 1LL << 27, 1L << 30, 1LL << 32, 10LL << 32, 100LL << 32,
    123LL << 32, 500LL << 32, 512LL << 32, 1000LL << 32, 1024LL << 32, 2001LL << 32, 5000LL << 32
};

/// <summary> Sampling rates to test. </summary>
const std::array<size_t, 7> SAMPLINGS = { 100, 128, 512, 1000, 1024, 16000, 44100 };

/// <summary> Samples to test. </summary>
const std::array<uint64_t, 14> SAMPLES = { 0, 1, 100, 128, 512, 1000, 1021, 1024, 5005, 12345, 59876, 100000, 717893, 1000001 };

/// <summary> Epoch durations to test. </summary>
const std::array<double, 13> DURATIONS = { 0.01, 0.1, 0.2, 0.25, 0.5, 1.0, 1.1, 1.5, 2, 5, 10, 50, 100 };

/// <summary> We select 10-9 tolerance for seconds, being the best precision capacity of the CTime class. </summary>
const double TOLERANCE = 10e-9;
}   // namespace Dataset
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, constructor_time)
{
    for (const auto ref : Dataset::FIXED_POINT) {
        const OpenViBE::CTime res(ref);
        EXPECT_EQ(res.time(), ref) << ErrorMsg("Constructor with time", ref, res.time());
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, constructor_seconds)
{
    // test construction & conversion second -> fixed point -> second 
    for (const auto ref : Dataset::SECONDS) {
        const auto res = OpenViBE::CTime(ref).toSeconds();
        EXPECT_LT(std::fabs(res - ref), Dataset::TOLERANCE) << ErrorMsg("Constructor with seconds and conversion to seconds", ref, res);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, constructor_sampling)
{
    // test construction & conversion sample -> time -> sample
    for (const auto sampling : Dataset::SAMPLINGS) {
        for (auto ref : Dataset::SAMPLES) {
            const auto res = OpenViBE::CTime(sampling, ref).toSampleCount(sampling);
            EXPECT_EQ(ref, res) << ErrorMsg("Constructor with sampling and sample count and conversion to sample count", ref, res);
        }
    }
    EXPECT_TRUE(OpenViBE::CTime(0, 1) == OpenViBE::CTime::max()) << ErrorMsg("Special Case if sampling = 0", OpenViBE::CTime::max(), OpenViBE::CTime(0, 1));
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, ceil)
{
    // Ceil Test
    const OpenViBE::CTime t0 = OpenViBE::CTime::min().ceil();
    const OpenViBE::CTime t1 = OpenViBE::CTime::max().ceil();
    const OpenViBE::CTime t2 = OpenViBE::CTime(0xFEDCBA9876543210).ceil();
    const OpenViBE::CTime t3 = OpenViBE::CTime(0xFEDCBA9900000000);

    EXPECT_TRUE(t0 == t0) << ErrorMsg("ceil of Time Min (0)", t0, t0);
    EXPECT_TRUE(t1 == t0) << ErrorMsg("ceil of Time Max (0xFFFFFFFFFFFFFFFF)", t0, t1);
    EXPECT_TRUE(t2 == t3) << ErrorMsg("ceil of Time (0xFEDCBA9876543210)", t3, t2);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, operators)
{
    // Operators Tests
    const OpenViBE::CTime t0 = OpenViBE::CTime(1.0) + OpenViBE::CTime(1.0);
    const OpenViBE::CTime t1 = OpenViBE::CTime(4.0) - OpenViBE::CTime(1.0);
    const OpenViBE::CTime t2 = OpenViBE::CTime(1.0) - OpenViBE::CTime(4.0);

    const OpenViBE::CTime r0 = OpenViBE::CTime(2.0);
    const OpenViBE::CTime r1 = OpenViBE::CTime(3.0);
    // we remove 2 second from the max
    // - We have -3 as result, with uint max = -1 so result is max - 2
    // and max seconds = 2^32 - 1 (0xFFFFFFFF) so result is max - 2 : 0xFFFFFFFD so in Fixed Point 0xFFFFFFFD00000000)
    const OpenViBE::CTime r2 = OpenViBE::CTime(0xFFFFFFFD00000000);

    // Comparison
    EXPECT_TRUE(r0 == r0) << ErrorMsg("ref == calc", t0, t0);
    EXPECT_FALSE(r0 != r0) << ErrorMsg("!(ref != calc)", t1, t1);
    EXPECT_TRUE(r0 < r1) << ErrorMsg("(ref < calc)", r0, r1);
    EXPECT_TRUE(r0 <= r1) << ErrorMsg("(ref <= calc)", r0, r1);
    EXPECT_TRUE(r1 > r0) << ErrorMsg("(ref > calc)", r1, r0);
    EXPECT_TRUE(r1 >= r0) << ErrorMsg("(ref >= calc)", r1, r0);

    // Operation
    EXPECT_TRUE(t0 == r0) << ErrorMsg("Addition result", r0, t0);
    EXPECT_TRUE(t1 == r1) << ErrorMsg("Substract result", r1, t1);
    EXPECT_TRUE(t2 == r2) << ErrorMsg("substract with negative result", r2, t2);

    // Str
    EXPECT_EQ("1.000 sec (0x0000000100000000)", OpenViBE::CTime(1.0).str(true, true));
    EXPECT_EQ("1.000 sec", OpenViBE::CTime(1.0).str(true, false));
    EXPECT_EQ("4294967296 (0x0000000100000000)", OpenViBE::CTime(1.0).str(false, true));
    EXPECT_EQ("4294967296", OpenViBE::CTime(1.0).str(false, false));
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, conversion_chain)
{
    // test conversion time -> sample -> time
    for (auto sampling : Dataset::SAMPLINGS) {
        for (auto seconds : Dataset::SECONDS) {
            auto tmp = OpenViBE::CTime(seconds).time();
            // If the sample count would overflow an uint64_t we skip the test
            if (std::log2(sampling) + std::log2(tmp) >= 64) { continue; }

            const uint64_t compute    = OpenViBE::CTime(sampling, OpenViBE::CTime(tmp).toSampleCount(sampling)).time();
            const uint64_t difference = uint64_t(std::abs(int64_t(compute) - int64_t(tmp)));

            EXPECT_LT(OpenViBE::CTime(difference).toSeconds(), (1.0 / double(sampling)))
            << "Time difference too large between OV(" << seconds << ") and " << "SCtoOV(" << sampling << ", OVtoSC(" << sampling << "," << tmp << "))";
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
TEST(CTime_Tests, legacy_epoching)
{
    // compare second -> time conversion to legacy method
    for (const auto duration : Dataset::DURATIONS) {
        const auto legacy = uint64_t(duration * (1LL << 32)); // Legacy code from stimulationBasedEpoching
        const auto calc   = OpenViBE::CTime(duration).time();
        EXPECT_EQ(calc, legacy) << ErrorMsg("Legacy Epoching for duration : " + std::to_string(duration), legacy, calc);
    }
}
//---------------------------------------------------------------------------------------------------
