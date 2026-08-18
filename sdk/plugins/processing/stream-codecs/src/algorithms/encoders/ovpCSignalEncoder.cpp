#include "ovpCSignalEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CSignalEncoder::initialize()
{
    CStreamedMatrixEncoder::initialize();
    ip_sampling.initialize(getInputParameter(OVP_Algorithm_SignalEncoder_InputParameterId_Sampling));
    return true;
}

bool CSignalEncoder::uninitialize()
{
    ip_sampling.uninitialize();
    CStreamedMatrixEncoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CSignalEncoder::processHeader()
{
    m_writerHelper->openChild(OVTK_NodeId_Header_Signal);
    m_writerHelper->openChild(OVTK_NodeId_Header_Signal_Sampling);
    m_writerHelper->setUInt(ip_sampling);
    m_writerHelper->closeChild();
    m_writerHelper->closeChild();

    CStreamedMatrixEncoder::processHeader();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
