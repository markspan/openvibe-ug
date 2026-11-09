/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file. Triggers from a trigger channel, without the values caught while its lines switch. See UG.md, 2.25.
 */
#pragma once

#include <cstdint>
#include <optional>

/**
 * @brief Follows an amplifier's trigger channel sample by sample, and reports a new value only once it has held for two samples.
 *
 * The lines of a trigger port never switch at quite the same moment. Now and then a sample falls in between, and catches a value
 * the sender never sent: switching on to 110, the amplifier may read 108 for one sample (one line still off) and 110 the next.
 * Reported as it comes, that is an extra trigger with a wrong code. So a value counts only once it has held for HOLD_SAMPLES
 * samples in a row. A value that lasts shorter is passed over, but the change it started still dates from it: a trigger is dated
 * at the first sample where the channel left its previous value, the earliest moment the sender's pulse can be seen.
 *
 * A pulse shorter than HOLD_SAMPLES samples is lost: at 2048 Hz that is under 1 ms, at 256 Hz under 8 ms.
 *
 * The caller decides what a change means (TMSi sends the new value, eego the lines that came on), so a change gives the
 * value before it as well. The return to 0 is a change like any other.
 */
class SteadyTrigger
{
public:
    /// How many samples in a row a value has to hold to count.
    static constexpr uint32_t HOLD_SAMPLES = 2;

    struct Change
    {
        uint32_t value;    ///< The value the channel settled on.
        uint32_t previous; ///< The value it held before.
        uint64_t onset;    ///< Stamp of the first sample where the channel left the previous value.
    };

    /// Starts over with the channel at @p value: a first sample at another value is a change.
    void reset(const uint32_t value)
    {
        m_steady  = value;
        m_known   = true;
        m_pending = false;
    }

    /// Starts over not knowing the channel's value: the first sample's value is taken as it is, and not reported.
    void resetUnknown()
    {
        m_known   = false;
        m_pending = false;
    }

    /**
     * @brief Takes the next sample's value of the trigger channel.
     * @param value The trigger channel's value at this sample.
     * @param stamp The sample's stamp.
     * @return The change, at the sample where the new value has held for HOLD_SAMPLES samples; nothing otherwise.
     */
    std::optional<Change> feed(const uint32_t value, const uint64_t stamp)
    {
        if (!m_known) {
            m_steady = value;
            m_known  = true;
            return std::nullopt;
        }
        if (value == m_steady) {
            m_pending = false;  // back where it was: what came in between was no change
            return std::nullopt;
        }
        if (!m_pending) {
            m_pending   = true;
            m_onset     = stamp;
            m_candidate = value;
            m_held      = 1;
        }
        else if (value == m_candidate) { ++m_held; }
        else {
            m_candidate = value;  // still switching: the change keeps its onset
            m_held      = 1;
        }
        if (m_held < HOLD_SAMPLES) { return std::nullopt; }

        const Change change{m_candidate, m_steady, m_onset};
        m_steady  = m_candidate;
        m_pending = false;
        return change;
    }

private:
    uint32_t m_steady    = 0;     ///< The value that last held long enough.
    bool m_known         = true;  ///< False until the first sample when started with resetUnknown().
    bool m_pending       = false; ///< Whether the channel has left m_steady.
    uint32_t m_candidate = 0;     ///< While pending: the value it has now.
    uint32_t m_held      = 0;     ///< While pending: samples in a row at m_candidate.
    uint64_t m_onset     = 0;     ///< While pending: stamp of the first sample away from m_steady.
};
