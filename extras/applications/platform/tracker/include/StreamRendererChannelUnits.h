//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererLabel.h"
#include "TypeChannelUnits.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererChannelUnits
 * \brief Renderer for Channel Units streams
 * \author J. T. Lindgren
 *
 * @ fixme this class inherits a lot of junk from the root class which is not initialized properly and
 * hence cannot be used. but if just the interface below is used, its alright. Can't make base class
 * private as we store the renderer as a pointer to the base.
 *
 */
class StreamRendererChannelUnits final : public StreamRendererLabel
{
public:
    StreamRendererChannelUnits(const Kernel::IKernelContext& ctx, const std::shared_ptr<const Stream<TypeChannelUnits>>& stream)
        : StreamRendererLabel(ctx), m_stream(stream) { }

    CString renderAsText(const size_t indent) const override;
    bool showChunkList() override;

protected:
    std::shared_ptr<const Stream<TypeChannelUnits>> m_stream;
};
}  // namespace Tracker
}  // namespace OpenViBE
