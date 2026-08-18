///-------------------------------------------------------------------------------------------------
/// 
/// \file ProcExternalProcessingHelper.h
/// \author J. T. Lindgren.
/// \version 1.0.
/// \date 25/01/2016.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///
///-------------------------------------------------------------------------------------------------

#pragma once

#include <communication/ovCMessagingClient.h>

#include <deque>

#include <iostream>

#include <mutex>
#include <condition_variable>

#include "EncodedChunk.h"

namespace OpenViBE {
namespace Tracker {

/// <summary> A messaging client base class thats associated with a buffer of chunks. </summary>
/// <details> The class is threaded. The thread connects to a single instance of OpenViBE's
/// External Processing Box that is assumed to be monitoring a specific port.
/// The derived clients are one - directional: they only send or receive data. </details>
class BufferedClient : protected Communication::MessagingClient
{
public:
    explicit BufferedClient(const uint32_t port) : m_port(port) { }
    void requestQuit();
    bool hasQuit();
    void start();

    CTime getStartTime()
    {
        // @todo might be a bit inefficient to have mutex here
        std::lock_guard<std::mutex> oLock(m_threadMutex);
        return m_startTime;
    }

    uint64_t getTime() override { return MessagingClient::getTime(); }

    static const CTime CLIENT_NOT_STARTED;

protected:
    bool connectClient();

    // Derived classes implement this to do either push or pull
    virtual bool step() = 0;

    uint32_t m_port = 0;

    // The mutex is used with the variables declared after it
    std::mutex m_threadMutex;
    std::condition_variable m_bufferCondition;

    std::deque<EncodedChunk*> m_buffer;
    bool m_pleaseQuit = false;
    bool m_hasQuit    = false;

    // Time when the client has connected and synced, CTime(-1) if not yet
    CTime m_startTime = CLIENT_NOT_STARTED;
};

/// <summary> A client dedicated to pushing data towards an External Processing Box. </summary>
class PushClient final : public BufferedClient
{
public:
    explicit PushClient(const uint32_t port) : BufferedClient(port) { }

    // Append a chunk to be sent out
    bool pushBuffer(const EncodedChunk& encodedChunk);

    // Force all pushed chunks to be sent
    void requestFlush();

protected:
    // implements push
    bool step() override;

    // Under the base class' mutex
    bool m_pleaseFlush = false;
};

/// <summary> A class dedicated to pulling data from an External Processing Box. </summary>
class PullClient final : public BufferedClient
{
public:
    explicit PullClient(const uint32_t port) : BufferedClient(port) { }

    // Get the oldest chunk received. Returns false if none.
    bool pullBuffer(EncodedChunk& chunk);

    bool isEndReceived() override { return BufferedClient::isEndReceived(); }

protected:
    // implements pull
    bool step() override;

    // Keep polling the sender for chunks as long as there is one available
    bool popMessagesToBuffer();
};
}  // namespace Tracker
}  // namespace OpenViBE
