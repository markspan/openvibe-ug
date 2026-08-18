#include "ovpCChannelLocalisationDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

// ________________________________________________________________________________________________________________
//

bool CChannelLocalisationDecoder::initialize()
{
    CStreamedMatrixDecoder::initialize();
    op_bDynamic.initialize(getOutputParameter(OVP_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Dynamic));
    return true;
}

bool CChannelLocalisationDecoder::uninitialize()
{
    op_bDynamic.uninitialize();
    CStreamedMatrixDecoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CChannelLocalisationDecoder::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == OVTK_NodeId_Header_ChannelLocalisation) { return true; }
    if (identifier == OVTK_NodeId_Header_ChannelLocalisation_Dynamic) { return false; }
    return CStreamedMatrixDecoder::isMasterChild(identifier);
}

void CChannelLocalisationDecoder::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ChannelLocalisation) || (top == OVTK_NodeId_Header_ChannelLocalisation_Dynamic)) { }
    else { CStreamedMatrixDecoder::openChild(identifier); }
}

void CChannelLocalisationDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ChannelLocalisation) || (top == OVTK_NodeId_Header_ChannelLocalisation_Dynamic))
    {
        if (top == OVTK_NodeId_Header_ChannelLocalisation_Dynamic) { op_bDynamic = (m_readerHelper->getUInt(buffer, size) ? true : false); }
    }
    else { CStreamedMatrixDecoder::processChildData(buffer, size); }
}

void CChannelLocalisationDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ChannelLocalisation) || (top == OVTK_NodeId_Header_ChannelLocalisation_Dynamic)) { }
    else { CStreamedMatrixDecoder::closeChild(); }

    m_nodes.pop();
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
