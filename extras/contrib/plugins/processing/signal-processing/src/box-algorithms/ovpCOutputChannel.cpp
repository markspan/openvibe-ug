#include "ovpCOutputChannel.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


bool COutputChannel::initialize(Toolkit::TBoxAlgorithm<IBoxAlgorithm>* boxAlgorithm)
{
    m_boxAlgorithm = boxAlgorithm;


    m_signalEncoder = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
        m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_signalEncoder->initialize();
    op_bufferSignal.initialize(m_signalEncoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));
    ip_matrixSignal.initialize(m_signalEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));
    ip_sampling.initialize(m_signalEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));


    m_stimEncoder = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
        m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
    m_stimEncoder->initialize();
    op_bufferStimulation.initialize(m_stimEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));
    ip_stimulationSet.initialize(m_stimEncoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));

    return true;
}

bool COutputChannel::uninitialize()
{
    ip_sampling.uninitialize();
    ip_matrixSignal.uninitialize();
    op_bufferSignal.uninitialize();
    m_signalEncoder->uninitialize();
    m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_signalEncoder);

    op_bufferStimulation.uninitialize();
    ip_stimulationSet.uninitialize();
    m_stimEncoder->uninitialize();
    m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_stimEncoder);

    return true;
}

void COutputChannel::sendStimulation(CStimulationSet* stimset, const uint64_t startTime, const uint64_t endTime)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    for (size_t j = 0; j < stimset->size(); ++j)
    {
        if (stimset->getDate(j) < m_timeStimulationPos)
        {
            stimset->erase(j);
            j--;
        }
        else { stimset->setDate(j, stimset->getDate(j) - m_timeStimulationPos); }
    }

    ip_stimulationSet    = stimset;
    op_bufferStimulation = boxContext.getOutputChunk(STIMULATION_CHANNEL);
    m_stimEncoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer);
    boxContext.markOutputAsReadyToSend(STIMULATION_CHANNEL, startTime - m_timeStimulationPos, endTime - m_timeStimulationPos);
}

void COutputChannel::sendHeader(const size_t sampling, CMatrix* matrix)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    m_buffer   = matrix;
    m_sampling = sampling;

    op_bufferSignal = boxContext.getOutputChunk(SIGNAL_CHANNEL);
    ip_matrixSignal = m_buffer;
    ip_sampling     = m_sampling;

    //copy channel names
    for (size_t i = 0; i < matrix->getDimensionSize(0); ++i) { ip_matrixSignal->setDimensionLabel(0, i, matrix->getDimensionLabel(0, i)); }

    m_signalEncoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);
    boxContext.markOutputAsReadyToSend(SIGNAL_CHANNEL, 0, 0);
}

void COutputChannel::sendSignal(CMatrix* matrix, const uint64_t startTime, const uint64_t endTime)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    op_bufferSignal = boxContext.getOutputChunk(SIGNAL_CHANNEL);
    ip_matrixSignal = matrix;
    ip_sampling     = m_sampling;
    m_signalEncoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);
    boxContext.markOutputAsReadyToSend(SIGNAL_CHANNEL, startTime - m_timeSignalPos, endTime - m_timeSignalPos);
}

void COutputChannel::processSynchroSignal(const uint64_t stimulationPos, const uint64_t signalPos)
{
    m_timeStimulationPos = stimulationPos;
    m_timeSignalPos      = signalPos;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
