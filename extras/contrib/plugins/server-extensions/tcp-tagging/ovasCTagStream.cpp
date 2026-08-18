#include "ovasCTagStream.h"

#include <system/ovCTime.h>
#include <boost/bind.hpp>

#include <iostream>

#include <thread>
#include <mutex>

namespace OpenViBE {
namespace AcquisitionServer {
namespace Plugins {

void CTagQueue::push(const Tag& tag)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_queue.push(tag);
}

bool CTagQueue::pop(Tag& tag)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_queue.empty()) { return false; }
    tag = m_queue.front();
    m_queue.pop();
    return true;
}

void CTagSession::start()
{
    m_errorState = 0;
    startRead();
}

void CTagSession::startRead()
{
    // Caveat: a shared pointer is used (instead of simply using this) to ensure that this instance of TagSession is still alive when the callback is called.
    async_read(m_socket, boost::asio::buffer(static_cast<void*>(&m_tag), sizeof(Tag)), boost::bind(&CTagSession::handleRead, shared_from_this(), _1));
}

void CTagSession::handleRead(const boost::system::error_code& error)
{
    if (!error) {
        if (m_tag.timestamp == 0 || (m_tag.flags & FLAG_AUTOSTAMP_SERVERSIDE)) {
            // Client didn't provide timestamp or asked the server to do it. Stamp current time.
            m_tag.timestamp = System::Time::zgetTimeRaw(false);
        }
        else if (!(m_tag.flags & FLAG_FPTIME)) {
            // Client provided stamp but not in FPTIME
            m_tag.timestamp = System::Time::zgetTimeRaw(false);
            if (!(m_errorState & (1LL << 1))) {
                // @fixme not appropriate to print errors from a thread, but better than silent fail
                std::cout << "[WARNING] TCP Tagging: Received tag(s) not in fixed point time. Not supported, will replace with server time.\n";
                m_errorState |= (1LL << 1);
            }
        }

        // Push tag to the queue.
        m_queuePtr->push(m_tag);

        // Continue reading.
        startRead();
    }
}


CTagServer::CTagServer(const SharedQueuePtr& queue, int port)
    : m_acceptor(m_ioService), m_queuePtr(queue)
{
    boost::asio::ip::tcp::endpoint endp = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), uint16_t(port));
    m_acceptor.open(endp.protocol());

    // Try to make sure that the port cannot be used by multiple processes
    boost::asio::socket_base::reuse_address option(false);
    m_acceptor.set_option(option);

    m_acceptor.bind(endp);
    m_acceptor.listen();
}


void CTagServer::run()
{
    try {
        startAccept();
        m_ioService.run();
    }
    catch (std::exception&) {
        // TODO: log error message (needs to be thread-safe)
    }
}

void CTagServer::startAccept()
{
    const SharedSessionPtr newSession(new CTagSession(m_ioService, m_queuePtr));
    // Note: if this instance of CTagSever is destroyed then the associated io_service is destroyed as well.
    // Therefore the call-back will never be called if this instance is destroyed and it is safe to use this instead of a shared pointer.

    m_acceptor.async_accept(newSession->socket(), boost::bind(&CTagServer::handleAccept, this, newSession, _1));
}

void CTagServer::handleAccept(const SharedSessionPtr& session, const boost::system::error_code& error)
{
    if (!error) { session->start(); }
    startAccept();
}

CTagStream::CTagStream(const int port) : m_queuePtr(new CTagQueue), m_port(port)
{
    // can throw exceptions, e.g. when the port is already in use.
    m_serverPtr.reset(new CTagServer(m_queuePtr, m_port));
    m_threadPtr.reset(new std::thread(&CTagStream::startServer, this));
}

CTagStream::~CTagStream()
{
    // m_serverPtr and m_threadPtr cannot be null
    m_serverPtr->stop();
    m_threadPtr->join();
}

}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
