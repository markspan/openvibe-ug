#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define SET_BIT(bit)    (1 << bit)

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CInputChannel
{
    typedef enum
    {
        NOT_STARTED = 0,
        SIGNAL_HEADER_DETECTED = SET_BIT(0),
        STIMULATION_SYNCHRO_DETECTED = SET_BIT(1),
        SIGNAL_SYNCHRO_DETECTED = SET_BIT(2),
        IN_WORK = SET_BIT(3),
    } status_t;

    typedef enum
    {
        SIGNAL_CHANNEL,
        STIMULATION_CHANNEL,
    } channel_t;

public:
    CInputChannel();
    ~CInputChannel();
    bool initialize(Toolkit::TBoxAlgorithm<IBoxAlgorithm>* boxAlgorithm);
    bool uninitialize();

    bool hasHeader() const { return (m_status & SIGNAL_HEADER_DETECTED) != 0; }
    bool hasSynchro() const { return hasSynchroStimulation() && hasSynchroSignal(); }
    bool isWorking() const { return (m_status & IN_WORK) != 0; }
    bool waitForSignalHeader();
    void waitForSynchro();
    void startWorking() { m_status |= IN_WORK; }
    uint64_t getStimulationPosition() const { return m_timeStimulationPos; }
    uint64_t getSignalPosition() const { return m_timeSignalPos; }
    size_t getNStimulationBuffers() const { return m_boxAlgorithm->getDynamicBoxContext().getInputChunkCount(STIMULATION_CHANNEL); }
    size_t getNSignalBuffers() const { return m_boxAlgorithm->getDynamicBoxContext().getInputChunkCount(SIGNAL_CHANNEL); }
    CStimulationSet* getStimulation(uint64_t& startTimestamp, uint64_t& endTimestamp, size_t stimulationIndex);
    CMatrix* getSignal(uint64_t& startTimestamp, uint64_t& endTimestamp, size_t signalIndex);
    CMatrix* getMatrixPtr() { return m_oMatrix[m_ptrMatrixIdx & 1]; }
    uint64_t getSamplingRate() const { return op_sampling; }

private:
    bool hasSynchroStimulation() const { return (m_status & STIMULATION_SYNCHRO_DETECTED) != 0; }
    bool hasSynchroSignal() const { return (m_status & SIGNAL_SYNCHRO_DETECTED) != 0; }
    void waitForSynchroStimulation();
    void waitForSynchroSignal();
    void processSynchroSignal();
    CMatrix* getMatrix() { return m_oMatrix[m_ptrMatrixIdx++ & 1]; }
    void copyData(bool copyFirstBlock, size_t matrixIndex);

protected:
    uint16_t m_status = 0;
    CMatrix* m_oMatrix[2];
    uint64_t m_ptrMatrixIdx       = 0;
    uint64_t m_synchroStimulation = 0;

    uint64_t m_timeStimulationPos   = 0;
    uint64_t m_timeStimulationStart = 0;
    uint64_t m_timeStimulationEnd   = 0;
    bool m_hasFirstStimulation      = false;


    uint64_t m_timeSignalPos   = 0;
    uint64_t m_timeSignalStart = 0;
    uint64_t m_timeSignalEnd   = 0;

    size_t m_firstBlock  = 0;
    size_t m_secondBlock = 0;
    size_t m_nSamples    = 0;
    size_t m_nChannels   = 0;
    bool m_hasFirstChunk = false;

    CStimulationSet* m_stimulationSet = nullptr;

    // parent memory
    Toolkit::TBoxAlgorithm<IBoxAlgorithm>* m_boxAlgorithm = nullptr;

    // signal section
    Kernel::IAlgorithmProxy* m_signalDecoder = nullptr;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_bufferSignal;
    Kernel::TParameterHandler<CMatrix*> op_matrixSignal;
    Kernel::TParameterHandler<uint64_t> op_sampling;


    // stimulation section
    Kernel::IAlgorithmProxy* m_stimDecoder = nullptr;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_bufferStimulation;
    Kernel::TParameterHandler<CStimulationSet*> op_stimulationSet;
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
