/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "TagProtocolHandler.h"
#include "TcpTaggingProtocol.h"

#include <ClientServerCommon.h>

#include <cstring>

namespace OpenViBE {
namespace Services {
namespace TcpTagging {

namespace CSC = Network::common;

bool TagProtocolHandler::receive(uint8_t* payload, size_t payloadSize)
{
    if (payload && payloadSize > 0) {
        m_accumBuffer.insert(m_accumBuffer.end(), payload, payload + payloadSize);
    }

    while (m_accumBuffer.size() >= sizeof(Tag)) {
        auto msg = std::make_unique<uint8_t[]>(CSC::typeIdSize + sizeof(Tag));

        const CSC::typeField tf = CSC::computTypeId(typeid(Tag).name());
        const uint64_t wireTypeId = tf.getNetworkInt64();
        std::memcpy(msg.get(), &wireTypeId, CSC::typeIdSize);
        std::memcpy(msg.get() + CSC::typeIdSize, m_accumBuffer.data(), sizeof(Tag));

        m_receivedMsgQueue.push(std::make_pair(static_cast<int>(CSC::typeIdSize + sizeof(Tag)), std::move(msg)));
        m_accumBuffer.erase(m_accumBuffer.begin(), m_accumBuffer.begin() + sizeof(Tag));
    }

    return true; // this protocol has no framing/CRC, so there is no corruption to detect at this level
}

Network::ProtocolMsgHandler* TagProtocolHandler::clone() const
{
    return new TagProtocolHandler();
}

} // namespace TcpTagging
} // namespace Services
} // namespace OpenViBE
