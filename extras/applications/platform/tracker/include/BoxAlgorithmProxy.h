//
//

#pragma once

#include <string>
#include <vector>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>

#include "Contexted.h"

namespace OpenViBE {
namespace Kernel {

/**
 * \class IBoxIOProxy
 * \brief This proxy is intended for reusing codecs from the toolkit. 
 * @fixme it'd be more elegant not to duplicate this interface as a very similar one is already implemented in BoxAdapterHelper. 
 * however unifying the two would need some work.
 * \author J. T. Lindgren
 *
 */
class OV_API_Export IBoxIOProxy final : public IBoxIO
{
public:
    size_t getInputChunkCount(const size_t /*index*/) const override { return 0; }

    bool getInputChunk(const size_t /*inputIdx*/, const size_t /*chunkIdx*/, uint64_t& /*startTime*/, uint64_t& /*endTime*/, size_t& /*chunkSize*/,
                       const uint8_t*& /*chunkBuffer*/) const override { return true; }

    const CMemoryBuffer* getInputChunk(const size_t /*inputIdx*/, const size_t /*chunkIdx*/) const override { return &m_InBuffer; }
    uint64_t getInputChunkStartTime(const size_t /*inputIdx*/, const size_t /*chunkIdx*/) const override { return 0; }
    uint64_t getInputChunkEndTime(const size_t /*inputIdx*/, const size_t /*chunkIdx*/) const override { return 0; }
    bool markInputAsDeprecated(const size_t /*inputIdx*/, const size_t /*chunkIdx*/) override { return true; }
    size_t getOutputChunkSize(const size_t /*index*/) const override { return m_OutBuffer.getSize(); }

    bool setOutputChunkSize(const size_t /*index*/, const size_t size, const bool /*discard*/  = true) override { return m_OutBuffer.setSize(size, true); }

    uint8_t* getOutputChunkBuffer(const size_t /*index*/) override { return m_OutBuffer.getDirectPointer(); }
    bool appendOutputChunkData(const size_t /*index*/, const uint8_t* /*buffer*/, const size_t /*size*/) override { return false; } // not implemented

    CMemoryBuffer* getOutputChunk(const size_t /*outputIdx*/) override { return &m_OutBuffer; }

    bool markOutputAsReadyToSend(const size_t /*outputIdx*/, const uint64_t /*startTime*/, const uint64_t /*endTime*/) override { return true; }

    CIdentifier getClassIdentifier() const override { return 0; }

    CMemoryBuffer m_InBuffer;
    CMemoryBuffer m_OutBuffer;
};
}  // namespace Kernel
}  // namespace OpenViBE

/**
 * \class BoxAlgorithmProxy 
 * \brief This proxy is needed in order to use the stream codecs from the toolkit
 * \author J. T. Lindgren
 *
 */
class BoxAlgorithmProxy final : protected OpenViBE::Tracker::Contexted
{
public:
    explicit BoxAlgorithmProxy(const OpenViBE::Kernel::IKernelContext& ctx) : Contexted(ctx) { }

    OpenViBE::Kernel::IBoxIO& getDynamicBoxContext() { return dummy; }
    static bool markAlgorithmAsReadyToProcess() { return true; }

    OpenViBE::Kernel::IAlgorithmManager& getAlgorithmManager() const override { return Contexted::getAlgorithmManager(); }

    OpenViBE::Kernel::IBoxIOProxy dummy;
};
