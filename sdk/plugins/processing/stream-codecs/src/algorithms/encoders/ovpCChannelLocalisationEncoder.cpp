#include "ovpCChannelLocalisationEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CChannelLocalisationEncoder::initialize()
{
    CStreamedMatrixEncoder::initialize();
    ip_bDynamic.initialize(getInputParameter(OVP_Algorithm_ChannelLocalisationEncoder_InputParameterId_Dynamic));
    return true;
}

bool CChannelLocalisationEncoder::uninitialize()
{
    ip_bDynamic.uninitialize();
    CStreamedMatrixEncoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CChannelLocalisationEncoder::processHeader()
{
    CStreamedMatrixEncoder::processHeader();

    m_writerHelper->openChild(OVTK_NodeId_Header_ChannelLocalisation);
    m_writerHelper->openChild(OVTK_NodeId_Header_ChannelLocalisation_Dynamic);
    m_writerHelper->setUInt(ip_bDynamic ? 1 : 0);
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
