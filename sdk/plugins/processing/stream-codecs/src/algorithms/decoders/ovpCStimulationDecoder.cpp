#include "ovpCStimulationDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

// ________________________________________________________________________________________________________________
//

bool CStimulationDecoder::initialize()
{
    CEBMLBaseDecoder::initialize();
    op_stimulationSet.initialize(getOutputParameter(OVP_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
    return true;
}

bool CStimulationDecoder::uninitialize()
{
    op_stimulationSet.uninitialize();
    CEBMLBaseDecoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CStimulationDecoder::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == OVTK_NodeId_Buffer_Stimulation) { return true; }
    if (identifier == OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations) { return false; }
    if (identifier == OVTK_NodeId_Buffer_Stimulation_Stimulation) { return true; }
    if (identifier == OVTK_NodeId_Buffer_Stimulation_Stimulation_ID) { return false; }
    if (identifier == OVTK_NodeId_Buffer_Stimulation_Stimulation_Date) { return false; }
    if (identifier == OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration) { return false; }
    return CEBMLBaseDecoder::isMasterChild(identifier);
}

void CStimulationDecoder::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Buffer_Stimulation)
        || (top == OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_ID)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Date)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration)) { }
    else { CEBMLBaseDecoder::openChild(identifier); }
}

void CStimulationDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Buffer_Stimulation)
        || (top == OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_ID)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Date)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration))
    {
        if (top == OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations)
        {
            op_stimulationSet->resize(m_readerHelper->getUInt(buffer, size));
            m_stimulationIdx = 0;
        }
        if (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_ID)
        {
            op_stimulationSet->setId(m_stimulationIdx, m_readerHelper->getUInt(buffer, size));
        }
        if (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Date)
        {
            op_stimulationSet->setDate(m_stimulationIdx, m_readerHelper->getUInt(buffer, size));
        }
        if (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration)
        {
            op_stimulationSet->setDuration(m_stimulationIdx, m_readerHelper->getUInt(buffer, size));
        }
    }
    else { CEBMLBaseDecoder::processChildData(buffer, size); }
}

void CStimulationDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Buffer_Stimulation)
        || (top == OVTK_NodeId_Buffer_Stimulation_NumberOfStimulations)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_ID)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Date)
        || (top == OVTK_NodeId_Buffer_Stimulation_Stimulation_Duration)) { if (top == OVTK_NodeId_Buffer_Stimulation_Stimulation) { m_stimulationIdx++; } }
    else { CEBMLBaseDecoder::closeChild(); }

    m_nodes.pop();
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
