//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererBase.h"
#include "TypeMatrix.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererMatrix 
 * \brief Renderer for Matrix streams
 * \author J. T. Lindgren
 *
 */
class StreamRendererMatrix final : public StreamRendererBase
{
public:
    StreamRendererMatrix(const Kernel::IKernelContext& ctx, std::shared_ptr<const Stream<TypeMatrix>> stream) : StreamRendererBase(ctx), m_stream(stream) { }

    bool initialize() override;

    bool spool(const CTime startTime, const CTime endTime) override { return spoolImpl<TypeMatrix, StreamRendererMatrix>(m_stream, *this, startTime, endTime); }

    CString renderAsText(const size_t indent) const override;
    bool showChunkList() override;

protected:
    friend bool spoolImpl<TypeMatrix, StreamRendererMatrix>(std::shared_ptr<const Stream<TypeMatrix>> stream, StreamRendererMatrix& renderer, CTime startTime,
                                                            CTime endTime);

    bool finalize() override;
    bool reset(CTime startTime, CTime endTime) override;
    bool push(const TypeMatrix::Buffer& chunk, bool zeroInput = false);
    bool MouseButton(int x, int y, int button, int status) override;

    bool PreDraw() override;
    bool Draw() override;

    size_t m_nRows = 0;
    size_t m_nCols = 0;

    std::shared_ptr<const Stream<TypeMatrix>> m_stream;
    GtkWidget* m_streamListWindow = nullptr;

    std::vector<float> m_swap;

    StreamRendererMatrix() = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
