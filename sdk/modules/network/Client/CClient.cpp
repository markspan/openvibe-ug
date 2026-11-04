/**
* @file CClient.cpp
* @brief Implementation of the CClient class for managing client-side network operations.
* @details This file contains the implementation of the CClient class, which handles
* connecting to a server, data transmission, and callback management for a network client.
* It utilizes sockets for communication and supports custom protocol handling.
* The class is designed to be thread-safe and efficient in handling data reception and processing.
* @note This class is intended for use in a networked environment where a client needs to connect to a server,
* send data, and receive responses.
*/

#include <CClient.h>
#include <CNetwork.h>
#include <NAL.h>
#include <ClientServerCommon.h>
#include <ProtocolMsgHandler.h>


#include <string>
#include <map>
#include <memory>

#include <list>

#include <thread>
#include <condition_variable>
#include <mutex>
#include "ConnectedClient.h"

using namespace Network;
namespace CSC = common;
#define BUFFER_SIZE 10 * 1024 * 1024 // 10 MB

namespace Network
{
    /**
     * @internal
     * @brief Private implementation details for the CClient class.
     * @details This class encapsulates the private data and operations of the CClient class,
     * including connection management, callback registration, and data handling.
     */
    struct CClientPrivate
    {
        CNetwork* net = nullptr;
        std::string address;
        uint16_t port = 0;
        bool connected = false;
        SOCKET socket = INVALID_SOCKET;

        std::map<uint64_t, readFunc>  readCallbackMap;
        std::map<uint64_t, writeFunc> writeCallbackMap;

        char* buffer = new char[BUFFER_SIZE];
        bool  bDataReceiving = false;
        size_t expectedBodySize = 0;

        ProtocolMsgHandler* protocolHandler = nullptr;
        std::mutex receivedDataQueueMutex;
        std::list<std::pair<size_t, std::unique_ptr<uint8_t[]>>> receivedDataQueue;
        std::condition_variable dataAvailable;
        std::mutex dataMutex;
        std::mutex sendMutex;

        bool bRun = false;
        std::thread receivingDataThread;
        std::thread treatmentDataThread;
    };

    /**
     * @brief Constructs a CClient instance associated with the given CNetwork.
     * @param net Pointer to the CNetwork instance to associate with this client.
     */
    CClient::CClient(CNetwork* net)
    {
        d = new CClientPrivate;
        d->net = net;
    }

    /**
     * @brief Destroys the CClient instance and cleans up resources.
     */
    CClient::~CClient()
    {
        disconnect();
        detachNetwork();
        delete[] d->buffer;
        delete d->protocolHandler;
        delete d;
    }

    /**
     * @brief Sets the target address and port for the client.
     *
     * This method accepts an address string which can be an IPv4, IPv6, or hostname,
     * optionally followed by a port number. If a port number is provided in both the
     * address string and the separate port parameter, they must match.
     *
     * @param address The target address (IPv4, IPv6, or hostname), optionally with a port (e.g., "<address>:<port>")
     * @param port The target port (optional)
     * @return true if the address and port were successfully set, false otherwise
     */
    bool CClient::setAddress(const char* address, uint16_t port)
    {
        bool bRes = false;

        std::string tmpAddress;
        uint16_t tmpPort = 0;

        bRes = CNetwork::splitAddressAndPort(address, tmpAddress, tmpPort);
        bRes = bRes && port <= 65535; // Port is optional. If provided, it must be valid.
        if (bRes && port != 0)
        {
            if (tmpPort == 0 || tmpPort == port) // If port is provided in both address and parameter, they must match.
            {
                tmpPort = port;
            }
            else
            {
                bRes = false;
            }
        }

        if (bRes)
        {
            d->address = tmpAddress;
            d->port = tmpPort;
        }

        return bRes;
    }

