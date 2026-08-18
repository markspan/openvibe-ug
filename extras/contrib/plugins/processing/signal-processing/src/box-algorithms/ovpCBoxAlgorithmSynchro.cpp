#include "ovpCBoxAlgorithmSynchro.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmSynchro::initialize()
{
    m_inputChannel.initialize(this);
    m_outputChannel.initialize(this);
    m_stimulationReceivedStart = false;
    return true;
}

bool CBoxAlgorithmSynchro::uninitialize()
{
    m_inputChannel.uninitialize();
    m_outputChannel.uninitialize();
    return true;
}

bool CBoxAlgorithmSynchro::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSynchro::process()
{
    // FIXME is it necessary to keep next line uncomment ?
    //IBoxIO& boxContext = this->getDynamicBoxContext();

    if (m_inputChannel.isWorking())
    {
        // process stimulations
        for (size_t index = 0, nb = m_inputChannel.getNStimulationBuffers(); index < nb; ++index)
        {
            uint64_t startTime, endTime;
            CStimulationSet* stimset = m_inputChannel.getStimulation(startTime, endTime, index);

            if (!stimset) { break; }

            m_outputChannel.sendStimulation(stimset, startTime, endTime);
        }
        // process signal
        for (size_t index = 0, nb = m_inputChannel.getNSignalBuffers(); index < nb; ++index)
        {
            uint64_t startTime, endTime;
            CMatrix* matrix = m_inputChannel.getSignal(startTime, endTime, index++);

            if (!matrix) { break; }

            m_outputChannel.sendSignal(matrix, startTime, endTime);
        }
    }
    else if (m_inputChannel.hasSynchro())
    {
        m_outputChannel.processSynchroSignal(m_inputChannel.getStimulationPosition(), m_inputChannel.getSignalPosition());
        m_inputChannel.startWorking();
    }
    else if (m_inputChannel.hasHeader()) { m_inputChannel.waitForSynchro(); }
    else if (m_inputChannel.waitForSignalHeader()) { m_outputChannel.sendHeader(m_inputChannel.getSamplingRate(), m_inputChannel.getMatrixPtr()); }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
