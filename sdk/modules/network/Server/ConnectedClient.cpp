/**
 * @file ConnectedClient.cpp
 * @brief Implementation of the ConnectedClient class for managing individual client connections in a server.
 * @author Florent LERAY
 * @date 2025-10-04
 *
 * This file contains the implementation of the ConnectedClient class, which is responsible for handling
 * communication with a connected client in a server application. It includes methods for reading data,
 * sending messages, and managing the connection lifecycle.
 *
 * The class utilizes a ProtocolMsgHandler to process incoming and outgoing messages according to a defined protocol.
 * It also manages a separate thread for reading data from the client socket.
 */
#include <ConnectedClient.h>
#include <CServer.h>
#include <thread>
#include <ProtocolMsgHandler.h>

#define BUFFER_SIZE 10 * 1024 * 1024 // 10 MB

namespace Network
{
    /**
     * @brief Constructor for the ConnectedClient class.
     * @param server Pointer to the server instance.
     * @param clientId The identifier for the connected client.
     * @param sock The socket associated with the connected client.
     * @note This constructor is intended for internal use by the CServer class.
     * @see CServer
     * @internal
     */
    ConnectedClient::ConnectedClient(CServer* server, int clientId, SOCKET sock)
    {
        m_server = server;
        m_clientId = clientId;
        m_sock = sock;
        m_bRun = false;
        m_buffer = new char[BUFFER_SIZE];
        m_protocolHandler = new ProtocolMsgHandler();
    }

    /**
     * @brief Move constructor.
     * @param rref Rvalue reference to another ConnectedClient instance.
     * @internal
     */
    ConnectedClient::ConnectedClient(ConnectedClient&& rref)
    {
        m_server = rref.m_server;
        m_clientId = rref.m_clientId;
        m_sock = rref.m_sock;
        m_name = std::move(rref.m_name);
        m_protocolHandler = std::move(rref.m_protocolHandler);

        rref.m_server = nullptr;
        rref.m_clientId = -1;
    }

    /**
     * @brief Destructor for the ConnectedClient class.
     * Cleans up resources and notifies the server to remove this client.
     * @internal
     */
    ConnectedClient::~ConnectedClient()
    {
        m_bRun = false;
        closesocket(m_sock);

        if (m_server)
        {
            m_server->removeClient(m_clientId);
        }
        m_thread.join();
        delete[] m_buffer;
        delete m_protocolHandler;
    }

    /**
     * @brief Move assignment operator.
     * @param rref Rvalue reference to another ConnectedClient instance.
     * @return Reference to this instance.
     * @internal
     */
    ConnectedClient& ConnectedClient::operator=(ConnectedClient&& rref)
    {
        if (this != &rref)
        {
            m_server = rref.m_server;
            m_clientId = rref.m_clientId;
            m_sock = rref.m_sock;
            m_name = std::move(rref.m_name);

            rref.m_server = nullptr;
            rref.m_clientId = -1;
        }
        return *this;
    }

    // ------------------------------------------------------------------------------------------------

    /**
     * @brief Returns the server instance.
     * @return The server instance.
     * @internal
     */
    CServer* ConnectedClient::server() const
    {
        return m_server;
    }

    /**
     * @brief Returns the client ID.
     * @return The client ID.
     * @internal
     */
    int ConnectedClient::clientId() const
    {
        return m_clientId;
    }

    /**
     * @brief Starts the read loop in a separate thread.
     * @internal
     */
    void ConnectedClient::run()
    {
        m_bRun = true;
        m_thread = std::thread(&ConnectedClient::readLoop, this);
        //m_thread.detach();
    }

