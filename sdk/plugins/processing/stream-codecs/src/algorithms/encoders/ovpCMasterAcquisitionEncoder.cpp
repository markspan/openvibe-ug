#include "ovpCMasterAcquisitionEncoder.h"
#include "../../algorithms/encoders/ovpCAcquisitionEncoder.h"
#include "../../algorithms/encoders/ovpCExperimentInfoEncoder.h"
#include "../../algorithms/encoders/ovpCSignalEncoder.h"
#include "../../algorithms/encoders/ovpCStimulationEncoder.h"
#include "../../algorithms/encoders/ovpCChannelLocalisationEncoder.h"
#include "../../algorithms/encoders/ovpCChannelUnitsEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CMasterAcquisitionEncoder::initialize()
{
    // Manages sub-algorithms

    m_acquisitionStreamEncoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_AcquisitionEncoder));
    m_experimentInfoStreamEncoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ExperimentInfoEncoder));
    m_signalStreamEncoder      = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_SignalEncoder));
    m_stimulationStreamEncoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_StimulationEncoder));
    m_channelLocalisationStreamEncoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ChannelLocalisationEncoder));
    m_channelUnitsStreamEncoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ChannelUnitsEncoder));

    m_acquisitionStreamEncoder->initialize();
    m_experimentInfoStreamEncoder->initialize();
    m_signalStreamEncoder->initialize();
    m_stimulationStreamEncoder->initialize();
    m_channelLocalisationStreamEncoder->initialize();
    m_channelUnitsStreamEncoder->initialize();

    // Declares parameter handlers for this algorithm

    Kernel::TParameterHandler<uint64_t> ip_subjectID(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectID));
    Kernel::TParameterHandler<uint64_t> ip_subjectAge(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectAge));
    Kernel::TParameterHandler<uint64_t> ip_subjectGender(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectGender));
    Kernel::TParameterHandler<CMatrix*> ip_pMatrix(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalMatrix));
    Kernel::TParameterHandler<uint64_t> ip_sampling(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalSampling));
    Kernel::TParameterHandler<CStimulationSet*> ip_stimSet(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_StimulationSet));
    Kernel::TParameterHandler<uint64_t> ip_bufferDuration(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_BufferDuration));
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer(this->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));
    Kernel::TParameterHandler<CMatrix*> ip_channelLocalisationMaster(
        this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelLocalisation));
    Kernel::TParameterHandler<CMatrix*> ip_channelUnitsMaster(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelUnits));

    // Declares parameter handlers for sub-algorithm acquisition

    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionChannelUnitsMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelUnitsStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionChannelLocalisationMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelLocalisationStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionExperimentInfoMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ExperimentInfoStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionSignalMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_SignalStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionStimulationMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_StimulationStream));
    Kernel::TParameterHandler<uint64_t> ip_AcquisitionBufferDuration(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_BufferDuration));
    Kernel::TParameterHandler<CMemoryBuffer*> op_pAcquisitionMemoryBuffer(
        m_acquisitionStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Declares parameter handlers for sub-algorithm experiment information

    // Kernel::TParameterHandler<uint64_t> ip_ExperimentInfoExperimentID(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentID));
    // Kernel::TParameterHandler<CString*> ip_experimentInfoExperimentDate(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_ExperimentDate));
    Kernel::TParameterHandler<uint64_t> ip_ExperimentInfoSubjectID(
        m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectID));
    // Kernel::TParameterHandler< CString*> ip_experimentInfoSubjectName(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectName));
    Kernel::TParameterHandler<uint64_t> ip_ExperimentInfoSubjectAge(
        m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectAge));
    Kernel::TParameterHandler<uint64_t> ip_ExperimentInfoSubjectGender(
        m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_SubjectGender));
    // Kernel::TParameterHandler<uint64_t> ip_ExperimentInfoLaboratoryID(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryID));
    // Kernel::TParameterHandler<CString*> ip_experimentInfoLaboratoryName(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_LaboratoryName));
    // Kernel::TParameterHandler<uint64_t> ip_ExperimentInfoTechnicianID(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianID));
    // Kernel::TParameterHandler<CString*> ip_experimentInfoTehnicianName(m_experimentInfoStreamEncoder->getInputParameter(OVP_Algorithm_ExperimentInfoEncoder_InputParameterId_TechnicianName));
    Kernel::TParameterHandler<CMemoryBuffer*> op_experimentInfoMemoryBuffer(
        m_experimentInfoStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Declares parameter handlers for sub-algorithm signal

    Kernel::TParameterHandler<CMatrix*> ip_matrix(m_signalStreamEncoder->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix));
    Kernel::TParameterHandler<uint64_t> ip_signalSamplingRate(m_signalStreamEncoder->getInputParameter(OVP_Algorithm_SignalEncoder_InputParameterId_Sampling));
    Kernel::TParameterHandler<CMemoryBuffer*> op_signalMemoryBuffer(
        m_signalStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    // Declares parameter handlers for sub-algorithm stimulation

    Kernel::TParameterHandler<CStimulationSet*> ip_stimulationStimulationSet(
        m_stimulationStreamEncoder->getInputParameter(OVP_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
    Kernel::TParameterHandler<CMemoryBuffer*> op_stimulationMemoryBuffer(
        m_stimulationStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    Kernel::TParameterHandler<CMatrix*> ip_channelLocalisation(
        m_channelLocalisationStreamEncoder->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix));
    Kernel::TParameterHandler<CMemoryBuffer*> op_channelLocalisationMemoryBuffer(
        m_channelLocalisationStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    Kernel::TParameterHandler<CMatrix*> ip_pUnits(m_channelUnitsStreamEncoder->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix));
    Kernel::TParameterHandler<CMemoryBuffer*> op_channelUnitsMemoryBuffer(
        m_channelUnitsStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));


    // Manage parameter connection / referencing | this algorithm to sub algorithm

    ip_ExperimentInfoSubjectID.setReferenceTarget(ip_subjectID);
    ip_ExperimentInfoSubjectAge.setReferenceTarget(ip_subjectAge);
    ip_ExperimentInfoSubjectGender.setReferenceTarget(ip_subjectGender);
    ip_matrix.setReferenceTarget(ip_pMatrix);
    ip_signalSamplingRate.setReferenceTarget(ip_sampling);
    ip_stimulationStimulationSet.setReferenceTarget(ip_stimSet);
    ip_AcquisitionBufferDuration.setReferenceTarget(ip_bufferDuration);
    op_buffer.setReferenceTarget(op_pAcquisitionMemoryBuffer);
    ip_channelLocalisationMaster.setReferenceTarget(ip_channelLocalisation);
    ip_channelUnitsMaster.setReferenceTarget(ip_pUnits);

    // Manage parameter connection / referencing | sub-algorithm to sub algorithm

    ip_pAcquisitionExperimentInfoMemoryBuffer.setReferenceTarget(op_experimentInfoMemoryBuffer);
    ip_pAcquisitionSignalMemoryBuffer.setReferenceTarget(op_signalMemoryBuffer);
    ip_pAcquisitionStimulationMemoryBuffer.setReferenceTarget(op_stimulationMemoryBuffer);
    ip_pAcquisitionChannelLocalisationMemoryBuffer.setReferenceTarget(op_channelLocalisationMemoryBuffer);
    ip_pAcquisitionChannelUnitsMemoryBuffer.setReferenceTarget(op_channelUnitsMemoryBuffer);

    return true;
}

bool CMasterAcquisitionEncoder::uninitialize()
{
    m_channelUnitsStreamEncoder->uninitialize();
    m_channelLocalisationStreamEncoder->uninitialize();
    m_stimulationStreamEncoder->uninitialize();
    m_signalStreamEncoder->uninitialize();
    m_experimentInfoStreamEncoder->uninitialize();
    m_acquisitionStreamEncoder->uninitialize();

    this->getAlgorithmManager().releaseAlgorithm(*m_channelUnitsStreamEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_channelLocalisationStreamEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_stimulationStreamEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_signalStreamEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_experimentInfoStreamEncoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_acquisitionStreamEncoder);

    return true;
}

bool CMasterAcquisitionEncoder::process()
{
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionChannelUnitsMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelUnitsStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionChannelLocalisationMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelLocalisationStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionExperimentInfoMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ExperimentInfoStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionSignalMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_SignalStream));
    Kernel::TParameterHandler<CMemoryBuffer*> ip_pAcquisitionStimulationMemoryBuffer(
        m_acquisitionStreamEncoder->getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_StimulationStream));
    // Kernel::TParameterHandler < CMemoryBuffer* > op_pAcquisitionMemoryBuffer(m_acquisitionStreamEncoder->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));

    if (this->isInputTriggerActive(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader))
    {
        ip_pAcquisitionChannelUnitsMemoryBuffer->setSize(0, true);
        ip_pAcquisitionChannelLocalisationMemoryBuffer->setSize(0, true);
        ip_pAcquisitionExperimentInfoMemoryBuffer->setSize(0, true);
        ip_pAcquisitionSignalMemoryBuffer->setSize(0, true);
        ip_pAcquisitionStimulationMemoryBuffer->setSize(0, true);
        // op_pAcquisitionMemoryBuffer->setSize(0, true);

        m_channelLocalisationStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
        m_channelUnitsStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
        m_stimulationStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
        m_signalStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
        m_experimentInfoStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
        m_acquisitionStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
    }
    if (this->isInputTriggerActive(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer))
    {
        ip_pAcquisitionChannelUnitsMemoryBuffer->setSize(0, true);
        ip_pAcquisitionChannelLocalisationMemoryBuffer->setSize(0, true);
        ip_pAcquisitionExperimentInfoMemoryBuffer->setSize(0, true);
        ip_pAcquisitionSignalMemoryBuffer->setSize(0, true);
        ip_pAcquisitionStimulationMemoryBuffer->setSize(0, true);
        // op_pAcquisitionMemoryBuffer->setSize(0, true);

        // For these streams, we only send the buffer if there is something to send
        const Kernel::TParameterHandler<bool>
                ip_bEncodeUnitData(this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelUnitData));
        if (ip_bEncodeUnitData)
        {
            // this->getLogManager() << Kernel::LogLevel_Info << "Encoding units " << ip_pUnits->getBufferElementCount() << "\n";
            m_channelUnitsStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer);
        }

        const Kernel::TParameterHandler<bool> ip_encodeChannelLocalisationData(
            this->getInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelLocalisationData));
        if (ip_encodeChannelLocalisationData) { m_channelLocalisationStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer); }

        m_stimulationStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer);
        m_signalStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer);
        m_experimentInfoStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer);
        m_acquisitionStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer);
    }
    if (this->isInputTriggerActive(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd))
    {
        ip_pAcquisitionChannelUnitsMemoryBuffer->setSize(0, true);
        ip_pAcquisitionChannelLocalisationMemoryBuffer->setSize(0, true);
        ip_pAcquisitionExperimentInfoMemoryBuffer->setSize(0, true);
        ip_pAcquisitionSignalMemoryBuffer->setSize(0, true);
        ip_pAcquisitionStimulationMemoryBuffer->setSize(0, true);
        // op_pAcquisitionMemoryBuffer->setSize(0, true);

        m_channelUnitsStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd);
        m_channelLocalisationStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd);
        m_stimulationStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd);
        m_signalStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd);
        m_experimentInfoStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd);
        m_acquisitionStreamEncoder->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd);
    }

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
