//
// OpenViBE Tracker
//


#pragma once

#include "StreamRendererLabel.h"
#include "TypeChannelLocalization.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererChannelLocalization 
 * \brief Renderer for Channel Localization streams
 * \author J. T. Lindgren
 *
 * @ fixme this class inherits a lot of junk from the root class which is not initialized properly and
 * hence cannot be used. but if just the interface below is used, its alright. Can't make base class
 * private as we store the renderer as a pointer to the base.
 *
 */
class StreamRendererChannelLocalization final : public StreamRendererLabel
{
public:
    StreamRendererChannelLocalization(const Kernel::IKernelContext& ctx, const std::shared_ptr<const Stream<TypeChannelLocalization>>& stream)
        : StreamRendererLabel(ctx), m_Stream(stream) { }

    CString renderAsText(const size_t indent) const override;

    bool showChunkList() override;

protected:
    std::shared_ptr<const Stream<TypeChannelLocalization>> m_Stream;
};
}  // namespace Tracker
}  // namespace OpenViBE
