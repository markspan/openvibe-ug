///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationBasedEpoching.cpp
/// \brief Classes implementation for the Box Stimulation based epoching.
/// \author Jozef Legeny (Mensia Technologies).
/// \version 2.0.
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

#include <cmath>
#include <algorithm>

#include "CBoxAlgorithmStimulationBasedEpoching.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

static const int INPUT_SIGNAL_IDX       = 0;
static const int INPUT_STIMULATIONS_IDX = 1;
static const int OUTPUT_SIGNAL_IDX      = 0;

bool CBoxAlgorithmStimulationBasedEpoching::initialize()
{
    //Number of Cues:
    m_numberOfStimulations = getStaticBoxContext().getSettingCount() - NON_CUE_SETTINGS_COUNT + 1;

    m_epochDurationInSeconds = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const double epochOffset = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_epochDuration = CTime(m_epochDurationInSeconds).time();

    const int epochOffsetSign = (epochOffset > 0) - (epochOffset < 0);
    m_epochOffset             = epochOffsetSign * int64_t(CTime(std::fabs(epochOffset)).time());

    //Stimulation IDs
    m_stimulationIDs.resize(m_numberOfStimulations);
    for (size_t i = 0; i < m_numberOfStimulations; ++i) {
        m_stimulationIDs[i] = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), NON_CUE_SETTINGS_COUNT - 1 + i);
    }

    m_lastReceivedStimulationDate   = 0;
    m_lastStimulationChunkStartTime = 0;
    m_lastSignalChunkEndTime        = 0;

    m_signalDecoder.initialize(*this, 0);
    m_stimDecoder.initialize(*this, 1);

    m_encoder.initialize(*this, 0);

    m_encoder.getInputSamplingRate().setReferenceTarget(m_signalDecoder.getOutputSamplingRate());
    m_nChannel = 0;
    m_sampling = 0;

    m_cachedChunks.clear();

    OV_ERROR_UNLESS_KRF(m_epochDurationInSeconds > 0,
                        "Epocher setting is invalid. Duration (= " << m_epochDurationInSeconds << ") must have a strictly positive value.",
                        Kernel::ErrorType::Internal);

    return true;
}

bool CBoxAlgorithmStimulationBasedEpoching::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_encoder.uninitialize();
    m_stimDecoder.uninitialize();
    m_cachedChunks.clear();
    return true;
}

