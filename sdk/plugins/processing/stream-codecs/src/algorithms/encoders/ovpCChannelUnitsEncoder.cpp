#include "ovpCChannelUnitsEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CChannelUnitsEncoder::initialize()
{
    CStreamedMatrixEncoder::initialize();
    ip_bDynamic.initialize(getInputParameter(OVP_Algorithm_ChannelUnitsEncoder_InputParameterId_Dynamic));
    return true;
}

bool CChannelUnitsEncoder::uninitialize()
{
    ip_bDynamic.uninitialize();
    CStreamedMatrixEncoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CChannelUnitsEncoder::processHeader()
{
    CStreamedMatrixEncoder::processHeader();

    m_writerHelper->openChild(OVTK_NodeId_Header_ChannelUnits);
    m_writerHelper->openChild(OVTK_NodeId_Header_ChannelUnits_Dynamic);
    m_writerHelper->setUInt(ip_bDynamic ? 1 : 0);
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