    /**
     * @brief Connects the client to the specified address and port.
     * @return true if the connection was successfully established, false otherwise.
     * @details This method resolves the address, creates a socket, and attempts to connect to the server.
     * Starts data receiving and treatment threads upon successful connection.
     */
    bool CClient::connect()
    {
        bool bResult = false;

        if (d->address.empty() || d->port == 0)
        {
            fprintf(stderr, "Address or port not set\n");
        }
        else if (d->connected)
        {
            fprintf(stderr, "Already connected\n");
        }
        else
        {
            /* 1. Address resolution using the existing Network Abstraction */
            auto resolvedAddress = CNetwork::resolveAddress(d->address, d->port);

            if (resolvedAddress)
            {
                /* Retrieve the raw pointer from the smart pointer for system calls */
                struct addrinfo* addrinfoPointer = resolvedAddress.get();

                /* Loop through all resolved addresses until a connection succeeds */
                while (addrinfoPointer != nullptr && !bResult)
                {
                    /* 2. Socket creation */
                    d->socket = socket(addrinfoPointer->ai_family, addrinfoPointer->ai_socktype, addrinfoPointer->ai_protocol);

                    if (d->socket != INVALID_SOCKET)
                    {
                        /* 3. Connection attempt */
                        if (::connect(d->socket, addrinfoPointer->ai_addr, static_cast<int>(addrinfoPointer->ai_addrlen)) != SOCKET_ERROR)
                        {
                            /* Success: Connection established */
                            d->connected = true;
                            bResult = true;

                            /* 4. Apply socket options (Timeouts and TCP_NODELAY) */
                            if (ConfigureSocket(d->socket, 5) != 0)
                            {
                                fprintf(stderr, "Warning: Failed to set socket options for client.\n");
                            }

                            /* 5. Launch reception and treatment threads */
                            d->protocolHandler = new ProtocolMsgHandler();
                            d->bRun = true;
                            d->receivingDataThread = std::thread(&CClient::loopDataFromServer, this);
                            d->treatmentDataThread = std::thread(&CClient::loopDataTreatment,  this);
                        }
                        else
                        {
                            /* Connection failed for this address, clean up the socket before the next iteration */
                            closesocket(d->socket);
                            d->socket = INVALID_SOCKET;
                        }
                    }

                    /* If connection hasn't succeeded yet, move to the next address in the list */
                    if (bResult == false)
                    {
                        addrinfoPointer = addrinfoPointer->ai_next;
                    }
                }

                if (bResult == false)
                {
                    fprintf(stderr, "Could not create socket or connect to any resolved address.\n");
                }
            }
            else
            {
                fprintf(stderr, "Address resolution failed for %s:%d\n", d->address.c_str(), d->port);
            }
        }

        return bResult;
    }


    /**
     * @brief Disconnects the client from the server and cleans up resources.
     * @return true if the disconnection was successful, false otherwise.
     * @details This method stops the data receiving and treatment threads,
     * clears the received data queue, and releases any resources held by the client.
     */
    bool CClient::disconnect()
    {
        bool bRes = false;

        if (d->connected)
        {
            d->bRun = false;
            d->connected = false;

            if (d->socket != INVALID_SOCKET)
            {
                bRes = shutdown(d->socket, SD_BOTH) == 0;
                if (bRes) {
                    closesocket(d->socket);
                }
                d->socket = INVALID_SOCKET;
            }
            {
                std::lock_guard<std::mutex> lockDataQueue(d->dataMutex);
                std::lock_guard<std::mutex> lockCV(d->receivedDataQueueMutex);
                d->receivedDataQueue.clear();
                d->dataAvailable.notify_all();
            }
            if (d->receivingDataThread.joinable())
            {
                d->receivingDataThread.join();
            }
            if (d->treatmentDataThread.joinable())
            {
                d->treatmentDataThread.join();
            }
        }

        return bRes;
    }

    CNetwork* CClient::getNetwork() const
    {
        return d->net;
    }

    void CClient::detachNetwork()
    {
        if (d->net)
        {
            CNetwork* net = nullptr;
            std::swap(net, d->net);
            net->removeClient(this);
        }
    }


