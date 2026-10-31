/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "AbstractWriter.h"
#include <utility>
#include <iostream>

static constexpr uint32_t MAX_PENDING_STIM_COUNT = 128;

AbstractWriter::AbstractWriter(const std::string& writerName) : m_name(writerName)
{
    m_channelCount = 0;
    m_chunkSamplesCount = 0;

    m_msg.msgType = ovMsg::OV_MSG_SAMPLES;
    m_msg.msgData.samplesStruct.count        = 0;
    m_msg.msgData.samplesStruct.stimCount    = 0;
    m_msg.msgData.samplesStruct.timestamp    = nullptr;
    m_msg.msgData.samplesStruct.samples      = nullptr;
    m_msg.msgData.samplesStruct.stimulations = nullptr;
    m_msg.msgSize = sizeof(ovMsg);
}

AbstractWriter::~AbstractWriter()
{
    delete[] m_msg.msgData.samplesStruct.timestamp;
    delete[] m_msg.msgData.samplesStruct.samples;
    delete[] m_msg.msgData.samplesStruct.stimulations;
}

bool AbstractWriter::init(uint32_t chunkSamplesCount, uint32_t chanelCount)
{
    bool bRes = false; ///< Initialize the result variable to false.

    m_channelCount = chanelCount; ///< Set the number of channels.
    m_chunkSamplesCount = chunkSamplesCount; ///< Set the number of samples in each chunk.

    if (m_chunkSamplesCount)
    {
        m_msg.msgType = ovMsg::OV_MSG_SAMPLES;
        m_msg.msgData.samplesStruct.count        = 0;
        m_msg.msgData.samplesStruct.stimCount    = 0;
        m_msg.msgData.samplesStruct.timestamp    = new timestampType[m_chunkSamplesCount];
        m_msg.msgData.samplesStruct.samples      = new sampleType[m_chunkSamplesCount * m_channelCount];
        m_msg.msgData.samplesStruct.stimulations = new ovStimulation[MAX_PENDING_STIM_COUNT];
        m_msg.msgSize = 4 * sizeof(uint32_t)
                      + sizeof(timestampType) * m_chunkSamplesCount
                      + sizeof(sampleType) * m_chunkSamplesCount * m_channelCount;
        bRes = m_msg.msgData.samplesStruct.timestamp    != nullptr
            && m_msg.msgData.samplesStruct.samples      != nullptr
            && m_msg.msgData.samplesStruct.stimulations != nullptr;
    }

    return bRes;
}

size_t AbstractWriter::writeAcquisitionConfig(uint32_t channelCount, uint32_t sampleRate, uint32_t sampleChunk,
                                              const ovChannelInfo& channels, uint64_t clientId)
{
    // UG fork: the configuration carries the channel names, which upstream does not send, so that Designer shows
    // them, and the montage that named them, so that a recording gets its electrode positions (ovChannelInfo.h)
    const std::string names = encodeChannelInfo(channels, channelCount);

    ovMsg msg; ///< Create a message to send.
    msg.msgType = ovMsg::OV_MSG_ACQUISITION_CONFIG; ///< Set the message type to acquisition configuration.
    msg.msgSize = uint32_t(5 * sizeof(uint32_t) + names.size()); ///< Set the message size to the size of the message on the wire.
    msg.msgData.acquisitionConfigStruct.channelCount = channelCount; ///< Set the number of channels in the message.
    msg.msgData.acquisitionConfigStruct.sampleRate = sampleRate; ///< Set the sample rate in the message.
    msg.msgData.acquisitionConfigStruct.sampleChunk = sampleChunk; ///< Set the sample chunk size in the message.
    msg.msgData.acquisitionConfigStruct.channelNames = names.data();
    msg.msgData.acquisitionConfigStruct.channelNamesSize = uint32_t(names.size());
    return send(&msg, clientId);
}

size_t AbstractWriter::writeDrifts(drift_st const & drift)
{
    ovMsg msg; ///< Create a message to send.
    msg.msgType = ovMsg::OV_MSG_DRIFT; ///< Set the message type to drift.
    msg.msgSize = sizeof(m_msg.msgType) + sizeof(m_msg.msgSize) + sizeof(msg.msgData.driftStruct); ///< Set the message size to the size of the message structure.
    msg.msgData.driftStruct = drift; ///< Set drift in the message.
    return send(&msg);
}

