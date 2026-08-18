#pragma once

/**
 * @file ConnectedClient.h
 * @brief Declaration of the ConnectedClient class for managing connected clients in a server application.
 * @details This class encapsulates the details of a connected client, including its socket, ID, and communication methods.
 * @note This class is intended for internal use by the CServer class and should not be instantiated directly.
 * @author Florent LERAY
 * @date 2025-10-04
 */

#include <string>
#include <typeinfo>
#include <CServer.h>

#include <map>
#include <mutex>
#include <vector>
#include <thread>

namespace Network
{
    class CServer;
    class ProtocolMsgHandler;

    /**
     * @class ConnectedClient
     * @brief ConnectedClient represents a client connected to the server.
     * It encapsulates the server instance, client ID, socket, and name.
     * Provides methods to read from and write data to the client.
     * @note You should not create instances of this class directly; instead, use CServer to manage connected clients.
     * This class is designed to be used internally by the CServer class.
     * @see CServer
     * @internal
     */
    class ConnectedClient
    {
    public:
        ConnectedClient() = delete;
        ConnectedClient(CServer* server, int clientId, SOCKET sock);

        ConnectedClient(const ConnectedClient&) = delete;
        ConnectedClient& operator=(const ConnectedClient&) = delete;

        ConnectedClient(ConnectedClient&& rref);
        ConnectedClient& operator=(ConnectedClient&& rref);

        ~ConnectedClient();

        CServer* server() const;
        int clientId() const;


        int sendMsg(void* data, size_t size);

        void run();

        bool isRunning() const { return m_bRun; }
        bool disconnect();

        void setCustomProtocolHandler(ProtocolMsgHandler* handler);

    private:
        int readLoop();

    private:
        CServer* m_server;
        int m_clientId;
        SOCKET m_sock;
        bool m_bRun;
        char* m_buffer;

        std::string m_name;

        bool m_bDataReceiving;
        size_t m_expectedBodySize;

        std::vector<uint8_t> m_receivedData;
        ProtocolMsgHandler* m_protocolHandler;

        std::thread m_thread;
        std::mutex m_sendMutex;
    };
} // namespace Network
