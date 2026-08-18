//
// OpenViBE Tracker
//

#pragma once

#include "StreamRendererBase.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererLabel 
 * \brief Renderer for any stream that is simply visualized as a labeled widget
 * \author J. T. Lindgren
 *
 * @ fixme this class inherits a lot of junk from the base class which is not initialized properly and
 * hence cannot be used. but if just the interface below is used, its alright. Can't make base class
 * private as we store the renderer as a pointer to the base.
 *
 */
class StreamRendererLabel : public StreamRendererBase
{
public:
    explicit StreamRendererLabel(const Kernel::IKernelContext& ctx) : StreamRendererBase(ctx) { }

    bool initialize() override;
    bool uninitialize() override { return true; }
    bool setTitle(const char* title) override;

    bool spool(const CTime /*startTime*/, const CTime /*endTime*/) override { return true; }

    bool setRulerVisibility(bool /*isVisible*/) override { return true; }

    virtual bool showChunkList(const char* title);

protected:
    bool reset(CTime /*startTime*/, CTime /*endTime*/) override { return true; }
    bool realize() override { return true; }

    GtkWidget* m_label = nullptr;
};
}  // namespace Tracker
}  // namespace OpenViBE
