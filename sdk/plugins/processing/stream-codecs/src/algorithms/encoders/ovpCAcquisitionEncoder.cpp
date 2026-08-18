#include "ovpCAcquisitionEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CAcquisitionEncoder::initialize()
{
    CEBMLBaseEncoder::initialize();

    ip_bufferDuration.initialize(getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_BufferDuration));
    ip_experimentInfoStream.initialize(getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ExperimentInfoStream));
    ip_signalStream.initialize(getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_SignalStream));
    ip_stimulationStream.initialize(getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_StimulationStream));
    ip_channelLocalisationStream.initialize(getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelLocalisationStream));
    ip_channelUnitsStream.initialize(getInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelUnitsStream));

    return true;
}

bool CAcquisitionEncoder::uninitialize()
{
    ip_channelUnitsStream.uninitialize();
    ip_channelLocalisationStream.uninitialize();
    ip_stimulationStream.uninitialize();
    ip_signalStream.uninitialize();
    ip_experimentInfoStream.uninitialize();
    ip_bufferDuration.uninitialize();

    CEBMLBaseEncoder::uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CAcquisitionEncoder::processHeader()
{
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Header_BufferDuration);
    m_writerHelper->setUInt(ip_bufferDuration);
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Header_ExperimentInfo);
    m_writerHelper->setBinary(ip_experimentInfoStream->getDirectPointer(), ip_experimentInfoStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Header_Signal);
    m_writerHelper->setBinary(ip_signalStream->getDirectPointer(), ip_signalStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Header_Stimulation);
    m_writerHelper->setBinary(ip_stimulationStream->getDirectPointer(), ip_stimulationStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Header_ChannelLocalisation);
    m_writerHelper->setBinary(ip_channelLocalisationStream->getDirectPointer(), ip_channelLocalisationStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Header_ChannelUnits);
    m_writerHelper->setBinary(ip_channelUnitsStream->getDirectPointer(), ip_channelUnitsStream->getSize());
    m_writerHelper->closeChild();

    return true;
}

bool CAcquisitionEncoder::processBuffer()
{
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Buffer_ExperimentInfo);
    m_writerHelper->setBinary(ip_experimentInfoStream->getDirectPointer(), ip_experimentInfoStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Buffer_Signal);
    m_writerHelper->setBinary(ip_signalStream->getDirectPointer(), ip_signalStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Buffer_Stimulation);
    m_writerHelper->setBinary(ip_stimulationStream->getDirectPointer(), ip_stimulationStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Buffer_ChannelLocalisation);
    m_writerHelper->setBinary(ip_channelLocalisationStream->getDirectPointer(), ip_channelLocalisationStream->getSize());
    m_writerHelper->closeChild();
    m_writerHelper->openChild(OVTK_NodeId_Acquisition_Buffer_ChannelUnits);
    m_writerHelper->setBinary(ip_channelUnitsStream->getDirectPointer(), ip_channelUnitsStream->getSize());
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