static uint32_t computeMsgSize(uint32_t sampleCount, uint32_t channelCount, uint32_t stimCount)
{
    return 4 * sizeof(uint32_t)
         + sampleCount * sizeof(timestampType)
         + sampleCount * channelCount * sizeof(sampleType)
         + stimCount * sizeof(ovStimulation);
}

size_t AbstractWriter::writeSamples(samplesBuffer* buffer)
{
    size_t writtenSamplesRes = 0;

    if (!(buffer == nullptr || m_chunkSamplesCount == 0))
    {
        auto& pendingStimCount = m_msg.msgData.samplesStruct.stimCount;
        size_t bufferOffset    = 0;
        auto& currentMsgCount  = m_msg.msgData.samplesStruct.count;
        size_t stimIdx         = 0;

        // Accumulate stims from the buffer whose date falls at or before upToTs.
        // Assumes stimulations are sorted by date. stimIdx persists across loop iterations.
        auto accumulateStims = [&](timestampType upToTs)
        {
            while (stimIdx < buffer->stimCount && pendingStimCount < MAX_PENDING_STIM_COUNT)
            {
                if (buffer->stimulationsBuff[stimIdx].date <= upToTs) {
                    m_msg.msgData.samplesStruct.stimulations[pendingStimCount++] = buffer->stimulationsBuff[stimIdx++];
                }
                else {
                    break;
                }
            }
        };

        if (buffer->count > 0) {
            while (buffer->count - bufferOffset > 0)
            {
                size_t freeSpace            = m_chunkSamplesCount - currentMsgCount;
                size_t samplesToWriteOrCopy = std::min(freeSpace, buffer->count - bufferOffset);
                timestampType chunkLastTs   = buffer->timestampsBuff[bufferOffset + samplesToWriteOrCopy - 1];

                // Optimization: direct send if m_msg is empty and buffer has a full chunk available
                if (currentMsgCount == 0 && buffer->count - bufferOffset >= m_chunkSamplesCount)
                {
                    accumulateStims(chunkLastTs);

                    ovMsg msg;
                    msg.msgType = ovMsg::OV_MSG_SAMPLES;
                    msg.msgData.samplesStruct.timestamp    = buffer->timestampsBuff + bufferOffset;
                    msg.msgData.samplesStruct.samples      = buffer->samplesBuff + (bufferOffset * m_channelCount);
                    msg.msgData.samplesStruct.count        = samplesToWriteOrCopy;
                    msg.msgData.samplesStruct.stimulations = m_msg.msgData.samplesStruct.stimulations;
                    msg.msgData.samplesStruct.stimCount    = pendingStimCount;
                    msg.msgSize = computeMsgSize(samplesToWriteOrCopy, m_channelCount, pendingStimCount);
                    send(&msg);
                    pendingStimCount = 0;
                }
                else
                {
                    memcpy(m_msg.msgData.samplesStruct.timestamp + currentMsgCount,
                        buffer->timestampsBuff + bufferOffset,
                        sizeof(timestampType) * samplesToWriteOrCopy);

                    memcpy(m_msg.msgData.samplesStruct.samples + (currentMsgCount * m_channelCount),
                        buffer->samplesBuff + (bufferOffset * m_channelCount),
                        sizeof(sampleType) * samplesToWriteOrCopy * m_channelCount);

                    currentMsgCount += samplesToWriteOrCopy;

                    accumulateStims(chunkLastTs);

                    if (currentMsgCount == m_chunkSamplesCount)
                    {
                        m_msg.msgSize = computeMsgSize(m_chunkSamplesCount, m_channelCount, pendingStimCount);
                        send(&m_msg);
                        currentMsgCount  = 0;
                        pendingStimCount = 0;
                    }
                }

                bufferOffset      += samplesToWriteOrCopy;
                writtenSamplesRes += samplesToWriteOrCopy;
            }
        } else if (buffer->stimCount > 0){
            accumulateStims(buffer->stimulationsBuff[buffer->stimCount-1].date);
        }
    }

    return writtenSamplesRes;
}