bool CBoxAlgorithmStimulationBasedEpoching::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmStimulationBasedEpoching::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t chunk = 0; chunk < boxCtx.getInputChunkCount(INPUT_SIGNAL_IDX); ++chunk) {
        OV_ERROR_UNLESS_KRF(m_signalDecoder.decode(chunk), "Failed to decode chunk", Kernel::ErrorType::Internal);
        const CMatrix* iMatrix   = m_signalDecoder.getOutputMatrix();
        uint64_t iChunkStartTime = boxCtx.getInputChunkStartTime(INPUT_SIGNAL_IDX, chunk);
        uint64_t iChunkEndTime   = boxCtx.getInputChunkEndTime(INPUT_SIGNAL_IDX, chunk);

        if (m_signalDecoder.isHeaderReceived()) {
            CMatrix* oMatrix = m_encoder.getInputMatrix();

            m_nChannel              = iMatrix->getDimensionSize(0);
            m_nSamplePerInputBuffer = iMatrix->getDimensionSize(1);

            m_sampling = m_signalDecoder.getOutputSamplingRate();
            OV_ERROR_UNLESS_KRZ(m_sampling, Kernel::LogLevel_Error << "Input sampling frequency is equal to 0. Plugin can not process.",
                                Kernel::ErrorType::Internal);

            m_nSampleCountOutputEpoch = size_t(CTime(m_epochDurationInSeconds).toSampleCount(m_sampling));

            oMatrix->resize(m_nChannel, m_nSampleCountOutputEpoch);

            for (size_t channel = 0; channel < m_nChannel; ++channel) { oMatrix->setDimensionLabel(0, channel, iMatrix->getDimensionLabel(0, channel)); }
            m_encoder.encodeHeader();
            boxCtx.markOutputAsReadyToSend(OUTPUT_SIGNAL_IDX, 0, 0);
        }

        if (m_signalDecoder.isBufferReceived()) {
            OV_ERROR_UNLESS_KRF((iChunkStartTime >= m_lastSignalChunkEndTime), "Stimulation Based Epoching can not work on overlapping signal",
                                Kernel::ErrorType::Internal);
            // Cache the signal data
            m_cachedChunks.emplace_back(iChunkStartTime, iChunkEndTime, new CMatrix());
            m_cachedChunks.back().matrix->copy(*iMatrix);

            m_lastSignalChunkEndTime = iChunkEndTime;
        }

        if (m_signalDecoder.isEndReceived()) {
            m_encoder.encodeEnd();
            boxCtx.markOutputAsReadyToSend(OUTPUT_SIGNAL_IDX, iChunkStartTime, iChunkEndTime);
        }
    }

    for (size_t chunk = 0; chunk < boxCtx.getInputChunkCount(INPUT_STIMULATIONS_IDX); ++chunk) {
        m_stimDecoder.decode(chunk);
        // We only handle buffers and ignore stimulation headers and ends
        if (m_stimDecoder.isBufferReceived()) {
            for (size_t stimulation = 0; stimulation < m_stimDecoder.getOutputStimulationSet()->size(); ++stimulation) {
                if (isWatchedStimulation(m_stimDecoder.getOutputStimulationSet()->getId(stimulation))) {
                    // Stimulations are put into cache, we ignore stimulations that would produce output chunks with negative start date (after applying the offset)
                    const uint64_t date = m_stimDecoder.getOutputStimulationSet()->getDate(stimulation);
                    if (date < m_lastReceivedStimulationDate) {
                        OV_WARNING_K(
                            "Skipping stimulation (received at date " << CTime(date) << ") that predates an already received stimulation (at date "
                            << CTime(m_lastReceivedStimulationDate) << ")");
                    }
                    else if (int64_t(date) + m_epochOffset >= 0 && date > m_lastReceivedStimulationDate) {
                        // Create epoch only if it is complete (at beginning of signal).
                        // Create only one epoch for several stims that are at exactly the same date.
                        m_receivedStimulations.push_back(date);
                        m_lastReceivedStimulationDate = date;
                    }
                }
                m_lastStimulationChunkStartTime = boxCtx.getInputChunkEndTime(INPUT_STIMULATIONS_IDX, chunk);
            }
        }
    }

    // Process the received stimulations
    uint64_t lastProcessedStimDate = 0;

    for (const auto& stimDate : m_receivedStimulations) {
        const uint64_t epochStartTime = uint64_t(int64_t(stimDate) + m_epochOffset);

        // No cache available
        if (m_cachedChunks.empty()) { break; }
        // During normal functioning only chunks that will no longer be useful are deprecated, this is to avoid failure in case of a bug
        if (m_cachedChunks.front().startTime > epochStartTime) {
            OV_WARNING_K("Skipped creating an epoch on a timespan with no signal. The input signal probably contains non-contiguous chunks.");
            break;
        }

        // We only process stimulations for which we have received enough signal to create an epoch
        if (m_lastSignalChunkEndTime >= epochStartTime + m_epochDuration) {
            auto* oBuffer     = m_encoder.getInputMatrix()->getBuffer();
            size_t oBufferIdx = 0;
            size_t idx        = 0;

            auto tStart = m_cachedChunks[idx].startTime;
            auto tEnd   = m_cachedChunks[idx].endTime;

            // Find the first chunk that contains data interesting for the sent epoch
            while (tStart > epochStartTime || tEnd < epochStartTime) {
                idx += 1;
                if (idx == m_cachedChunks.size()) { break; }
                tStart = m_cachedChunks[idx].startTime;
                tEnd   = m_cachedChunks[idx].endTime;
            }

            // If we have found a chunk that contains samples in the current epoch
            if (idx != m_cachedChunks.size()) {
                uint64_t iBufferIdx = CTime(epochStartTime - tStart).toSampleCount(m_sampling);

                while (oBufferIdx < m_nSampleCountOutputEpoch) {
                    const auto oTime = epochStartTime + CTime(m_sampling, oBufferIdx).time();

                    // If we handle non-dyadic sampling rates then we do not have a guarantee that all chunks will be
                    // dated with exact values. We add a bit of wiggle room around the incoming chunks to consider
                    // whether a sample is in them or not. This wiggle room will be of half of the sample duration
                    // on each side.
                    const uint64_t timeTolerance = CTime(m_sampling, 1).time() / 2;
                    if (iBufferIdx == m_nSamplePerInputBuffer) {
                        // advance to beginning of the next cached chunk
                        idx += 1;
                        if (idx == m_cachedChunks.size()) { break; }
                        tStart     = m_cachedChunks[idx].startTime;
                        tEnd       = m_cachedChunks[idx].endTime;
                        iBufferIdx = 0;

                        // Case of non-consecutive chunks
                        if (tStart > oTime + timeTolerance) { break; }
                    }
                    else if (tStart <= oTime + timeTolerance && oTime <= tEnd + timeTolerance) {
                        const auto& iBuffer = m_cachedChunks[idx].matrix->getBuffer();
                        for (size_t channel = 0; channel < m_nChannel; ++channel) {
                            oBuffer[channel * m_nSampleCountOutputEpoch + oBufferIdx] = iBuffer[channel * m_nSamplePerInputBuffer + iBufferIdx];
                        }
                        oBufferIdx += 1;
                        iBufferIdx += 1;
                    }
                    else { OV_ERROR_KRF("Can not construct the output chunk due to internal error", Kernel::ErrorType::Internal); }
                }
            }

            // If the epoch is not complete (due to holes in signal)
            if (oBufferIdx == m_nSampleCountOutputEpoch) {
                m_encoder.encodeBuffer();
                boxCtx.markOutputAsReadyToSend(OUTPUT_SIGNAL_IDX, epochStartTime, epochStartTime + m_epochDuration);
            }
            else { OV_WARNING_K("Skipped creating an epoch on a timespan with no signal. The input signal probably contains non-contiguous chunks."); }

            lastProcessedStimDate = stimDate;
        }
        // We only process stimulations for which we have received enough signal to create an epoch
        // No more complete epochs can be constructed
        else { break; }
    }

    // Remove all stimulations for which the epochs have been constructed and sent
    m_receivedStimulations.erase(std::remove_if(m_receivedStimulations.begin(), m_receivedStimulations.end(),
                                                [&lastProcessedStimDate](const uint64_t& stimulationDate) { return stimulationDate <= lastProcessedStimDate; }),
                                 m_receivedStimulations.end());

    // Deprecate cached chunks which will no longer be used because they are too far back in history compared to received stimulations
    const uint64_t lastUsefulChunkEndTime = m_receivedStimulations.empty() ? m_lastStimulationChunkStartTime : m_receivedStimulations.front();

    auto cutoffTime = int64_t(lastUsefulChunkEndTime) + m_epochOffset;
    if (cutoffTime > 0) {
        m_cachedChunks.erase(std::remove_if(m_cachedChunks.begin(), m_cachedChunks.end(),
                                            [cutoffTime](const SCachedChunk& chunk) { return chunk.endTime < uint64_t(cutoffTime); }), m_cachedChunks.end());
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
