//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererBase.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererNothing
 * \brief Renders nothing, to avoid null ptrs when we don't want even a label rendered
 * \author J. T. Lindgren
 *
 * @ fixme this class inherits a lot of junk from the base class which is not initialized properly and
 * hence cannot be used. but if just the interface below is used, its alright. Can't make base class
 * private as we store the renderer as a pointer to the base.
 *
 */
class StreamRendererNothing final : public StreamRendererBase
{
public:
    explicit StreamRendererNothing(const Kernel::IKernelContext& ctx) : StreamRendererBase(ctx) { }

    bool initialize() override { return true; }
    bool uninitialize() override { return true; }
    bool setTitle(const char* /*title*/) override { return true; }

    bool spool(const CTime /*startTime*/, const CTime /*endTime*/) override { return true; }

    bool setRulerVisibility(bool /*isVisible*/) override { return true; }

    bool showChunkList() override { return true; }

    CString renderAsText(const size_t indent) const override
    {
        return (std::string(indent, ' ') + "Stream not selected and hence has no renderer in current mode\n").c_str();
    }

protected:
    bool reset(CTime /*startTime*/, CTime /*endTime*/) override { return true; }
    bool realize() override { return true; }
};
}  // namespace Tracker
}  // namespace OpenViBE
