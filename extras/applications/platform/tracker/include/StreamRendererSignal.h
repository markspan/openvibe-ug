//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererBase.h"
#include "TypeSignal.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererSignal 
 * \brief Renderer for Signal streams
 * \author J. T. Lindgren
 *
 */
class StreamRendererSignal final : public StreamRendererBase
{
public:
    StreamRendererSignal(const Kernel::IKernelContext& ctx, std::shared_ptr<const Stream<TypeSignal>> stream) : StreamRendererBase(ctx), m_stream(stream) { }

    bool initialize() override;

    bool spool(const CTime startTime, const CTime endTime) override { return spoolImpl<TypeSignal, StreamRendererSignal>(m_stream, *this, startTime, endTime); }

    CString renderAsText(const size_t indent) const override;
    bool showChunkList() override;

protected:
    friend bool spoolImpl<TypeSignal, StreamRendererSignal>(std::shared_ptr<const Stream<TypeSignal>> stream, StreamRendererSignal& renderer, CTime startTime,
                                                            CTime endTime);

    bool push(const TypeSignal::Buffer& chunk, bool zeroInput = false);
    bool reset(CTime startTime, CTime endTime) override;
    bool MouseButton(int x, int y, int button, int status) override;

    size_t m_nChannel        = 0;
    size_t m_samplesPerChunk = 0;

    std::shared_ptr<const Stream<TypeSignal>> m_stream;
    GtkWidget* m_streamListWindow = nullptr;

    StreamRendererSignal() = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
