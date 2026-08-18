///-------------------------------------------------------------------------------------------------
/// 
/// \file uoSocketClientServerASyncCommunicationTest.cpp
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
#include <condition_variable>
#include <sstream>
#include <vector>

#include "socket/IConnection.h"
#include "socket/IConnectionClient.h"
#include "socket/IConnectionServer.h"

#include "ovtAssert.h"

namespace {
std::condition_variable gServerStartedCondVar;
std::mutex gServerStartedMutex;
std::vector<std::string> gReceivedData;
bool gServerStarted = false;

// server callback run from a child thread
void onServerListening(const int port, const size_t expectedPacketCount)
{
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

    // notify main thread that the server is created so that it can connect a single client
    gServerStartedCondVar.notify_one();

    // loop until all packet are received
    while (gReceivedData.size() < expectedPacketCount) {
        if (server->isReadyToReceive()) { clientConnection = server->accept(); }

        if (clientConnection && clientConnection->isReadyToReceive()) {
            size_t dataSize       = 0;
            size_t bytesToReceive = sizeof(dataSize);
            size_t bytesReceived  = 0;
            char dataBuffer[32];

            // first receive data size
            while (bytesReceived < bytesToReceive) { bytesReceived += clientConnection->receiveBuffer(&dataSize, bytesToReceive - bytesReceived); }

            // then receive data
            bytesToReceive = dataSize;
            bytesReceived  = 0;

            while (bytesReceived < bytesToReceive) { bytesReceived += clientConnection->receiveBuffer(dataBuffer, bytesToReceive - bytesReceived); }

            gReceivedData.push_back(std::string(dataBuffer, dataSize));
        }
    }

    server->release();
}

void sendData(Socket::IConnectionClient* client, const void* data, const size_t size)
{
    const size_t bytesToSend = size;
    size_t bytesSent         = 0;

    while (bytesSent < bytesToSend) { bytesSent += client->sendBuffer(data, bytesToSend - bytesSent); }
}
}   // namespace 

int uoSocketClientServerASyncCommunicationTest(int argc, char* argv[])
{
    OVT_ASSERT(argc == 4, "Failure to retrieve tests arguments. Expecting: server_name port_number packet_count");

    const std::string serverName = argv[1];
    std::stringstream ssPort(argv[2]);
    std::stringstream ssPacket(argv[3]);

    int portNumber;
    ssPort >> portNumber;

    size_t packetCount;
    ssPacket >> packetCount;

    // test asynchronous data transmission from a single client to server:
    // - launch a server on a background thread
    // - connect a single client
    // - make client sending data
    // - marke server receiving and storing data
    // - join the thread and do the assertions on received data when no data race is possible

    // create client
    Socket::IConnectionClient* client = Socket::createConnectionClient();

    // launch server on background thread
    std::thread serverThread(onServerListening, portNumber, packetCount);

    // wait until the server is started to connect clients
    std::unique_lock<std::mutex> lock(gServerStartedMutex);
    gServerStartedCondVar.wait(lock, []() { return gServerStarted; });

    client->connect(serverName.c_str(), portNumber);

    // transmit data
    // transmission follows the protocol: data size transmission + data transmission
    const std::string baseData = "Data packet index: ";

    for (size_t sendIndex = 0; sendIndex < packetCount; ++sendIndex) {
        std::string tmp = baseData + std::to_string(sendIndex);
        size_t size     = tmp.size();
        sendData(client, &size, sizeof(size));
        sendData(client, const_cast<char*>(tmp.c_str()), size);
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
