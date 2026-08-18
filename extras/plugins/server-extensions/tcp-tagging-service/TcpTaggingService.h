/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <memory>

#include <openvibe/parameters/parameters.h>
#include <openvibe/Services/IService.h>

#include "TcpTaggingProtocol.h"

#include <TcpTaggingService_Export.h>

namespace OpenViBE {
namespace Services {

class TcpTaggingServicePrivate;

/**
 * @brief Singleton IService exposing a dedicated TCP server that speaks the
 * legacy tcp-tagging wire protocol (see TcpTaggingProtocol.h), for backward
 * compatibility with existing external tag senders.
 */
class TCPTAGGINGSERVICE_EXPORT TcpTaggingService : public IService
{
public:
    ~TcpTaggingService() override = default;

    static TcpTaggingService& instance();

    // Thread-safe, non-blocking. Returns true and fills 'out' if a tag was
    // available, false otherwise.
    bool popTag(TcpTagging::Tag& out);

    // Discards any tags queued so far. Intended to be called by the consuming
    // driver at the start of an acquisition, so stale tags received before
    // the driver started polling aren't handed to it.
    void clearQueue();

    Parameters::ParameterInt* getPortParameter();
    Parameters::ParameterBool* getRunParameter();

    std::list<Parameters::ParameterTraits*> parameters() override;

private:
    TcpTaggingService();

    TcpTaggingService(const TcpTaggingService&) = delete;
    TcpTaggingService(TcpTaggingService&) = delete;
    TcpTaggingService& operator=(const TcpTaggingService&) = delete;
    TcpTaggingService& operator=(TcpTaggingService&) = delete;

    bool onTagReceived(void* data, size_t size, uint64_t clientId);

private:
    static std::unique_ptr<TcpTaggingServicePrivate> d;
};

} // namespace Services
} // namespace OpenViBE