    /**
     * @brief Main loop for reading data from the connected client.
     * @return The number of bytes read, or -1 if an error occurred.
     * @internal
     */
    int ConnectedClient::readLoop()
    {
        // Apply Socket Options (Nagle OFF, Timeouts 5s)
        if (ConfigureSocket(m_sock, 5) != 0)
        {
            fprintf(stderr, "Warning: Failed to set socket options for client %d\n", m_clientId);
        }

        m_bRun = true;
        bool connectionHealthy = true; // Flag to track protocol integrity  //TODO

        while (m_bRun && connectionHealthy)
        {
            int iResult = recv(m_sock, m_buffer, BUFFER_SIZE, 0);

            if (iResult > 0)
            {
                // Pass data to ProtocolHandler. 
                // IMPORTANT: logic inside receive() must handle splitting and check for MAGIC validity.
                bool protocolStatus = m_protocolHandler->receive((uint8_t*)m_buffer, iResult);

                if (!protocolStatus)
                {
                    // CORRUPTION DETECTED or PROTOCOL ERROR
                    // The handler detected garbage (bad Magic).
                    // We must disconnect immediately to avoid processing scrambled data.
                    fprintf(stderr, "Protocol corruption detected for client %d. Disconnecting.\n", m_clientId); //TODO
                    connectionHealthy = false;
                    m_bRun = false;
                }
                else
                {
                    // Data processed successfully (buffered or full message extracted)
                    // Check if we have complete messages ready to be dispatched
                    int msgSize = 0;
                    std::unique_ptr<uint8_t[]> msgData;

                    // Loop to handle MULTIPLE messages in a single recv buffer (Split)
                    while (m_protocolHandler->receivedData(msgSize, msgData))
                    {
                        if (msgData && msgSize > 0)
                        {
                            // Push to server queue
                            m_server->addNewData(std::move(msgData), msgSize, m_clientId);
                        }
                    }
                }
            }
            else if (iResult == 0)
            {
                // Graceful shutdown
                m_bRun = false;
            }
            else
            {
/*                // Error handling
                int err = WSAGetLastError(); //TODO: cross-platform error retrieval

                // Check if it's just a Timeout (keep alive) or a real error
#ifdef _WIN32
                if (err != WSAETIMEDOUT)
#else
                if (err != EWOULDBLOCK && err != EAGAIN)
#endif
                {*/
                    m_bRun = false;
                //}
                // If it is a timeout, we loop again (checking m_bRun)
            }
        }

        // cleanup...
        return 0;
    }


    bool ConnectedClient::disconnect()
    {
        bool bRes = false;

        if (m_thread.joinable())
        {
            m_bRun = false;
            closesocket(m_sock);
            m_thread.join();
            m_receivedData.clear();
            bRes = true;
        }

        return bRes;
    }

    /**
     * @brief Sends a message to the connected client.
     * @param data Pointer to the data to send.
     * @param size Size of the data in bytes.
     * @return The number of bytes sent, or -1 if an error occurred.
     * @internal
     */
    int ConnectedClient::sendMsg(void* data, size_t size)
    {
        int iSent = 0;
       
        std::lock_guard<std::mutex> lock(m_sendMutex); // THREAD-SAFETY: Protect the socket write operation

        if (m_protocolHandler->sendPacket(data, size))
        {
            int packetSize = 0;
            std::unique_ptr<uint8_t[]> packetData;

            m_protocolHandler->sendingData(packetSize, packetData);

            if (packetSize > 0 && packetData)
            {
                // Note: We use the raw send() here, assuming blocking mode with timeout set elsewhere
                iSent = send(m_sock, (const char*)packetData.get(), packetSize, 0);

                if (iSent == SOCKET_ERROR)
                {
                    // Log error ideally //Todo
                    iSent = -1;
                }
            }
        }

        return iSent;
    }

    /**
     * @brief Sets a custom protocol handler for the connected client.
     * @param handler Pointer to the custom ProtocolMsgHandler instance.
     * If nullptr is passed, a default ProtocolMsgHandler will be used.
     * @internal
     */
    void ConnectedClient::setCustomProtocolHandler(ProtocolMsgHandler* handler)
    {
        if (m_protocolHandler)
        {
            delete m_protocolHandler;
        }
        if (handler)
        {
            m_protocolHandler = handler->clone();
        }
        else
        {
            m_protocolHandler = new ProtocolMsgHandler();
        }
    }
} // namespace Network
