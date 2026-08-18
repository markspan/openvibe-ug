///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxTCPWriter.cpp
/// \brief Class of the box TCP Writer.
/// \author Jussi T. Lindgren (Inria).
/// \version 1.0.
/// \date 11/09/2013
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

#include "CBoxTCPWriter.hpp"

#include <ctime>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/predef/other/endian.h>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

using boost::asio::ip::tcp;

//--------------------------------------------------------------------------------
void CBoxTCPWriter::startAccept()
{
    tcp::socket* socket = new tcp::socket(m_ioContext);

    // Since startAccept will only be called inside ioContext.poll(), there is no need to access control m_sockets
    m_sockets.push_back(socket);

    this->getLogManager() << Kernel::LogLevel_Debug << "We are now using " << m_sockets.size() << " socket(s)\n";

    m_acceptor->async_accept(*socket, boost::bind(&CBoxTCPWriter::handleAccept, this, boost::asio::placeholders::error, socket));
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void CBoxTCPWriter::handleAccept(const boost::system::error_code& ec, tcp::socket* pSocket)
{
    if (!m_acceptor->is_open()) {
        this->getLogManager() << Kernel::LogLevel_Debug << "handleAccept() was called with acceptor already closed\n";
        return;
    }

    if (!ec) {
        this->getLogManager() << Kernel::LogLevel_Debug << "Handling a new incoming connection\n";

        // Send the known configuration to the client
        if (m_activeDecoder != &m_stimulationDecoder || m_outputStyle == TCPWRITER_RAW) {
            try {
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_rawVersion), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_endianness), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_frequency), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_nChannels), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_nSamplesPerChunk), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_reserved0), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_reserved1), sizeof(uint32_t)));
                write(*pSocket, boost::asio::buffer(static_cast<void*>(&m_reserved2), sizeof(uint32_t)));
            }
            catch (boost::system::system_error& error) {
                this->getLogManager() << Kernel::LogLevel_Warning << "Issue '" << error.code().message() << "' with writing header to client\n";
            }
        }
    }
    else {
        // @fixme should the socket be closed in this case?
        this->getLogManager() << Kernel::LogLevel_Warning << "Issue '" << ec.message() << "' with accepting a connection.\n";
    }
    // Already schedule the accepting of the next connection
    startAccept();
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxTCPWriter::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    boxContext.getInputType(0, m_inputType);
    if (m_inputType == OV_TypeId_StreamedMatrix) { m_activeDecoder = &m_matrixDecoder; }
    else if (m_inputType == OV_TypeId_Signal) { m_activeDecoder = &m_signalDecoder; }
    else { m_activeDecoder = &m_stimulationDecoder; }
    m_activeDecoder->initialize(*this, 0);

    const uint64_t port = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_outputStyle       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_rawVersion = htonl(1); // TCP Writer output format version
#if defined(BOOST_ENDIAN_LITTLE_BYTE)
    m_endianness = htonl(1);
#elif defined(BOOST_ENDIAN_BIG_BYTE)
    m_endianness = htonl(2);
#elif defined(BOOST_ENDIAN_LITTLE_WORD)
    m_endianness = htonl(3);
#else
    m_endianness = htonl(0);
    this->getLogManager() << Kernel::LogLevel_Warning << "Platform endianness was not recognized\n";
