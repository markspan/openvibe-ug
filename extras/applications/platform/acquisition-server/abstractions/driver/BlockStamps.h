/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. The times of samples that an amplifier's SDK hands over in blocks. See UG.md, 2.24.
 */
#pragma once

#include <cstdint>

/**
 * @brief Stamps samples that a driver hands over from one block of an amplifier's SDK.
 *
 * A driver fetches a block of samples from its SDK, and hands it over in one or more reads, as a read takes at most as many
 * samples as the server's buffer holds. A block's last sample is the newest the SDK had when the block was fetched, so it is
 * stamped with the moment of the fetch, and every other sample one nominal period earlier per sample after it in the block.
 * A sample's stamp so depends on its place in the block, and not on the read that hands it over.
 *
 * Stamps stay strictly increasing across blocks: a stamp that is not after the previous one becomes the previous one plus 1 ns.
 *
 * @param fetchTime  When the SDK returned the block, in ns of the driver's clock.
 * @param blockCount Samples in the block.
 * @param first      Index in the block of the first sample to stamp.
 * @param count      Samples to stamp.
 * @param periodNs   The nominal sample period, in ns.
 * @param last       Stamp of the previous sample handed over; updated to the last stamp given here.
 * @param stamps     Receives the count stamps.
 */
inline void stampFromBlock(const uint64_t fetchTime, const uint64_t blockCount, const uint64_t first, const uint64_t count, const uint64_t periodNs,
                           uint64_t& last, uint64_t* stamps)
{
    for (uint64_t i = 0; i < count; ++i) {
        const uint64_t index   = first + i;
        const uint64_t fromEnd = index < blockCount ? blockCount - 1 - index : 0;
        const uint64_t back    = fromEnd * periodNs;
        uint64_t stamp         = back < fetchTime ? fetchTime - back : 0;
        if (stamp <= last) { stamp = last + 1; }
        stamps[i] = last = stamp;
    }
}
