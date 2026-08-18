#pragma once

#include "BoxAdapter.h"

static const OpenViBE::CIdentifier STREAM_WRITER_ID = OpenViBE::CIdentifier(0x09C92218, 0x7C1216F8);

namespace OpenViBE {
namespace Tracker {

/**
 * \class BoxAdapterGenericStreamWriter 
 * \brief A specific wrapper for Generic Stream Writer allowing to write tracks as .ov files from the Tracker
 * \details
 * This class can be considered as the counterpart of the Demuxer class, except here we simply wrap the Generic Stream Writer
 * code instead of developing a new class. 
 * \author J. T. Lindgren
 *
 */
class BoxAdapterGenericStreamWriter final : public BoxAdapterBundle
{
public:
    BoxAdapterGenericStreamWriter(const Kernel::IKernelContext& ctx, StreamBundle& source, const std::string& filename)
        : BoxAdapterBundle(ctx, STREAM_WRITER_ID), m_filename(filename) { BoxAdapterBundle::setSource(&source); }

    bool initialize() override
    {
        // @fixme these should actually come from the box description
        Kernel::IBox* boxCtx = const_cast<Kernel::IBox*>(m_boxAlgorithmCtx.getStaticBoxContext());
        boxCtx->addSetting("Filename", 0, CString(m_filename.c_str()));
        boxCtx->addSetting("Compress", 0, "false");

        BoxAdapterBundle::initialize();

        return true;
    }

protected:
    std::string m_filename;
};
}  // namespace Tracker
}  // namespace OpenViBE
