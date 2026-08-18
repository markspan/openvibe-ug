/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <cstdint>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include <ProtocolMsgHandler.h>

namespace OpenViBE {
namespace Services {
namespace TcpTagging {

// Custom Network::ProtocolMsgHandler for the legacy tcp-tagging wire protocol:
// a fixed, unframed 24-byte Tag struct with no magic/size/CRC at all (unlike the
// module's default framing). This handler's only job is byte reassembly: it
// accumulates incoming bytes and, once a full Tag is available, prefixes it with
// a synthesized 8-byte typeId (matching common::computTypeId(typeid(Tag).name()))
// so Network::CServer's dispatch loop (which always expects a typeId-prefixed
// message) routes it to the registered read callback. Flag/timestamp semantics
// are NOT interpreted here; that happens in TcpTaggingService's read callback.
class TagProtocolHandler : public Network::ProtocolMsgHandler
{
public:
    bool receive(uint8_t* payload, size_t payloadSize) override;
    Network::ProtocolMsgHandler* clone() const override;

private:
    std::vector<uint8_t> m_accumBuffer;
};

} // namespace TcpTagging
} // namespace Services
} // namespace OpenViBE
