#include "ovpCStimulationEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CStimulationEncoder::initialize()
{
    CEBMLBaseEncoder::initialize();
    ip_stimSet.initialize(getInputParameter(OVP_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
    return true;
}

bool CStimulationEncoder::uninitialize()
{
    ip_stimSet.uninitialize();
    CEBMLBaseEncoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CStimulationEncoder::processBuffer()
{
    CStimulationSet* stimulationSet = ip_stimSet;

    m_writerHelper->openChild(OVTK_NodeId_Buffer_Stimulation);
    m_writerHelper->openChild(OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations);
    m_writerHelper->setUInt(stimulationSet->size());
    m_writerHelper->closeChild();
    for (size_t i = 0; i < stimulationSet->size(); ++i)
    {
        m_writerHelper->openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation);
        m_writerHelper->openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation_ID);
        m_writerHelper->setUInt(stimulationSet->getId(i));
        m_writerHelper->closeChild();
        m_writerHelper->openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation_Date);
        m_writerHelper->setUInt(stimulationSet->getDate(i));
        m_writerHelper->closeChild();
        m_writerHelper->openChild(OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration);
        m_writerHelper->setUInt(stimulationSet->getDuration(i));
        m_writerHelper->closeChild();
        m_writerHelper->closeChild();
    }
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
