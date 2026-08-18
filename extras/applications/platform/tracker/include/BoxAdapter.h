#pragma once

#include <openvibe/ov_all.h>

#include "Contexted.h"

#include "StreamBundle.h"

#include "BoxAdapterHelper.h"

#include "Encoder.h"
#include "Decoder.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class BoxAdapter 
 * \brief A partially abstract class allowing processing data with OpenViBE boxes using a simple interface. 
 *
 * The class implements a few of its members, but derived classes should implement the method spool() that feeds the data to the box,
 * as well as members allowing setting the sources and destinations of the data.
 *
 * \author J. T. Lindgren
 *
 */
class BoxAdapter : protected Contexted
{
public:
    BoxAdapter(const Kernel::IKernelContext& ctx, const CIdentifier& algorithmId) : Contexted(ctx), m_boxAlgorithmCtx(ctx), m_algorithmID(algorithmId) { }

    ~BoxAdapter() override { }

    virtual bool initialize();
    virtual bool spool(const bool verbose) = 0;
    virtual bool uninitialize();

    virtual TrackerBox& getBox() { return m_boxAlgorithmCtx.m_StaticBoxCtx; }
    virtual CIdentifier getAlgorithmId() { return m_algorithmID; }

protected:
    TrackerBoxAlgorithmContext m_boxAlgorithmCtx;
    Plugins::IBoxAlgorithm* m_boxAlgorithm = nullptr;
    CIdentifier m_algorithmID              = CIdentifier::undefined();
};

/**
 * \class BoxAdapterStream 
 * \brief A box adapter that reads from a Stream and writes to another Stream, having effect equivalent to outStream = process(inStream);
 * \author J. T. Lindgren
 *
 */
class BoxAdapterStream final : public BoxAdapter
{
public:
    BoxAdapterStream(const Kernel::IKernelContext& ctx, const CIdentifier& algorithmId) : BoxAdapter(ctx, algorithmId) { }

    void setSource(const StreamPtr& source) { m_src = source; }
    void setTarget(const StreamPtr& target) { m_dst = target; }

    bool initialize() override;

    // Process the whole stream from source to target
    bool spool(const bool verbose) override;

protected:
    StreamPtr m_src = nullptr, m_dst = nullptr;
};

/**
 * \class BoxAdapterBundle 
 * \brief A box adapter that reads from a StreamBundle and writes to another StreamBundle
 * \author J. T. Lindgren
 *
 */
class BoxAdapterBundle : public BoxAdapter
{
public:
    BoxAdapterBundle(const Kernel::IKernelContext& ctx, const CIdentifier& algorithmId) : BoxAdapter(ctx, algorithmId) { }

    ~BoxAdapterBundle() override;

    virtual void setSource(StreamBundle* source) { m_src = source; }
    virtual void setTarget(StreamBundle* target) { m_dst = target; }

    bool initialize() override;

    // Process the whole bundle from source to target
    bool spool(const bool verbose) override;

protected:
    StreamBundle *m_src = nullptr, *m_dst = nullptr;
    std::vector<EncoderBase*> m_encoders;
    std::vector<DecoderBase*> m_decoders;
};
}  // namespace Tracker
}  // namespace OpenViBE
