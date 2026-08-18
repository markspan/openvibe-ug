#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class COutputChannel
{
    typedef enum
    {
        SIGNAL_CHANNEL,
        STIMULATION_CHANNEL,
    } channel_t;

public:
    bool initialize(Toolkit::TBoxAlgorithm<IBoxAlgorithm>* boxAlgorithm);
    bool uninitialize();

    void sendStimulation(CStimulationSet* stimset, uint64_t startTime, uint64_t endTime);
    void sendSignal(CMatrix* matrix, uint64_t startTime, uint64_t endTime);

    void sendHeader(const size_t sampling, CMatrix* matrix);
    void processSynchroSignal(uint64_t stimulationPos, uint64_t signalPos);

protected:
    CMatrix* m_buffer = nullptr;

    uint64_t m_timeStimulationPos = 0;
    uint64_t m_timeSignalPos      = 0;
    uint64_t m_sampling           = 0;

    // parent memory
    Toolkit::TBoxAlgorithm<IBoxAlgorithm>* m_boxAlgorithm = nullptr;

    // signal section
    Kernel::IAlgorithmProxy* m_signalEncoder = nullptr;

    Kernel::TParameterHandler<CMemoryBuffer*> op_bufferSignal;
    Kernel::TParameterHandler<CMatrix*> ip_matrixSignal;
    Kernel::TParameterHandler<uint64_t> ip_sampling;

    // stimulation section
    Kernel::IAlgorithmProxy* m_stimEncoder = nullptr;

    Kernel::TParameterHandler<CMemoryBuffer*> op_bufferStimulation;
    Kernel::TParameterHandler<CStimulationSet*> ip_stimulationSet;
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
