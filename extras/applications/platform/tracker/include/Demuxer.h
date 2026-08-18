#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <map>
#include <vector>

#include <openvibe/ov_all.h>
#include <openvibe/CMatrix.hpp>

#include <socket/IConnectionServer.h>

#include <fs/Files.h>

#include <ebml/CReader.h>
#include <ebml/CReaderHelper.h>

#include "StreamBundle.h"

#include "EBMLSourceFile.h"
#include "Contexted.h"

#include "EncodedChunk.h"
#include "Decoder.h"

class CClientHandler;
class OutputEncoder;

class EBMLSource;

namespace OpenViBE {
namespace Tracker {

/**
 * \class Demuxer 
 * \brief Demuxes (and decodes) EBML streams
 * \details EBML containers such as .ov multiplex one or more streams. Demuxer takes an EBML source and splits and decodes it to a Stream Bundle.
 *
 * It has similar purpose as the Generic Stream Reader box.
 *
 * \author J. T. Lindgren
 *
 */
class Demuxer final : public EBML::IReaderCallback, protected Contexted
{
public:
    Demuxer(const Kernel::IKernelContext& ctx, EBMLSourceFile& origin, StreamBundle& target)
        : Contexted(ctx), m_origin(origin), m_target(target), m_reader(*this) { initialize(); }

    bool initialize();
    bool uninitialize() const;

    // Reads some data from the origin and decodes it to target. Returns false on EOF.
    bool step();
    static bool stop() { return true; }

protected:
    EBMLSourceFile& m_origin;
    StreamBundle& m_target;

    bool m_playingStarted = false;

    EBML::CReader m_reader;
    EBML::CReaderHelper m_readerHelper;

    EncodedChunk m_pendingChunk;

    uint32_t m_chunksSent = 0;

    bool m_pending       = false;
    bool m_hasEBMLHeader = false;

    std::stack<EBML::CIdentifier> m_nodes;
    std::map<uint32_t, uint32_t> m_streamIdxToOutputIdxs;
    std::map<uint32_t, CIdentifier> m_streamIdxToTypeIDs;

    std::vector<size_t> m_chunkStreamType;

    std::vector<DecoderBase*> m_decoders;

private:
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;
};
}  // namespace Tracker
}  // namespace OpenViBE
