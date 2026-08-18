#pragma once

// @author Gipsa-lab

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

/**
    Use this class to receive send and stimulations channels
*/

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CInputChannel
{
    typedef enum
    {
        SIGNAL_CHANNEL,
        STIMULATION_CHANNEL,
        NB_CHANNELS,
    } channel_t;

public:
    explicit CInputChannel(const uint16_t index = 0)
        : m_signalChannel(index * NB_CHANNELS + SIGNAL_CHANNEL), m_stimulationChannel(index * NB_CHANNELS + STIMULATION_CHANNEL) {}

    ~CInputChannel() { }

    bool initialize(Toolkit::TBoxAlgorithm<IBoxAlgorithm>* boxAlgorithm);
    bool uninitialize() const;

    bool isLastChannel(const size_t index) const { return index == m_stimulationChannel; }
    bool isWorking() const { return m_isWorking; }
    bool waitForSignalHeader();
    size_t getNStimulationBuffers() const { return m_boxAlgorithm->getDynamicBoxContext().getInputChunkCount(m_stimulationChannel); }
    size_t getNSignalBuffers() const { return m_boxAlgorithm->getDynamicBoxContext().getInputChunkCount(m_signalChannel); }
    CStimulationSet* getStimulation(uint64_t& startTime, uint64_t& endTime, size_t index);
    CStimulationSet* discardStimulation(size_t index);
    double* getSignal(uint64_t& startTime, uint64_t& endTime, size_t index) const;
    double* discardSignal(size_t index) const;
    uint64_t getSamplingRate() const { return m_signalDecoder->getOutputSamplingRate(); }
    size_t getNChannels() const { return m_signalDecoder->getOutputMatrix()->getDimensionSize(0); }
    size_t getNSamples() const { return m_signalDecoder->getOutputMatrix()->getDimensionSize(1); }
    uint64_t getStartTimestamp() const { return m_startTimestamp; }
    uint64_t getEndTimestamp() const { return m_endTimestamp; }
    const char* getChannelName(const size_t index) const { return m_signalDecoder->getOutputMatrix()->getDimensionLabel(0, index); }
    const Kernel::TParameterHandler<CMatrix*>& getOpMatrix() const { return m_signalDecoder->getOutputMatrix(); }

protected:
    size_t m_signalChannel      = 0;
    size_t m_stimulationChannel = 0;
    bool m_isWorking            = false;

    uint64_t m_startTimestamp = 0;
    uint64_t m_endTimestamp   = 0;

    CStimulationSet* m_stimulationSet = nullptr;

    // parent memory
    Toolkit::TBoxAlgorithm<IBoxAlgorithm>* m_boxAlgorithm = nullptr;

    // signal section
    //Kernel::IAlgorithmProxy* m_signalDecoder;
    Toolkit::TSignalDecoder<Toolkit::TBoxAlgorithm<IBoxAlgorithm>>* m_signalDecoder = nullptr;

    // stimulation section
    Toolkit::TStimulationDecoder<Toolkit::TBoxAlgorithm<IBoxAlgorithm>>* m_stimDecoder = nullptr;
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
