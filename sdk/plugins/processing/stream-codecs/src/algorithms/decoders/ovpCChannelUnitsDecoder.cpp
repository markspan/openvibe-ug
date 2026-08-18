#include "ovpCChannelUnitsDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

// ________________________________________________________________________________________________________________
//

bool CChannelUnitsDecoder::initialize()
{
    CStreamedMatrixDecoder::initialize();
    op_bDynamic.initialize(getOutputParameter(OVP_Algorithm_ChannelUnitsDecoder_OutputParameterId_Dynamic));
    return true;
}

bool CChannelUnitsDecoder::uninitialize()
{
    op_bDynamic.uninitialize();
    CStreamedMatrixDecoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CChannelUnitsDecoder::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == OVTK_NodeId_Header_ChannelUnits) { return true; }
    if (identifier == OVTK_NodeId_Header_ChannelUnits_Dynamic) { return false; }
    return CStreamedMatrixDecoder::isMasterChild(identifier);
}

void CChannelUnitsDecoder::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ChannelUnits)
        || (top == OVTK_NodeId_Header_ChannelUnits_Dynamic)
    ) { }
    else { CStreamedMatrixDecoder::openChild(identifier); }
}

void CChannelUnitsDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ChannelUnits)
        || (top == OVTK_NodeId_Header_ChannelUnits_Dynamic)
    )
    {
        if (top == OVTK_NodeId_Header_ChannelUnits_Dynamic) { op_bDynamic = (m_readerHelper->getUInt(buffer, size) ? true : false); }

        //if(top==OVTK_NodeId_Header_ChannelUnits_MeasurementUnit_Unit)    op_pMeasurementUnits->getBuffer()[m_unitIdx*2  ]=m_readerHelper->getDouble(buffer, size);
        //if(top==OVTK_NodeId_Header_ChannelUnits_MeasurementUnit_Factor)  op_pMeasurementUnits->getBuffer()[m_unitIdx*2+1]=m_readerHelper->getDouble(buffer, size);
    }
    else { CStreamedMatrixDecoder::processChildData(buffer, size); }
}

void CChannelUnitsDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_ChannelUnits)
        || (top == OVTK_NodeId_Header_ChannelUnits_Dynamic)
    )
    {
        //if(top==OVTK_NodeId_Header_ChannelUnits_MeasurementUnit)
        //{
        // m_unitIdx++;
        //}
    }
    else { CStreamedMatrixDecoder::closeChild(); }

    m_nodes.pop();
}
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
