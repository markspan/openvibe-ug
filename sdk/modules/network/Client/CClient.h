#pragma once

/**
* @file CClient.h
* @brief Header file for the CClient class.
* @details This file contains the declaration of the CClient class, which provides methods to connect to a server,
* send and receive messages, and manage the connection state.
* The class is designed to be used in a networked environment where a client needs to connect to a server,
* send data, and receive responses.
* It supports custom read and write callbacks for handling specific data types.
* The class is intended to be used as part of a larger networking library.
* @note This class is not thread-safe and should be used from a single thread.
* @see CNetwork
* @see ProtocolMsgHandler
* @author Florent LERAY
* @date 2025-10-04
*/

#include <cstdint>
#include <memory>
#include <utility>

#include <ClientServerCommon.h>

namespace Network
{
    class CNetwork;
    class CClientPrivate;

    /**
     * @class CClient
     * @brief CClient class for handling network communication.
     * @details This class provides methods to connect to a server, send and receive messages, and manage the connection state.
     * @note This class is not thread-safe and should be used from a single thread.
     */
    class CClient
    {
    public:
        virtual ~CClient();

        bool setAddress(const char* address, uint16_t port = 0);

        bool connect();
        bool disconnect();

        CNetwork* getNetwork() const;
        void detachNetwork();

        bool isConnected() const;
        uint64_t getId() { return reinterpret_cast<uint64_t>(this); }

        template <typename T> bool registerReadCallback(readFunc  function)  { return registerReadCallback  (common::computTypeId(typeid(T).name()), function); }
        template <typename T> bool registerWriteCallback(writeFunc function) { return registerWriteCallback (common::computTypeId(typeid(T).name()), function); }

        template <typename T> int send(T* data) { return send(common::computTypeId(typeid(*data).name()), data); }

    protected:
        CClient(CNetwork* net);
        int send(common::typeField type, void* data);

    private:
        bool registerReadCallback(uint64_t type, readFunc function);
        bool registerWriteCallback(uint64_t type, writeFunc function);

        readFunc  readCallback(uint64_t type);
        writeFunc writeCallback(uint64_t type);

        void loopDataFromServer();
        void loopDataTreatment();

        // Disable copy constructor, assignment operator and default constructor
        CClient() = delete;
        CClient(const CClient&) = delete;
        CClient& operator=(const CClient&) = delete;
        CClient(CClient&&) = delete;
        CClient& operator=(CClient&&) = delete;

    private:
        CClientPrivate* d;
    };

} // namespace Network
