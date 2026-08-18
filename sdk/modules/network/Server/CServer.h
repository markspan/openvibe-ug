#pragma once
/**
* @file CServer.h
* @brief Declaration of the CServer class for managing server functionalities.
* @details This class provides methods to read and write data, manage clients,
* and register callbacks for reading and writing operations. It encapsulates the server's
* private data and operations.
* @note This class is designed to be used in a networked environment where clients can connect to the server,
* @author Florent LERAY
* @date 2025-10-04
*/

#include <list>
#include <memory>

#include <ClientServerCommon.h>
#include <NAL.h>


#define MAX_CLIENTS 100 /// Maximum number of clients handled by a server instance

/**
* @brief Client removed callback function type.
* @details This function is called when a client is removed from the server.
* The function takes the client ID as a parameter.
* The uint64_t parameter is the client ID.
*/
using clientRemouved = void (*) (uint64_t); // Client removed callback function type. The uint64_t parameter is the client ID.

namespace Network
{
    class CNetwork;
    class ConnectedClient;
    class ProtocolMsgHandler;
    class CServerPrivate;

    /**
    * @class CServer
    * @brief CServer is a class that provides server functionalities, allowing to read and write data,
    * manage clients, and register callbacks for reading and writing operations.
    * It encapsulates the server's private data and operations.
    * It provides methods to set the port and interface, read data, write data of specific types,
    * manage client connections, and register callbacks for read and write operations.
    *
    * The class is designed to be used in a networked environment where clients can connect to the server,
    * send data, and receive responses. It supports multiple clients and allows for custom handling.
    */
    class CServer
    {
    public:
        virtual ~CServer();

        bool setPort(uint16_t port);
        //bool setInterface(); //TODO

        CNetwork* getNetwork() const;
        void detachNetwork();

        std::list<int> clients();
        bool removeClient(int client);
        bool removeAllClients();
        void clientDisconected(int clientId);

        template <typename T> bool registerReadCallback(readFunc  function)  { return registerReadCallback(common::computTypeId (typeid(T).name()), function); }
        template <typename T> bool registerWriteCallback(writeFunc function) { return registerWriteCallback(common::computTypeId(typeid(T).name()), function); }


        void registerRemovedClientCallback(clientRemouved function);

        void addNewData(std::unique_ptr<uint8_t[]> data, size_t size, int clientId = -1);

        template <typename T> int send(T* data, int clientId = -1) { return send(common::computTypeId(typeid(*data).name()), data, clientId); }
        template <typename T> int send(T* data, std::list<int> clientIds = std::list<int>()) { return send(common::computTypeId(typeid(*data).name()), data, clientIds); }

        bool run();
        void stop();

        void setCustomProtocolHandler(ProtocolMsgHandler* handler);
    
    protected:
        CServer(CNetwork* network);

    private:
        bool registerReadCallback(uint64_t type, readFunc function);
        bool registerWriteCallback(uint64_t type, writeFunc function);

        int send(common::typeField type, void* data, int clientId = -1);
        int send(common::typeField type, void* data, std::list<int> clientIds = std::list<int>());

        readFunc  readCallback(uint64_t type);
        writeFunc writeCallback(uint64_t type);

        bool initConnection(int afInetType = AF_INET);
        void loopEntranceConnection();
        void loopDataFromClient();

        int computeClientId();

        // Disable copy constructor, assignment operator and default constructor
        CServer() = delete;
        CServer(const CServer&) = delete;
        CServer& operator=(const CServer&) = delete;
        CServer(const CServer&&) = delete;
        CServer& operator=(const CServer&&) = delete;

    private:
        CServerPrivate* d = nullptr;
    };
} // namespace Network
