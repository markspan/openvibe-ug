/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, see speller.md, 3.3
 */
#pragma once

#include <openvibe/acquisition-messaging/samples.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <utility>
#include <vector>

/**
 * @brief Holds the stimulation stream back behind the signal, so that a stimulation that comes late still gets its own sample.
 *
 * A TCP tag is dated when the stimulus was shown, and reaches the server some time later. If the chunk that holds its sample has
 * gone by then, it comes with a later chunk. A stimulation has to be dated within the chunk it is sent in: boxes count on this
 * (the Stimulation based epoching drops signal by it, and the CSV writer takes a chunk's end to mean no earlier stimulation will
 * come). So the signal goes out at once, and each stimulation chunk goes out some chunks later, with the same times it would have
 * had. A stimulation that comes within that time finds its chunk still open.
 *
 * The signal is not delayed, and neither is an epoch, which has to wait for its signal anyway, as long as it is longer than this.
 */
class StimulationDelay
{
public:
    /// How long a stimulation can be late and still get its own sample. A marker reached the server up to 33 ms after its date
    /// with the generic oscillator; an amplifier's driver can read the tags later, between its blocks of samples.
    static constexpr uint64_t WINDOW_NS = 100000000ULL;

    /// The whole number of chunks that covers the window; at least one, as a stimulation can always miss the chunk it belongs to.
    static size_t chunksFor(const uint64_t sampleCount, const uint64_t sampleRate)
    {
        if (sampleCount == 0 || sampleRate == 0) { return 1; }
        const uint64_t chunkNs = sampleCount * 1000000000ULL / sampleRate;
        return chunkNs == 0 ? 1 : std::max<size_t>(1, size_t((WINDOW_NS + chunkNs - 1) / chunkNs));
    }

    /// The stimulations of one chunk of the stimulation stream, dated in the scenario's time.
    struct Chunk
    {
        uint64_t startTime = 0;
        std::vector<ovStimulation> stims;
    };

    /**
     * @param delayChunks How many chunks the stimulations stay behind the signal.
     * @param sampleTime  The scenario's time from the start of a chunk to the sample with this number.
     */
    void configure(const size_t delayChunks, std::function<uint64_t(size_t)> sampleTime)
    {
        m_delayChunks = delayChunks;
        m_sampleTime  = std::move(sampleTime);
        m_open.clear();
    }

    /**
     * @brief Adds a chunk of signal, and the stimulations that came with it, dated in the server's clock.
     * @param startTime The chunk's start in the scenario's time.
     * @param stamps    The time of each sample, in the server's clock.
     * @param stims     The stimulations that came with this chunk. Each goes to the chunk that holds its sample, or to the first
     *                  sample still open when that chunk has gone.
     */
    void addChunk(const uint64_t startTime, std::vector<uint64_t> stamps, const std::vector<ovStimulation>& stims)
    {
        m_open.push_back({Chunk{startTime, {}}, std::move(stamps)});
        for (const ovStimulation& stim : stims) { place(stim); }
    }

    /// Whether the oldest chunk has waited long enough.
    bool hasDueChunk() const { return m_open.size() > m_delayChunks; }

    /// Removes the oldest chunk, and returns it.
    Chunk takeDueChunk()
    {
        Chunk due = std::move(m_open.front().output);
        m_open.pop_front();
        return due;
    }

private:
    struct OpenChunk
    {
        Chunk output;
        std::vector<uint64_t> stamps;
    };

    // The last sample stamped at or before the stimulation. Of samples sharing a stamp, which is how drift correction stamps the
    // samples it repeats, the first: the original that the stimulation was dated with.
    void place(const ovStimulation& stim)
    {
        size_t chunk = 0, sample = 0;
        bool hasPrevious = false, found = false;
        uint64_t previous = 0;

        for (size_t c = 0; c < m_open.size() && !found; ++c) {
            const std::vector<uint64_t>& stamps = m_open[c].stamps;
            for (size_t i = 0; i < stamps.size(); ++i) {
                if (hasPrevious) {
                    if (stamps[i] < stim.date || (stamps[i] == stim.date && previous != stim.date)) {
                        chunk  = c;
                        sample = i;
                    }
                    else {
                        found = true;
                        break;
                    }
                }
                previous    = stamps[i];
                hasPrevious = true;
            }
        }

        Chunk& target = m_open[chunk].output;
        const ovStimulation placed{stim.id, target.startTime + m_sampleTime(sample), stim.duration};
        target.stims.insert(std::upper_bound(target.stims.begin(), target.stims.end(), placed,
                                             [](const ovStimulation& a, const ovStimulation& b) { return a.date < b.date; }),
                            placed);
    }

    size_t m_delayChunks = 1;
    std::function<uint64_t(size_t)> m_sampleTime;
    std::deque<OpenChunk> m_open;
};