#endif

    m_frequency        = 0;
    m_nChannels        = 0;
    m_nSamplesPerChunk = 0;
    m_reserved0        = 0;
    m_reserved1        = 0;
    m_reserved2        = 0;

    this->getLogManager() << Kernel::LogLevel_Trace << "Setting up an acceptor at port " << port << "\n";

    try {
#ifdef TARGET_OS_Windows
        // On Windows, unless we deny reuse_addr, it seems several different servers can bind to socket. This is not what we want.
        m_acceptor = new tcp::acceptor(m_ioContext, tcp::endpoint(tcp::v4(), uint32_t(port)), false);
#else
        // On Linux, unless we allow reuse_addr, disconnection may set the socket to TIME_WAIT state and prevent opening it again until that state expires
        m_acceptor = new tcp::acceptor(m_ioContext, tcp::endpoint(tcp::v4(), uint32_t(port)), true);
#endif
    }
    catch (boost::system::system_error& error) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Got error '" << error.code().message() << "' allocating acceptor to port " << port << "\n";
        m_activeDecoder->uninitialize();
        m_activeDecoder = nullptr;
        m_acceptor      = nullptr; // if new throws, deleting the returned m_acceptor causes problems on Linux. So we NULL it.
        return false;
    }

    const boost::asio::socket_base::linger option(true, 0);
    m_acceptor->set_option(option);
    startAccept();
    m_ioContext.poll();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxTCPWriter::uninitialize()
{
    if (m_activeDecoder) {
        m_activeDecoder->uninitialize();
        m_activeDecoder = nullptr;
    }

    for (tcp::socket* sock : m_sockets) {
        if (sock->is_open()) {
            try {
                sock->shutdown(boost::asio::socket_base::shutdown_both);
                sock->close();
            }
            catch (boost::system::system_error& error) {
                // Just report...
                this->getLogManager() << Kernel::LogLevel_Warning << "Error in uninitialize() socket shutdown/close: '" << error.code().message() << "'\n";
            }
        }
    }
    m_ioContext.poll();
    m_ioContext.stop();

    for (tcp::socket* sock : m_sockets) { delete sock; }
    m_sockets.clear();

    delete m_acceptor;
    m_acceptor = nullptr;

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxTCPWriter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxTCPWriter::sendToClients(const void* buffer, const size_t size)
{
    if (size == 0 || buffer == nullptr) {
        // Nothing to send, shouldn't happen
        this->getLogManager() << Kernel::LogLevel_Warning << "Asked to send an empty buffer to clients (shouldn't happen)\n";
        return false;
    }

    auto it = m_sockets.begin();
    while (it != m_sockets.end()) {
        tcp::socket* tmpSock = (*it);
        bool hadError        = false;
        if (tmpSock->is_open()) {
            try { write(*tmpSock, boost::asio::buffer(buffer, size)); }
            catch (boost::system::system_error& error) {
                this->getLogManager() << Kernel::LogLevel_Warning << "Got error '" << error.code().message() << "' while trying to write to socket\n";
                hadError = true;
            }
        }
        if (hadError) {
            // Close the socket
            this->getLogManager() << Kernel::LogLevel_Debug << "Closing the socket\n";
            try {
                tmpSock->shutdown(boost::asio::socket_base::shutdown_both);
                tmpSock->close();
            }
            catch (boost::system::system_error& error) {
                // Just report...
                this->getLogManager() << Kernel::LogLevel_Warning << "Error while socket shutdown/close: '" << error.code().message() << "'\n";
            }
            m_ioContext.poll();
            delete tmpSock;
            it = m_sockets.erase(it);
        }
        else { ++it; }
    }
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxTCPWriter::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Process the asio loop once (e.g. see if there's new connections)
    m_ioContext.poll();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_activeDecoder->decode(i);
        if (m_activeDecoder->isHeaderReceived()) {
            // Matrix part
            if (m_activeDecoder == &m_matrixDecoder || m_activeDecoder == &m_signalDecoder) {
                // Casting to base class, ok
                Toolkit::TStreamedMatrixDecoder<CBoxTCPWriter>* decoder = (Toolkit::TStreamedMatrixDecoder<CBoxTCPWriter>*)(m_activeDecoder);

                const size_t nDim = decoder->getOutputMatrix()->getDimensionCount();
                switch (nDim) {
                    case 0:
                        this->getLogManager() << Kernel::LogLevel_Error << "Nothing to send, zero size matrix stream received\n";
                        return false;
                    case 1:
                        // Ok, this is a vector, openvibe style. Interpret it as 1 channel row vector.
                        m_nChannels = 1;
                        m_nSamplesPerChunk = decoder->getOutputMatrix()->getDimensionSize(0);
                        break;
                    case 2:
                        m_nChannels = decoder->getOutputMatrix()->getDimensionSize(0);
                        m_nSamplesPerChunk = decoder->getOutputMatrix()->getDimensionSize(1);
                        break;
                    default:
                        this->getLogManager() << Kernel::LogLevel_Error << "Only 1 and 2 dimensional matrices are supported\n";
                        return false;
                }

                // Conformance checking for all matrix based streams
                if (m_nChannels == 0 || m_nSamplesPerChunk == 0) {
                    this->getLogManager() << Kernel::LogLevel_Error << "For matrix-like inputs, both input dimensions must be larger than 0\n";
                    return false;
                }
            }

            // Signal specific part
            if (m_activeDecoder == &m_signalDecoder) { m_frequency = size_t(m_signalDecoder.getOutputSamplingRate()); }

            //if (m_activeDecoder == &m_stimDecoder) { }    // Stimulus, do nothing
        }
        if (m_activeDecoder->isBufferReceived()) {
            if (m_activeDecoder == &m_matrixDecoder) {
                const CMatrix* matrix = m_matrixDecoder.getOutputMatrix();

                sendToClients((void*)matrix->getBuffer(), matrix->getBufferElementCount() * sizeof(double));
            }
            else if (m_activeDecoder == &m_signalDecoder) {
                const CMatrix* matrix = m_signalDecoder.getOutputMatrix();

                sendToClients((void*)matrix->getBuffer(), matrix->getBufferElementCount() * sizeof(double));
            }
            else // stimulus
            {
                const CStimulationSet* stimSet = m_stimulationDecoder.getOutputStimulationSet();
                for (size_t j = 0; j < stimSet->size(); ++j) {
                    const uint64_t id = stimSet->getId(j);
                    // uint64_t date = stimSet->getDate(j);
                    this->getLogManager() << Kernel::LogLevel_Trace << "Sending out " << id << "\n";

                    switch (m_outputStyle) {
                        case TCPWRITER_RAW:
                            sendToClients((void*)&id, sizeof(id));
                            break;
                        case TCPWRITER_HEX:
                        {
                            std::string tmp = CIdentifier(id).str() + "\r\n";
                            const char* ptr = tmp.c_str();
                            sendToClients((void*)ptr, strlen(ptr));
                        }
                        break;
                        case TCPWRITER_STRING:
                        {
                            std::string tmp = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, id).toASCIIString();
                            if (tmp.empty()) { tmp = "Unregistered_stimulus " + CIdentifier(id).str(); }
                            tmp += "\r\n";

                            const char* ptr = tmp.c_str();
                            sendToClients((void*)ptr, strlen(ptr));
                        }
                        break;
                        default:
                            this->getLogManager() << Kernel::LogLevel_Error << "Unknown stimulus output style\n";
                            return false;
                    }
                }
            }
        }
        if (m_activeDecoder->isEndReceived()) { }
    }

    return true;
}
//--------------------------------------------------------------------------------

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE
