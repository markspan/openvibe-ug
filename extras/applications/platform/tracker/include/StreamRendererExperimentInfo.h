//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererLabel.h"
#include "TypeExperimentInfo.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererExperimentInfo 
 * \brief Renderer for Experiment Information streams
 * \author J. T. Lindgren
 *
 * @ fixme this class inherits a lot of junk from the root class which is not initialized properly and
 * hence cannot be used. but if just the interface below is used, its alright. Can't make base class
 * private as we store the renderer as a pointer to the base.
 *
 */
class StreamRendererExperimentInfo final : public StreamRendererLabel
{
public:
    StreamRendererExperimentInfo(const Kernel::IKernelContext& ctx, const std::shared_ptr<const Stream<TypeExperimentInfo>>& stream)
        : StreamRendererLabel(ctx), m_stream(stream) { }

    CString renderAsText(const size_t indent) const override;

    bool showChunkList() override;

protected:
    std::shared_ptr<const Stream<TypeExperimentInfo>> m_stream;
};
}  // namespace Tracker
}  // namespace OpenViBE
