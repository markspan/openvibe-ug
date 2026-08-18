///-------------------------------------------------------------------------------------------------
/// 
/// \file uoSocketClientServerBaseTest.cpp
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

#include "socket/IConnection.h"
#include "socket/IConnectionClient.h"
#include "socket/IConnectionServer.h"

#include "ovtAssert.h"

int uoSocketClientServerBaseTest(int argc, char* argv[])
{
    OVT_ASSERT(argc == 3, "Failure to retrieve tests arguments. Expecting: server_name port_number");

    const std::string name = argv[1];
    char* end;
    const size_t port = strtol(argv[2], &end, 10);

    // basic tests on server and clients

    Socket::IConnectionServer* server = Socket::createConnectionServer();
    Socket::IConnectionClient* client = Socket::createConnectionClient();

    OVT_ASSERT(!server->isConnected(), "Failure to check for connection state before connection happens");
    OVT_ASSERT(server->listen(port) && server->isConnected(), "Failure to make socket listening for input connections");
    OVT_ASSERT(!server->listen(port), "Failure to generate connection error if the socket is already connected");
    OVT_ASSERT(!server->isReadyToReceive(), "Failure to check for readyness to receive when no client is connected");
    OVT_ASSERT(server->close() && !server->isConnected(), "Failure to close connection");
    OVT_ASSERT(!client->isConnected(), "Failure to check for connection state before connection happens");
    OVT_ASSERT(!client->connect(name.c_str(), port) && !client->isConnected(), "Failure to generate connection error due to no server currently running");
    OVT_ASSERT(server->listen(port), "Failure to make socket listening for input connections after a disconnection");
    OVT_ASSERT(!client->connect("bad_server_name", port) && !client->isConnected(), "Failure to generate connection error caused by wrong server name");
    OVT_ASSERT(!client->connect(name.c_str(), 0) && !client->isConnected(), "Failure to generate connection error caused by wrong port number");
    OVT_ASSERT(client->connect(name.c_str(), port) && client->isConnected(), "Failure to connect to server");
    OVT_ASSERT(client->close() && !client->isConnected(), "Failure to disconnect");

    // Test method getSocketPort

    size_t guessedPort;
    OVT_ASSERT(server->getSocketPort(guessedPort), "Failure to get socket informations");
    OVT_ASSERT(guessedPort == port, "Get Socket information should return server port.");
    OVT_ASSERT(client->connect(name.c_str(), guessedPort) && client->isConnected(), "Failure to connect to server");
    OVT_ASSERT(client->close() && !client->isConnected(), "Failure to disconnect");
    OVT_ASSERT(server->close() && !server->isConnected(), "Failure to close connection");

    // Test to connect using port 0

    OVT_ASSERT(server->listen(0) && server->isConnected(), "Failure to make socket listening for input connections");
    OVT_ASSERT(server->getSocketPort(guessedPort), "Failure to get socket informations");
    OVT_ASSERT(client->connect(name.c_str(), guessedPort) && client->isConnected(), "Failure to connect to server");
    OVT_ASSERT(client->close() && !client->isConnected(), "Failure to disconnect");
    OVT_ASSERT(server->close() && !server->isConnected(), "Failure to close connection");

    // Release ressources

    server->release();
    client->release();

    return EXIT_SUCCESS;
}
