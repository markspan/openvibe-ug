#include "ovpCSignalDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

// ________________________________________________________________________________________________________________
//

bool CSignalDecoder::initialize()
{
    CStreamedMatrixDecoder::initialize();
    op_sampling.initialize(getOutputParameter(OVP_Algorithm_SignalDecoder_OutputParameterId_Sampling));
    return true;
}

bool CSignalDecoder::uninitialize()
{
    op_sampling.uninitialize();
    CStreamedMatrixDecoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CSignalDecoder::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == OVTK_NodeId_Header_Signal) { return true; }
    if (identifier == OVTK_NodeId_Header_Signal_Sampling) { return false; }
    return CStreamedMatrixDecoder::isMasterChild(identifier);
}

void CSignalDecoder::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_Signal) || (top == OVTK_NodeId_Header_Signal_Sampling)) { }
    else { CStreamedMatrixDecoder::openChild(identifier); }
}

void CSignalDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_Signal) || (top == OVTK_NodeId_Header_Signal_Sampling))
    {
        if (top == OVTK_NodeId_Header_Signal_Sampling) { op_sampling = m_readerHelper->getUInt(buffer, size); }
    }
    else { CStreamedMatrixDecoder::processChildData(buffer, size); }
}

void CSignalDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_Signal) || (top == OVTK_NodeId_Header_Signal_Sampling)) { }
    else { CStreamedMatrixDecoder::closeChild(); }

    m_nodes.pop();
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
