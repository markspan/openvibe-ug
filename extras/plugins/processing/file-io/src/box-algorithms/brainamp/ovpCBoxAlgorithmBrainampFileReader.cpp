#include "ovpCBoxAlgorithmBrainampFileReader.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

uint64_t CBoxAlgorithmBrainampFileReader::getClockFrequency()
{
    // Brainamp file reader parameters
    Kernel::TParameterHandler<CString*> ip_filename(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_Filename));
    Kernel::TParameterHandler<double> ip_epochDuration(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_EpochDuration));
    Kernel::TParameterHandler<uint64_t> ip_seekTime(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_SeekTime));
    Kernel::TParameterHandler<uint64_t> op_startTime(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentStartTime));
    Kernel::TParameterHandler<uint64_t> op_endTime(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentEndTime));
    Kernel::TParameterHandler<uint64_t> op_sampling(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Sampling));
    Kernel::TParameterHandler<CMatrix*> op_signalMatrix(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_SignalMatrix));
    Kernel::TParameterHandler<CStimulationSet*> op_stimulations(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Stimulations));

    return uint64_t((1LL << 32) / ip_epochDuration);
}

bool CBoxAlgorithmBrainampFileReader::initialize()
{
    // Creates algorithms
    m_reader = &getAlgorithmManager().
            getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_BrainampFileReader));
    m_experimentInfoEncoder = &getAlgorithmManager().getAlgorithm(
        getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ExperimentInfoEncoder));
    m_signalEncoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_stimEncoder   = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));

    m_reader->initialize();
    m_experimentInfoEncoder->initialize();
    m_signalEncoder->initialize();
    m_stimEncoder->initialize();

    // Brainamp file reader parameters
    Kernel::TParameterHandler<CString*> ip_filename(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_Filename));
    Kernel::TParameterHandler<double> ip_epochDuration(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_EpochDuration));
    Kernel::TParameterHandler<uint64_t> ip_seekTime(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_SeekTime));
    Kernel::TParameterHandler<bool> ip_convertStimuli(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_ConvertStimuli));
    Kernel::TParameterHandler<uint64_t> op_startTime(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentStartTime));
    Kernel::TParameterHandler<uint64_t> op_endTime(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentEndTime));
    Kernel::TParameterHandler<uint64_t> op_sampling(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Sampling));
    Kernel::TParameterHandler<CMatrix*> op_signalMatrix(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_SignalMatrix));
    Kernel::TParameterHandler<CStimulationSet*> op_stimulations(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Stimulations));

    // Signal stream encoder parameters
    Kernel::TParameterHandler<uint64_t> ip_sampling(m_signalEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));
    Kernel::TParameterHandler<CMatrix*> ip_signalMatrix(m_signalEncoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));

    // Stimulation stream encoder parameters
    Kernel::TParameterHandler<CStimulationSet*> ip_stimulations(
        m_stimEncoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));

    // Connect parameters together
    ip_sampling.setReferenceTarget(op_sampling);
    ip_signalMatrix.setReferenceTarget(op_signalMatrix);
    ip_stimulations.setReferenceTarget(op_stimulations);

    // Configures settings according to box
    *ip_filename      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    ip_epochDuration  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    ip_convertStimuli = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    m_headerSent = false;

    return true;
}

bool CBoxAlgorithmBrainampFileReader::uninitialize()
{
    m_reader->process(OVP_Algorithm_BrainampFileReader_InputTriggerId_Close);

    m_reader->uninitialize();
    m_stimEncoder->uninitialize();
    m_signalEncoder->uninitialize();
    m_experimentInfoEncoder->uninitialize();

    getAlgorithmManager().releaseAlgorithm(*m_reader);
    getAlgorithmManager().releaseAlgorithm(*m_stimEncoder);
    getAlgorithmManager().releaseAlgorithm(*m_signalEncoder);
    getAlgorithmManager().releaseAlgorithm(*m_experimentInfoEncoder);

    return true;
}

bool CBoxAlgorithmBrainampFileReader::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmBrainampFileReader::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Brainamp file reader parameters
    Kernel::TParameterHandler<CString*> ip_filename(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_Filename));
    Kernel::TParameterHandler<double> ip_epochDuration(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_EpochDuration));
    Kernel::TParameterHandler<uint64_t> ip_seekTime(m_reader->getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_SeekTime));
    Kernel::TParameterHandler<uint64_t> op_startTime(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentStartTime));
    Kernel::TParameterHandler<uint64_t> op_endTime(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentEndTime));
    Kernel::TParameterHandler<uint64_t> op_sampling(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Sampling));
    Kernel::TParameterHandler<CMatrix*> op_signalMatrix(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_SignalMatrix));
    Kernel::TParameterHandler<CStimulationSet*> op_stimulations(m_reader->getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Stimulations));

    // Signal stream encoder parameters
    Kernel::TParameterHandler<CMemoryBuffer*>
            op_signalBuffer(m_signalEncoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Stimulation stream encoder parameters
    Kernel::TParameterHandler<CMemoryBuffer*> op_stimulationBuffer(
        m_stimEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Experiment information
    Kernel::TParameterHandler<CMemoryBuffer*> op_experimentInfoBuffer(
        m_experimentInfoEncoder->getOutputParameter(OVP_GD_Algorithm_ExperimentInfoEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Connects parameters to memory buffer
    op_experimentInfoBuffer = boxContext.getOutputChunk(0);
    op_signalBuffer         = boxContext.getOutputChunk(1);
    op_stimulationBuffer    = boxContext.getOutputChunk(2);

    if (!m_headerSent)
    {
        // Opens file
        if (!m_reader->process(OVP_Algorithm_BrainampFileReader_InputTriggerId_Open)) { return false; }

        // Produces header
        m_experimentInfoEncoder->process(OVP_GD_Algorithm_ExperimentInfoEncoder_InputTriggerId_EncodeHeader);
        m_stimEncoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader);
        m_signalEncoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);

        // Sends header
        boxContext.markOutputAsReadyToSend(0, 0, 0);
        boxContext.markOutputAsReadyToSend(1, 0, 0);
        boxContext.markOutputAsReadyToSend(2, 0, 0);

        // Turn flag off
        m_headerSent = true;
    }

    if (!m_reader->process(OVP_Algorithm_BrainampFileReader_InputTriggerId_Next)) { return false; }

    if (m_reader->isOutputTriggerActive(OVP_Algorithm_BrainampFileReader_OutputTriggerId_DataProduced))
    {
        // Produces buffer
        m_experimentInfoEncoder->process(OVP_GD_Algorithm_ExperimentInfoEncoder_InputTriggerId_EncodeBuffer);
        m_stimEncoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer);
        m_signalEncoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);

        // Sends buffer
        boxContext.markOutputAsReadyToSend(0, op_startTime, op_endTime);
        boxContext.markOutputAsReadyToSend(1, op_startTime, op_endTime);
        boxContext.markOutputAsReadyToSend(2, op_startTime, op_endTime);
    }

    return true;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
