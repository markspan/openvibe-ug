///-------------------------------------------------------------------------------------------------
/// 
/// \file uoSocketClientServerSyncCommunicationTest.cpp
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

#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <vector>

#include "socket/IConnection.h"
#include "socket/IConnectionClient.h"
#include "socket/IConnectionServer.h"

#include "ovtAssert.h"

namespace {
std::condition_variable gServerStartedCondVar;
std::mutex gServerStartedMutex;
std::condition_variable gClientConnectedCondVar;
std::mutex gClientConnectedMutex;
std::vector<std::string> gReceivedData;
bool gServerStarted   = false;
bool gClientConnected = false;

// server callback run from a child thread
void onServerListening(const int port, const size_t packetCount)
{
    // only the server side modifies gReceivedData thus no need to handle race condition
    gReceivedData.clear();

    Socket::IConnection* clientConnection = nullptr;

    // create server
    Socket::IConnectionServer* server = Socket::createConnectionServer();
    server->listen(port);

    // keep the scope braces here, as it ensures mutex is released
    {
        std::lock_guard<std::mutex> lockOnServerStart(gServerStartedMutex);
        gServerStarted = true;
    }

    gServerStartedCondVar.notify_one();

    // connect clients
    while (!clientConnection) { if (server->isReadyToReceive()) { clientConnection = server->accept(); } }

    // keep the scope braces here, as it ensures mutex is released
    {
        std::lock_guard<std::mutex> lockOnClientConnected(gClientConnectedMutex);
        gClientConnected = true;
    }

    gClientConnectedCondVar.notify_one();

    while (gReceivedData.size() < packetCount) {
        if (clientConnection->isReadyToReceive()) {
            size_t dataSize = 0;
            char dataBuffer[64];
            clientConnection->receiveBufferBlocking(&dataSize, sizeof(dataSize));
            clientConnection->receiveBufferBlocking(dataBuffer, dataSize);
            gReceivedData.push_back(std::string(dataBuffer, dataSize));
        }
    }

    server->release();
}
}  // namespace

int uoSocketClientServerSyncCommunicationTest(int argc, char* argv[])
{
    OVT_ASSERT(argc == 4, "Failure to retrieve tests arguments. Expecting: server_name port_number packet_count");

    const std::string serverName = argv[1];
    char* end;
    const size_t port  = strtol(argv[2], &end, 10);
    size_t packetCount = strtol(argv[3], &end, 10);

    // test synchronous data transmission from a single client to server:
    // - launch a server on a background thread
    // - connect single client
    // - make client sending data
    // - make server receiving and storing data
    // - join the thread and do assertions on received data in the main thread

    // create a client
    Socket::IConnectionClient* client = Socket::createConnectionClient();

    // launch server on background thread
    std::thread serverThread(onServerListening, port, packetCount);

    // wait until the server is started to connect client
    std::unique_lock<std::mutex> lockOnServerStart(gServerStartedMutex);
    gServerStartedCondVar.wait(lockOnServerStart, []() { return gServerStarted; });


    client->connect(serverName.c_str(), port);

    // wait until the connection is made to transmit data
    std::unique_lock<std::mutex> lockOnClientConnected(gClientConnectedMutex);
    gClientConnectedCondVar.wait(lockOnClientConnected, []() { return gClientConnected; });

    // transmit data
    // transmission follows the protocol: data size transmission + data transmission
    const std::string baseData = "Data packet index: ";

    for (size_t sendIndex = 0; sendIndex < packetCount; ++sendIndex) {
        std::string dataString = baseData + std::to_string(sendIndex);
        size_t dataSize        = dataString.size();

        client->sendBufferBlocking(&dataSize, sizeof(dataSize));
        client->sendBufferBlocking(dataString.c_str(), dataSize);
    }

    serverThread.join(); // wait until the end of the thread

    // release resources
    client->close();
    client->release();

    // do the assertion on the main thread
    OVT_ASSERT(gReceivedData.size() == packetCount, "Failure to retrieve packet count");

    for (size_t receivedIndex = 0; receivedIndex < packetCount; ++receivedIndex) {
        OVT_ASSERT_STREQ(gReceivedData[receivedIndex], (baseData + std::to_string(receivedIndex)), "Failure to retrieve packet");
    }

    return EXIT_SUCCESS;
}
