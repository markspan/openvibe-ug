#include <iostream>
#include <thread>
#include <sstream>

#include <system/ovCTime.h>

#include "ProcExternalProcessingHelper.h"

namespace OpenViBE {
namespace Tracker {

const CTime BufferedClient::CLIENT_NOT_STARTED = CTime::max();

void BufferedClient::requestQuit()
{
    std::lock_guard<std::mutex> oLock(m_threadMutex);
    m_pleaseQuit = true;
    m_bufferCondition.notify_one();
}

bool BufferedClient::hasQuit()
{
    std::lock_guard<std::mutex> oLock(m_threadMutex);
    return m_hasQuit;
}

void BufferedClient::start()
{
    if (!connectClient()) {
        std::lock_guard<std::mutex> oLock(m_threadMutex);
        m_hasQuit = true;
        return;
    }

    while (true) {
        // Push or pull, uses lock internally
        if (!step()) { break; }

        {
            std::lock_guard<std::mutex> oLock(m_threadMutex);

            // Exit the loop if we're told to quit or if we've lost the connection
            if (m_pleaseQuit || !isConnected() || isEndReceived()) { break; }
        }
    }

    // Shut the connection 
    this->close();

    {
        std::lock_guard<std::mutex> oLock(m_threadMutex);
        m_hasQuit = true;
    }

    // The thread will exit here and can be joined
}

bool BufferedClient::connectClient()
{
    static int connectId = 0;

    std::stringstream cId;
    cId << std::string("tracker");
    cId << connectId++;

    const std::string connectionID = cId.str();
    this->setConnectionID(connectionID);
    int errorCount = 0;
    while (!this->connect("127.0.0.1", m_port)) {
        const ELibraryError error = this->getLastError();

        if (error == Socket_FailedToConnect) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (errorCount++ > 20) {
                std::cout << "Designer not responding on port  " << m_port << " retried 5 secs" << std::endl;
                return false;
            }
        }
        else {
            std::cout << "Error " << error << std::endl;
            return false;
        }

        //if (s_DidRequestForcedQuit) { exit(EXIT_SUCCESS); }
    }

    // Announce to server that the box has finished initializing and wait for acknowledgement
    errorCount = 0;
    while (!this->waitForSyncMessage()) {
        if (errorCount++ > 10) {
            std::cout << "Server not syncing in port " << m_port << std::endl;
            this->close();
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    this->pushLog(Communication::ELogLevel::LogLevel_Info, "Received Ping");

    this->pushSync();
    this->pushLog(Communication::ELogLevel::LogLevel_Info, "Sent Pong");

    {
        std::lock_guard<std::mutex> oLock(m_threadMutex);
        m_startTime = this->getTime();
    }

    return true;
}

bool PushClient::pushBuffer(const EncodedChunk& encodedChunk)
{
    {
        std::lock_guard<std::mutex> oLock(m_threadMutex);
        if (!m_pleaseQuit) {
            EncodedChunk* buffer = new EncodedChunk(encodedChunk);
            m_buffer.push_back(buffer);
        }
    }

    // No big harm notifying in any case, though if in 'quit' state, the quit request has already notified
    m_bufferCondition.notify_one();

    return true;
}

bool PushClient::step()
{
    std::unique_lock<std::mutex> oLock(m_threadMutex, std::defer_lock);

    oLock.lock();

    // Normal condition for the wait to exit is the flush request or having more data
    m_bufferCondition.wait(oLock, [this]() { return (m_pleaseFlush || !m_buffer.empty() || m_pleaseQuit || !isConnected() || isEndReceived()); });

    while (!m_buffer.empty()) {
        EncodedChunk* chunk = m_buffer[0];
        m_buffer.pop_front();

        if (!this->pushEBML(chunk->m_StreamIndex, chunk->m_StartTime.time(), chunk->m_EndTime.time(),
                            std::make_shared<const std::vector<uint8_t>>(chunk->m_Buffer))) {
            std::cerr << "Failed to push EBML.\n";
            std::cerr << "Error " << this->getLastError() << "\n";
            oLock.unlock();
            return false;
        }

        delete chunk;
    }

    if (m_pleaseFlush) {
        this->pushSync();
        m_pleaseFlush = false;

        // We don't use condition variable here as users of the client should never 
        // be able to interrupt this wait unless error state is reached
        while (!m_pleaseQuit && isConnected() && !isEndReceived() && !this->waitForSyncMessage()) {
            oLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            oLock.lock();
        }
    }

    oLock.unlock();

    return true;
}

void PushClient::requestFlush()
{
    std::lock_guard<std::mutex> oLock(m_threadMutex);
    m_pleaseFlush = true;
    m_bufferCondition.notify_one();
}

bool PullClient::pullBuffer(EncodedChunk& chunk)
{
    std::lock_guard<std::mutex> oLock(m_threadMutex);

    if (m_buffer.empty()) { return false; }

    EncodedChunk* ptr = m_buffer[0];
    m_buffer.pop_front();
    chunk = *ptr;
    delete ptr;

    return true;
}

bool PullClient::popMessagesToBuffer()
{
    uint64_t packetId, startTime, endTime;
    size_t streamIndex;
    std::shared_ptr<const std::vector<uint8_t>> ebml;
    bool gotSomething = false;

    while (this->popEBML(packetId, streamIndex, startTime, endTime, ebml)) {
        // @todo optimize by passing in the buffer to popEBML already
        EncodedChunk* chunk = new EncodedChunk();

        chunk->m_StartTime   = startTime;
        chunk->m_EndTime     = endTime;
        chunk->m_StreamIndex = streamIndex;

        // @fixme inefficient to query for each chunk since its stream specific
        uint64_t id;
        std::string streamName;
        this->getInput(chunk->m_StreamIndex, id, chunk->m_StreamType, streamName);

        chunk->m_Buffer.resize(ebml->size());
        for (size_t i = 0; i < ebml->size(); ++i) { chunk->m_Buffer[i] = (*ebml)[i]; }

        {
            std::lock_guard<std::mutex> oLock(m_threadMutex);
            m_buffer.push_back(chunk);
        }

        gotSomething = true;

        //std::cout << "Got pkg " << packetId << " idx " << streamIndex << " siz " << chunk.bufferData.size() << "\n";
    }

    return gotSomething;
}

// pull
bool PullClient::step()
{
    // Pull items until we get the sync message (no more to send)
    while (!this->waitForSyncMessage() && this->isConnected() && !this->isEndReceived()) {
        popMessagesToBuffer();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // std::this_thread::yield();
    }

    // Got sync, pull whatever remaining buffered on the sender
    popMessagesToBuffer();

    if (!this->isConnected() || this->isEndReceived()) { return false; }

    // Notify we've processed everything
    this->pushSync();

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