    /**
     * @brief Checks if the client state is currently connected to the server.
     * @return true if state connected, false otherwise.
     */
    bool CClient::isConnected() const
    {
        return d->connected;
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
    bool CClient::registerReadCallback(uint64_t type, readFunc function)
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
    bool CClient::registerWriteCallback(uint64_t type, writeFunc function)
    {
        if (d->writeCallbackMap.find(type) == d->writeCallbackMap.end() && function != nullptr)
        {
            d->writeCallbackMap[type] = function;
            return true;
        }

        return false;
    }

    /**
     * @brief Retrieves the registered read callback function for a specific data type.
     * @param type The type information of the data to read.
     * @return The registered read callback function, or nullptr if not found.
     */
    readFunc  CClient::readCallback(uint64_t type)
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
    writeFunc CClient::writeCallback(uint64_t type)
    {
        writeFunc funcRes = nullptr;

        if (d->writeCallbackMap.find(type) != d->writeCallbackMap.end())
        {
            funcRes = d->writeCallbackMap[type];
        }

        return funcRes;
    }





    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Data reception and sending with their loops
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Sends a message to the server.
     * @param type The type information of the data to send.
     * @param data Pointer to the data to send.
     * @return The number of bytes sent, or -1 if an error occurred.
     */
    int CClient::send(CSC::typeField type, void* data)
    {
        int iResult = -1;

        if (d->connected)
        {
            writeFunc func = writeCallback(type);
            if (func != nullptr)
            {
                size_t dataSize = 0;
                std::unique_ptr<uint8_t[]> msgData;
                auto remoteId = getId();

                compactTypeAndData(func, data, remoteId, dataSize, msgData, type);

                if (d->protocolHandler->sendPacket(msgData.get(), dataSize))
                {
                    int size = 0;
                    std::unique_ptr<uint8_t[]> data;

                    d->protocolHandler->sendingData(size, data);
                    if (size > 0 && data)
                    {
                        int iSent = ::send(d->socket, (const char*)data.get(), size, 0);
                        if (iSent == SOCKET_ERROR)
                        {
                            fprintf(stderr, "send failed\n");
                        }
                        iResult = iSent;
                    }
                }
            }
        }

        return iResult;
    }



    /**
     * @brief Loops and processes incoming data from the server.
     */
    void CClient::loopDataFromServer()
    {
        int iCount = 0;

        while (d->bRun)
        {
            iCount = recv(d->socket, d->buffer, BUFFER_SIZE, 0);

            if (iCount > 0)
            {
                if (d->protocolHandler->receive((uint8_t*)d->buffer, iCount))
                {
                    int size = 0;
                    std::unique_ptr<uint8_t[]> data;

                    while (d->protocolHandler->receivedData(size, data))
                    {
                        if (size > 0 && data)
                        {
                            std::lock_guard<std::mutex> lockCV(d->dataMutex);
                            std::lock_guard<std::mutex> lockDataQueue(d->receivedDataQueueMutex);
                            d->receivedDataQueue.push_back({ size, std::move(data) });
                        }
                    }
                    if(!d->receivedDataQueue.empty())
                    {
                        d->dataAvailable.notify_all();
                    }
                }
                else
                {
                    //fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
                }
            }
            else if (iCount == 0)
            {
                break; // Client close connection
            }
            else if (!NETWORK_ERROR_TIMEOUT)
            {
                // UG fork: a timeout only means the server had nothing to send, e.g. it isn't playing yet. Upstream closes
                // the connection then, so a scenario started before the server stayed without a signal, with no error.
                //fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
                break;
            }
        }

        closesocket(d->socket);
    }

    /**
     * @brief Processes received data in a separate thread.
     *
     * This function waits for data to be available in the received data queue,
     * then processes each data packet by extracting its type ID and invoking
     * the corresponding registered read callback function.
     */
    void CClient::loopDataTreatment()
    {
        while (d->bRun)
        {
            std::unique_lock<std::mutex> lock(d->dataMutex);
            d->dataAvailable.wait(lock, [this] { return !d->receivedDataQueue.empty() || !d->bRun; });

            while (!d->receivedDataQueue.empty())
            {
                std::pair < size_t, std::unique_ptr<uint8_t[]> > data;
                {
                    std::lock_guard<std::mutex> lock(d->receivedDataQueueMutex);
                    data = std::move(d->receivedDataQueue.front());
                    d->receivedDataQueue.pop_front();
                }

                uint8_t* raw_data = data.second.get();
                size_t dataSize = data.first;
                auto typeId = CSC::extractTypeId(raw_data);
                readFunc func = readCallback(typeId.typeId);
                if (func != nullptr)
                {
                    func(raw_data + CSC::typeIdSize, dataSize - CSC::typeIdSize, getId());
                }
                else
                {
                    fprintf(stderr, "No read callback registered for type ID: %llu\n", typeId.typeId);// No callback registered for this type
                }
            }
        }
    }
} // namespace Network
