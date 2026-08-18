//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>
#include "StreamRendererChannelUnits.h"
#include <iostream>

namespace OpenViBE {
namespace Tracker {

CString StreamRendererChannelUnits::renderAsText(const size_t indent) const
{
    const TypeChannelUnits::Header& hdr = m_stream->getHeader();

    std::stringstream ss;

    ss << std::string(indent, ' ') << "Dynamic: " << (hdr.m_Dynamic ? "True" : "False") << std::endl;

    for (size_t i = 0; i < m_stream->getChunkCount(); ++i) {
        const TypeChannelUnits::Buffer* buf = m_stream->getChunk(i);

        ss << std::string(indent, ' ') << "Configuration at time " << buf->m_StartTime.toSeconds() << "s:" << std::endl;

        const double* ptr = buf->m_buffer.getBuffer();

        for (uint32_t chn = 0; chn < buf->m_buffer.getDimensionSize(0); ++chn) {
            const CString unit   = m_kernelCtx.getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_MeasurementUnit, uint64_t(ptr[chn * 2 + 0]));
            const CString factor = m_kernelCtx.getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Factor, uint64_t(ptr[chn * 2 + 1]));

            ss << std::string(indent, ' ') << "  Channel " << chn << " (" << hdr.m_Header.getDimensionLabel(0, chn) << ") " << "Unit: " << unit << ", Factor: "
                    << factor << std::endl;
        }
    }

    return ss.str().c_str();
}

bool StreamRendererChannelUnits::showChunkList() { return StreamRendererLabel::showChunkList("Channel units stream details"); }

}  // namespace Tracker
}  // namespace OpenViBE
