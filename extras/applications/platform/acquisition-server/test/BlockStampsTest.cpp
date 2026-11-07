/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. The times of samples handed over from an SDK block in several reads. See UG.md, 2.24.
 */
#include <BlockStamps.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

constexpr uint64_t PERIOD = 976562;              // 1024 Hz, in ns
constexpr uint64_t FETCH  = 50'000'000'000ULL;   // when the block came from the SDK

// Hands over a block of blockCount samples in reads of at most readSize, as a driver does, and returns every sample's stamp
std::vector<uint64_t> handOver(const uint64_t blockCount, const uint64_t readSize, uint64_t last = 0)
{
    std::vector<uint64_t> stamps(blockCount);
    for (uint64_t first = 0; first < blockCount; first += readSize) {
        const uint64_t count = std::min(readSize, blockCount - first);
        stampFromBlock(FETCH, blockCount, first, count, PERIOD, last, stamps.data() + first);
    }
    return stamps;
}

}  // namespace

TEST(BlockStampsTest, aBlockReadInOneGoEndsAtItsFetch)
{
    const auto stamps = handOver(32, 32);
    EXPECT_EQ(stamps.back(), FETCH);
    for (size_t i = 0; i < stamps.size(); ++i) { EXPECT_EQ(stamps[i], FETCH - (31 - i) * PERIOD) << "sample " << i; }
}

// The bug this repairs: a block bigger than one read was stamped per read, so all but its last read came out too late, and
// the later reads' samples were squeezed 1 ns apart.
TEST(BlockStampsTest, aBlockReadInSeveralGoesKeepsTheNominalPeriod)
{
    const auto stamps = handOver(100, 32);
    for (size_t i = 0; i < stamps.size(); ++i) { EXPECT_EQ(stamps[i], FETCH - (99 - i) * PERIOD) << "sample " << i; }
}

TEST(BlockStampsTest, aSampleKeepsItsStampWhateverTheReadSize)
{
    const auto whole = handOver(100, 100);
    for (const uint64_t readSize : {1, 7, 32, 99}) { EXPECT_EQ(handOver(100, readSize), whole) << "reads of " << readSize; }
}

TEST(BlockStampsTest, stampsStayIncreasingAfterAnEarlierBlock)
{
    // The previous block's last stamp is later than this block's first: a fetch that came late, then one on time
    const uint64_t previous = FETCH - 5 * PERIOD;
    const auto stamps       = handOver(10, 4, previous);
    for (size_t i = 0; i < stamps.size(); ++i) { EXPECT_GT(stamps[i], i == 0 ? previous : stamps[i - 1]) << "sample " << i; }
    EXPECT_EQ(stamps.back(), FETCH);
}

TEST(BlockStampsTest, aBlockLongerThanTheClockHasRunDoesNotWrapAround)
{
    uint64_t last = 0;
    std::vector<uint64_t> stamps(3);
    stampFromBlock(PERIOD, 3, 0, 3, PERIOD, last, stamps.data());
    EXPECT_EQ(stamps[0], 1u);
    EXPECT_EQ(stamps[1], 2u);
    EXPECT_EQ(stamps[2], PERIOD);
}
