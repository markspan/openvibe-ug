//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererBase.h"
#include "TypeSpectrum.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererSpectrum 
 * \brief Renderer for Spectrum streams
 * \author J. T. Lindgren
 *
 */
class StreamRendererSpectrum final : public StreamRendererBase
{
public:
    StreamRendererSpectrum(const Kernel::IKernelContext& ctx, std::shared_ptr<const Stream<TypeSpectrum>> stream)
        : StreamRendererBase(ctx), m_stream(stream) { }

    bool initialize() override;

    bool spool(const CTime startTime, const CTime endTime) override
    {
        return spoolImpl<TypeSpectrum, StreamRendererSpectrum>(m_stream, *this, startTime, endTime);
    }

    CString renderAsText(const size_t indent) const override;
    bool showChunkList() override;

protected:
    friend bool spoolImpl<TypeSpectrum, StreamRendererSpectrum>(std::shared_ptr<const Stream<TypeSpectrum>> stream, StreamRendererSpectrum& renderer,
                                                                CTime startTime, CTime endTime);

    bool finalize() override;
    bool reset(CTime startTime, CTime endTime) override;
    bool push(const TypeSpectrum::Buffer& chunk, bool zeroInput = false);
    bool MouseButton(int x, int y, int button, int status) override;

    bool PreDraw() override;
    bool Draw() override;

    size_t m_nChannel         = 0;
    size_t m_spectrumElements = 0;

    std::vector<float> m_swaps;

    std::shared_ptr<const Stream<TypeSpectrum>> m_stream;
    GtkWidget* m_streamListWindow = nullptr;

    StreamRendererSpectrum() = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
