/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, see UG.md 2.30
 */
#pragma once

#include <openvibe/acquisition-messaging/samples.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

/**
 * @brief Cuts the samples the acquisition server sends into the chunks of the scenario's signal stream, whatever the size of the
 *        server's blocks. No sample is ever dropped.
 *
 * The server's "Sample count per sent block" can differ from the chunk size the scenario's stream started with: the server can
 * be restarted with another one while a scenario stays connected. Upstream's client then kept the first chunk-size samples of each
 * block and dropped the rest, without a word; a block of 64 in a stream of 4 kept one sample in 16.
 */
class SampleBlocks
{
public:
    /// One chunk of the scenario's stream: channel-major samples, the server's stamp of each sample, and the stimulations that came
    /// with the samples in it.
    struct Chunk
    {
        std::vector<double> samples;
        std::vector<uint64_t> stamps;
        std::vector<ovStimulation> stims;
    };

    /// Starts again, for chunks of @p chunkSamples samples of @p channels channels. Anything not yet taken is dropped.
    void configure(const size_t channels, const size_t chunkSamples)
    {
        m_channels = channels;
        m_chunk    = chunkSamples;
        m_values.clear();
        m_stamps.clear();
        m_stims.clear();
        m_ready.clear();
        m_received = 0;
        m_cut      = 0;
    }

    bool configured() const { return m_channels > 0 && m_chunk > 0; }

    /**
     * @brief Adds a block from the server.
     * @param values     Its samples, sample-major, as the server sends them.
     * @param valueCount The number of values in @p values.
     * @param stamps     The server's stamp of each sample, @p count of them.
     * @param count      The number of samples in the block.
     * @return false, and nothing added, when the block does not hold @p count samples of the configured channels: then the server's
     *         channels are not the stream's, and no sample can be placed.
     */
    bool add(const sampleType* values, const size_t valueCount, const uint64_t* stamps, const size_t count, const ovStimulation* stims,
             const size_t stimCount)
    {
        if (!configured() || valueCount != count * m_channels) { return false; }

        m_values.insert(m_values.end(), values, values + valueCount);
        m_stamps.insert(m_stamps.end(), stamps, stamps + count);
        m_received += count;
        // A stimulation goes with the chunk that holds the last sample of the block it came with, as the server sends it with that
        // block (AbstractWriter::writeSamples()); never with an earlier one, whose samples StimulationDelay would place it in
        for (size_t s = 0; s < stimCount; ++s) { m_stims.push_back({m_received, stims[s]}); }

        while (m_stamps.size() >= m_chunk) { cut(); }
        return true;
    }

    bool hasChunk() const { return !m_ready.empty(); }

    Chunk takeChunk()
    {
        Chunk chunk = std::move(m_ready.front());
        m_ready.pop_front();
        return chunk;
    }

    /// The samples received that do not fill a chunk yet.
    size_t pendingSamples() const { return m_stamps.size(); }

private:
    void cut()
    {
        Chunk chunk;
        chunk.samples.resize(m_channels * m_chunk);
        for (size_t i = 0; i < m_chunk; ++i) {
            for (size_t c = 0; c < m_channels; ++c) { chunk.samples[c * m_chunk + i] = double(m_values[i * m_channels + c]); }
        }
        chunk.stamps.assign(m_stamps.begin(), m_stamps.begin() + std::ptrdiff_t(m_chunk));
        m_values.erase(m_values.begin(), m_values.begin() + std::ptrdiff_t(m_chunk * m_channels));
        m_stamps.erase(m_stamps.begin(), m_stamps.begin() + std::ptrdiff_t(m_chunk));
        m_cut += m_chunk;

        while (!m_stims.empty() && m_stims.front().afterSamples <= m_cut) {
            chunk.stims.push_back(m_stims.front().stim);
            m_stims.pop_front();
        }
        m_ready.push_back(std::move(chunk));
    }

    struct PendingStim
    {
        uint64_t afterSamples;  ///< The samples received when it came
        ovStimulation stim;
    };

    size_t m_channels = 0;
    size_t m_chunk    = 0;
    std::deque<sampleType> m_values;
    std::deque<uint64_t> m_stamps;
    std::deque<PendingStim> m_stims;
    std::deque<Chunk> m_ready;
    uint64_t m_received = 0;  ///< Samples received since configure()
    uint64_t m_cut      = 0;  ///< Samples cut into chunks since configure()
};
