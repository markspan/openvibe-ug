//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererBase.h"
#include "TypeStimulation.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererStimulation 
 * \brief Renderer for Stimulation streams
 * \author J. T. Lindgren
 *
 */
class StreamRendererStimulation final : public StreamRendererBase
{
public:
    StreamRendererStimulation(const Kernel::IKernelContext& ctx, std::shared_ptr<const Stream<TypeStimulation>> stream)
        : StreamRendererBase(ctx), m_stream(stream) { }

    bool initialize() override;

    bool spool(const CTime startTime, const CTime endTime) override
    {
        return spoolImpl<TypeStimulation, StreamRendererStimulation>(m_stream, *this, startTime, endTime);
    }

    CString renderAsText(const size_t indent) const override;
    bool showChunkList() override;

protected:
    friend bool spoolImpl<TypeStimulation, StreamRendererStimulation>(std::shared_ptr<const Stream<TypeStimulation>> stream,
                                                                      StreamRendererStimulation& renderer, CTime startTime, CTime endTime);

    bool push(const TypeStimulation::Buffer& chunk, bool zeroInput = false);

    bool reset(CTime startTime, CTime endTime) override;

    bool MouseButton(int x, int y, int button, int status) override;

    size_t m_nChannel        = 0;
    size_t m_samplesPerChunk = 0;
    uint32_t m_sampling      = 0;

    GtkWidget* m_stimulationListWindow = nullptr;

    std::shared_ptr<const Stream<TypeStimulation>> m_stream;

    StreamRendererStimulation() = delete;
};
}  // namespace Tracker
}  // namespace OpenViBE
