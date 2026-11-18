// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.40.
// The trace shading of the Signal display and the LSL display (TraceShading.h).

#include "TraceShading.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

using namespace OpenViBE::Plugins::SimpleVisualization;

TEST(TraceShading, aSubColumnPerWholeSampleUpToFour)
{
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 1.0), 1);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 1.9), 1);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 2.0), 2);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 3.5), 3);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 4.0), MAX_SUB_COLUMNS);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 1000.0), MAX_SUB_COLUMNS);
}

TEST(TraceShading, oneColumnWhenSparseUnknownOrTransparent)
{
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, 0.3), 1);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, std::numeric_limits<double>::quiet_NaN()), 1);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Density, std::numeric_limits<double>::infinity()), MAX_SUB_COLUMNS);
    EXPECT_EQ(subColumnsPerPixel(ETraceShading::Transparent, 1000.0), 1);
}

TEST(TraceShading, densityIsOpaqueWithOneColumn)
{
    EXPECT_DOUBLE_EQ(traceOpacity(ETraceShading::Density, 1), 1.0);
    EXPECT_DOUBLE_EQ(traceOpacity(ETraceShading::Density, 0), 1.0);
}

TEST(TraceShading, everySubColumnTogetherReachesTheFullOpacity)
{
    double previous = 1.0;
    for (int k = 2; k <= MAX_SUB_COLUMNS; ++k)
    {
        const double a = traceOpacity(ETraceShading::Density, k);
        EXPECT_GT(a, 0.0);
        EXPECT_LT(a, previous) << "more sub-columns, each lighter";
        EXPECT_NEAR(1.0 - std::pow(1.0 - a, k), DENSITY_FULL_OPACITY, 1e-12) << k << " sub-columns";
        previous = a;
    }
    // A pixel only one of four sub-columns passes, such as the tip of a spike, stays clearly lighter
    EXPECT_LT(traceOpacity(ETraceShading::Density, 4), 0.6);
}

TEST(TraceShading, transparentWhateverTheDensity)
{
    for (int k = 1; k <= MAX_SUB_COLUMNS; ++k) { EXPECT_DOUBLE_EQ(traceOpacity(ETraceShading::Transparent, k), TRANSPARENT_OPACITY); }
}

TEST(TraceShading, namesGoBothWays)
{
    for (const ETraceShading v : TRACE_SHADINGS)
    {
        ETraceShading parsed = v == ETraceShading::Density ? ETraceShading::Transparent : ETraceShading::Density;
        EXPECT_TRUE(parseTraceShading(toString(v), parsed));
        EXPECT_EQ(parsed, v);
    }
    ETraceShading untouched = ETraceShading::Transparent;
    EXPECT_FALSE(parseTraceShading("Solid", untouched));
    EXPECT_EQ(untouched, ETraceShading::Transparent);
}
