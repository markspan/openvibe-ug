/**
 * @file CServer.cpp
 * @brief Implementation of the CServer class for managing server-side network operations.
 * @details This file contains the implementation of the CServer class, which handles
 * client connections, data transmission, and callback management for a network server.
 * It utilizes sockets for communication and supports custom protocol handling.
 * The class is designed to be thread-safe and efficient in handling multiple clients.
 */

#include <CServer.h>
#include <ConnectedClient.h>
#include <ClientServerCommon.h>
#include <CNetwork.h>
#include <ProtocolMsgHandler.h>

#include <string>
#include <typeinfo>
#include <map>

#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include "CClient.h"


namespace Network
{
    namespace CSC = common;
    /**
     * @internal
     * @class CServerPrivate
     * @brief Private implementation details for the CServer class.
     * @details This class encapsulates the private data and operations of the CServer class,
     * including client management and callback registration.
     */
    class CServerPrivate
    {
    public:
        CNetwork* parentNetwork = nullptr;

        std::map<uint64_t, readFunc>  readCallbackMap;
        std::map<uint64_t, writeFunc> writeCallbackMap;
        std::map<int, ConnectedClient*> clients; // Maps client ID to ConnectedClient
        std::map<int, std::vector<std::pair<size_t, std::unique_ptr<uint8_t[]>>>> receivedData; // Maps client ID to received data
        std::list<clientRemouved> clientRemovedCallbacks;

        std::thread threadLoopEntranceConnection;
        std::thread threadLoopDataFromClient;

        uint16_t maxClientsCount = 10;
        uint16_t port = 0;
        SOCKET sock = INVALID_SOCKET;
        bool bRun = false;

        //declare condition variable and mutex for data reception
        std::condition_variable dataAvailable;
        std::mutex dataMutex;

        std::mutex clientsMutex; // Mutex to protect access to the clients map

        int nextClientId = 1; // Start client IDs from 1

        ProtocolMsgHandler* protocolHandler = nullptr;
    };

    /**
     * @brief Default constructor for the CServer class.
     */
    CServer::CServer(CNetwork *network) : d(new CServerPrivate())
    {
        d->maxClientsCount = 10; // Default maximum number of clients
        d->port = 0; // Default port
        d->sock = INVALID_SOCKET; // Initialize socket to invalid
        d->bRun = true; // Server is running by default
        d->parentNetwork = network;
    }

    /**
     * @brief Destructor for the CServer class.
     */
    CServer::~CServer()
    {
        stop();
        if (d->parentNetwork)
        {
            d->parentNetwork->removeServer(this);
        }
        delete d;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Sets the port for the server.
     * @param port The port number to set.
     * @return true if the port was successfully set, false otherwise.
     */
    bool CServer::setPort(uint16_t port)
    {
        d->port = port;
        return true;
    }

    /**
     * @brief Retrieves the parent network of the server.
     * @return Pointer to the parent CNetwork instance.
     */
    CNetwork* CServer::getNetwork() const
    {
        return d->parentNetwork;
    }

    /**
     * @brief Detaches the server from its parent network.
     */
    void CServer::detachNetwork()
    {
        if (d->parentNetwork)
        {
            CNetwork* net = nullptr;
            std::swap(net, d->parentNetwork);
            net->removeServer(this);
        }
    }

    /**
     * @brief Retrieves a list of connected client identifiers.
     * @return A list of client identifiers.
     */
    std::list<int> CServer::clients()
    {
        std::list<int> clientList;
        for (const auto& client : d->clients)
        {
            clientList.push_back(client.first);
        }
        return clientList;
    }

    /**
     * @brief Removes a client from the server by its identifier.
     * @param client The identifier of the client to remove.
     * @return true if the client was successfully removed, false otherwise.
     */
    bool CServer::removeClient(int client)
    {
        bool bResult = false;

        // Find the client in the map of clients and remove it if found
        auto it = d->clients.find(client);
        if (it != d->clients.end())
        {
            ConnectedClient* connectedClient = it->second;
            // Erase from the map BEFORE deleting: ~ConnectedClient() calls back into
            // m_server->removeClient(m_clientId) defensively. If the entry were still
            // in the map at that point, the reentrant call would find it again, delete
            // it a second time, and recurse forever (stack overflow/segfault). Erasing
            // first makes the reentrant lookup fail and turns it into a harmless no-op.
            d->clients.erase(it);
            delete connectedClient;
            clientDisconected(client);
            bResult = true;
        }

        // Also remove any received data associated with the client
        d->receivedData.erase(client);

        return bResult;
    }

    /**
     * @brief Removes all clients from the server.
     * @return true if all clients were successfully removed, false otherwise.
     */
    bool CServer::removeAllClients()
    {
        bool bResult = true;

        // Swap out the map before deleting anything, for the same reason as removeClient():
        // ~ConnectedClient() reenters CServer::removeClient(), which must find an empty/
        // not-containing map to become a no-op instead of double-deleting and corrupting
        // the map that this loop is iterating over.
        std::map<int, ConnectedClient*> clientsToDelete;
        clientsToDelete.swap(d->clients);

        for (auto& client : clientsToDelete)
        {
            clientDisconected(client.first);
            delete client.second;
        }

        return bResult;
    }

    /**
     * @brief Notifies that a client has disconnected.
     * @param clientId The identifier of the disconnected client.
     */
    void CServer::clientDisconected(int clientId)
    {
        for (const auto& callback : d->clientRemovedCallbacks)
        {
            callback(static_cast<uint64_t>(clientId));
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Callbacks handling
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Registers a read callback function for a specific data type.
     * @param type The type information of the data to read.
     * @param function The callback function to invoke when data of the specified type is received.
     * @return true if the callback was registered successfully, false otherwise.
     */
    bool CServer::registerReadCallback(uint64_t type, readFunc function)
    {
        if (d->readCallbackMap.find(type) == d->readCallbackMap.end() && function != nullptr)
        {
            d->readCallbackMap[type] = function;
            return true;
        }

        return false;
    }

    /**
     * @brief Registers a write callback function for a specific data type.
     * @param type The type information of the data to write.
     * @param function The callback function to invoke when data of the specified type is to be sent.
     * @return true if the callback was registered successfully, false otherwise.
     */
    bool CServer::registerWriteCallback(uint64_t type, writeFunc function)
    {
        if (d->writeCallbackMap.find(type) == d->writeCallbackMap.end() && function != nullptr)
        {
            d->writeCallbackMap[type] = function;
            return true;
        }

        return false;
    }

    /**
     * @brief Registers a callback function to be invoked when a client is removed.
     * @param function The callback function to invoke when a client is removed.
     */
    void CServer::registerRemovedClientCallback(clientRemouved function)
    {
        d->clientRemovedCallbacks.push_back(function);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Data reception and sending
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Handles received data from a client.
     * @param data Pointer to the received data.
     * @param size Size of the received data in bytes.
     * @param clientId The identifier of the client that sent the data.
     */
    void Network::CServer::addNewData(std::unique_ptr<uint8_t[]> data, size_t size, int clientId)
    {
        std::lock_guard<std::mutex> lock(d->dataMutex);
        d->receivedData[clientId].emplace_back(size, std::move(data));
        d->dataAvailable.notify_all();
    }

    /**
     * @brief Sends data to a connected client or broadcasts to all.
     * @param type The type information of the data to send.
     * @param data Pointer to the data to send.
     * @param clientId The identifier of the client to send to. If -1, sends to all clients.
     * @return The result of the send operation, or -1 if an error occurred.
     */
    int CServer::send(CSC::typeField type, void* data, int clientId)
    {
        int iResult = -1;

        writeFunc func = writeCallback(type);

        if (func != nullptr)
        {
            size_t dataSize = 0;
            std::unique_ptr<uint8_t[]> msgData;

            CSC::compactTypeAndData(func, data, clientId, dataSize, msgData, type);

            std::lock_guard<std::mutex> lock(d->clientsMutex);

            if (clientId == -1) /* Broadcast to all clients */
            {
                for (auto const& [id, clientPtr] : d->clients)
                {
                    iResult = clientPtr->sendMsg(msgData.get(), dataSize);
                }
            }
            else /* Targeted send to a specific client */
            {

                auto it = d->clients.find(clientId);

                if (it != d->clients.end())
                {
                    iResult = it->second->sendMsg(msgData.get(), dataSize);
                }
            }
        }

        return iResult;
    }

    /**
     * @brief Sends data to a specific list of connected clients.
     * @param type The type information of the data to send.
     * @param data Pointer to the data to send.
     * @param clientIds A list of client identifiers to send the data to. If empty, sends to all.
     * @return The result of the send operation, or -1 if an error occurred.
     */
    int CServer::send(common::typeField type, void* data, std::list<int> clientIds)
    {
        int iResult = -1;

        writeFunc func = writeCallback(type);

        if (func != nullptr)
        {
            std::lock_guard<std::mutex> lock(d->clientsMutex);

            if (clientIds.empty())
            {
                clientIds = clients();
            }

            if (!clientIds.empty())
            {
                size_t dataSize = 0;
                std::unique_ptr<uint8_t[]> msgData;

                CSC::compactTypeAndData(func, data, *clientIds.begin(), dataSize, msgData, type);

                for (auto clientId : clientIds)
                {
                    auto it = d->clients.find(clientId);
                    if (it != d->clients.end())
                    {
                        iResult = it->second->sendMsg(msgData.get(), dataSize);
                    }
                }
            }
        }

        return iResult;
    }




    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Loop for the server
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Runs the server, accepting and handling client connections.
     * @return true if the server is running successfully, false otherwise.
     */
    bool CServer::run()
    {
        bool bRes = false;

        d->bRun = true;

        if (initConnection())
        {
            d->threadLoopEntranceConnection = std::thread(&CServer::loopEntranceConnection, this);
            d->threadLoopDataFromClient = std::thread(&CServer::loopDataFromClient, this);

            bRes = d->threadLoopDataFromClient.get_id() != std::thread::id() && d->threadLoopEntranceConnection.get_id() != std::thread::id();
        }

        return bRes;
    }

    /**
     * @brief Stops the server and closes all client connections.
     */
    void CServer::stop()
    {
        d->bRun = false;
        d->dataAvailable.notify_all(); // triggers threadLoopDataFromClient in order let it exit
        // shutdown() reliably wakes a thread blocked in accept()/recv() on this socket from
        // another thread, unlike close() alone, which would make the join() below hang.
        shutdown(d->sock, SD_BOTH);
        closesocket(d->sock);
        if (d->threadLoopDataFromClient.joinable())
        {
            d->threadLoopDataFromClient.join();
        }
        removeAllClients();
        if(d->threadLoopEntranceConnection.joinable())
        {
            d->threadLoopEntranceConnection.join();
        }
    }

    /**
     * @brief Sets a custom protocol handler for the server.
     * @param handler Pointer to the ProtocolMsgHandler to set.
     * @detail if handler is nullptr, the default protocol handler will be used.
     * @note This function allows the user to specify a custom protocol handler for processing incoming messages.
     */
    void CServer::setCustomProtocolHandler(ProtocolMsgHandler* handler)
    {
        d->protocolHandler = handler;
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Private functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    /**
     * @brief Retrieves the registered read callback function for a specific data type.
     * @param type The type information of the data to read.
     * @return The registered read callback function, or nullptr if not found.
     */
    readFunc CServer::readCallback(uint64_t type)
    {
        readFunc funcRes = nullptr;

        if (d->readCallbackMap.find(type) != d->readCallbackMap.end())
        {
            funcRes = d->readCallbackMap[type];
        }

        return funcRes;
    }

    /**
     * @brief Retrieves the registered write callback function for a specific data type.
     * @param type The type information of the data to write.
     * @return The registered write callback function, or nullptr if not found.
     */
    writeFunc CServer::writeCallback(uint64_t type)
    {
        writeFunc funcRes = nullptr;

        if (d->writeCallbackMap.find(type) != d->writeCallbackMap.end())
        {
            funcRes = d->writeCallbackMap[type];
        }

        return funcRes;
    }

    /**
     * @brief Initializes the server connection.
     * @param afInetType The address family type (AF_INET for IPv4, AF_INET6 for IPv6).
     * @return true if the connection was successfully initialized, false otherwise.
     */
    bool CServer::initConnection(int afInetType)
    {
        bool bResult = false;

        struct sockaddr sa = { 0 };
        SOCKADDR_IN* sa4 = (SOCKADDR_IN*)&sa;
        SOCKADDR_IN6* sa6 = (SOCKADDR_IN6*)&sa;

        d->sock = socket(afInetType, SOCK_STREAM, 0);

        if (d->sock != INVALID_SOCKET)
        {
            if (afInetType == AF_INET6 || afInetType == AF_INET)
            {
                if (afInetType == AF_INET)
                {
                    sa4->sin_family = AF_INET;
                    sa4->sin_port = htons(d->port);
                    sa4->sin_addr.s_addr = htonl(INADDR_ANY); // Use any address
                    sa4->sin_zero[0] = 0; // Zero out the rest of the structure
                    sa4->sin_zero[1] = 0;
                }
                else if (afInetType == AF_INET6)
                {
                    sa6->sin6_family = AF_INET6;
                    sa6->sin6_port = htons(d->port);
                    sa6->sin6_flowinfo = 0;
                    sa6->sin6_addr = in6addr_any; // Use any address
                    sa6->sin6_scope_id = 0; // No specific scope
                }

                // Set the socket options to allow reuse of the address
                int opt = 1;
                if (setsockopt(d->sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) != SOCKET_ERROR)
                {
                    if (bind(d->sock, &sa, sizeof(sa)) != SOCKET_ERROR)
                    {
                        bResult = listen(d->sock, d->maxClientsCount) != SOCKET_ERROR;
                    }
                }
                else
                {
                    closesocket(d->sock);
                    d->sock = INVALID_SOCKET;
                }
            }
            else
            {
                // Unsupported address family
                closesocket(d->sock);
                d->sock = INVALID_SOCKET;
            }
        }

        return bResult;
    }



    /**
     * @brief Main loop for the server to accept and handle client connections.
     * @details This function continuously accepts incoming client connections.
     * @note This function runs indefinitely until the server is stopped.
     */
    void CServer::loopEntranceConnection()
    {
        while (d->bRun)
        {
            struct sockaddr_storage client_addr;
            socklen_t addr_size = sizeof client_addr;

            SOCKET client_socket = accept(d->sock, (struct sockaddr*)&client_addr, &addr_size);
            if (client_socket != INVALID_SOCKET)
            {
                ConnectedClient* clientNode = new ConnectedClient(this, client_socket, client_socket);
                d->clients[clientNode->clientId()] = clientNode;
                if(d->protocolHandler)
                {
                    clientNode->setCustomProtocolHandler(d->protocolHandler);
                }
                clientNode->run();
            }
            else
            {

                /*auto lastError = WSAGetLastError();
                char errorMsg[256];
                strerror_s(errorMsg, sizeof(errorMsg), lastError);
                fprintf(stderr, "accept failed: %d %s\n", lastError, errorMsg);
*/
                continue;
            }
        }
    }

    /**
     * @brief Processes data received from clients in a loop.
     * @details This function waits on a condition variable until data is available
     * from any client. It then iterates through the received data packets,
     * processing them, and clears the buffer for that client. The ownership of
     * the data buffer is swapped from the reception queue to a local variable.
     * @note This function runs indefinitely until the server is stopped.
     */
    void CServer::loopDataFromClient()
    {
        while (d->bRun)
        {
            std::unique_lock<std::mutex> lock(d->dataMutex);
            d->dataAvailable.wait(lock, [this] { return !d->receivedData.empty() || !d->bRun; });

            for (auto& client : d->receivedData)
            {
                int clientId = client.first;
                for (auto& dataPair : client.second)
                {
                    int size = dataPair.first;
                    std::unique_ptr<uint8_t[]> data = std::move(dataPair.second);

                    if (data)
                    {
                        uint8_t* raw_data = data.get();
                        auto typeId = CSC::extractTypeId(raw_data);

                        readFunc func = readCallback(typeId);
                        if (func != nullptr)
                        {
                            func(raw_data + CSC::typeIdSize, size - CSC::typeIdSize, clientId);
                        }
                        else
                        {
                            fprintf(stderr, "No read callback registered for type ID: %llu\n", typeId.typeId);// No callback registered for this type
                        }
                    }
                }
                d->receivedData[clientId].clear();
            }
        }
    }

    /**
     * @brief Computes a unique client identifier.
     * @details This function generates a new client ID by incrementing the last assigned ID.
     * If the ID exceeds the maximum value, it wraps around and finds the next available ID.
     * @return A unique client identifier.
     */
    int CServer::computeClientId()
    {
        int clientIdRes = d->nextClientId + 1;

        if (clientIdRes < 0)
        {
            clientIdRes = 1;

            while (d->clients.find(clientIdRes) != d->clients.end())
            {
                clientIdRes++;
            }
        }

        d->nextClientId = clientIdRes;

        return clientIdRes;
    }
} // namespace Network
