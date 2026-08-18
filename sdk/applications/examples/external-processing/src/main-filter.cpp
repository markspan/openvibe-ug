#include <csignal>
#include <cstring>

#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <thread>

#include <communication/ovCMessagingClient.h>

static bool didRequestForcedQuit = false;
static void signalHandler(int /* signal */) { didRequestForcedQuit = true; }

int main(const int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);

    std::string connectionID;
    size_t port = 49687;

    for (int i = 0; i < argc; ++i) {
        if (std::strcmp(argv[i], "--connection-id") == 0) { if (argc > i + 1) { connectionID = argv[i + 1]; } }
        else if (std::strcmp(argv[i], "--port") == 0) { if (argc > i + 1) { port = size_t(std::stoi(argv[i + 1])); } }
    }
    didRequestForcedQuit = false;

    Communication::MessagingClient client;

    client.setConnectionID(connectionID);

    while (!client.connect("127.0.0.1", port)) {
        const Communication::MessagingClient::ELibraryError error = client.getLastError();

        if (error == Communication::MessagingClient::ELibraryError::Socket_FailedToConnect) {
            std::cout << "Server not responding\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        else {
            std::cout << "Error " << error << std::endl;
            exit(EXIT_FAILURE);
        }

        if (didRequestForcedQuit) { exit(EXIT_SUCCESS); }
    }

    std::cout << "Connected to server\n";

    // Initialize

    for (size_t i = 0; i < client.getInputCount(); ++i) {
        uint64_t id;
        uint64_t type;
        std::string name;

        if (client.getInput(i, id, type, name)) { std::cout << "Input:\n\tIndex: " << id << "\n\tType: " << type << "\n\tName: " << name << "\n\n"; }
    }

    for (size_t i = 0; i < client.getOutputCount(); ++i) {
        uint64_t id;
        uint64_t type;
        std::string name;

        if (client.getOutput(i, id, type, name)) { std::cout << "Output:\n\tIndex: " << id << "\n\tType: " << type << "\n\tName: " << name << "\n\n"; }
    }

    for (size_t i = 0; i < client.getParameterCount(); ++i) {
        uint64_t id;
        uint64_t type;
        std::string name;
        std::string value;

        if (client.getParameter(i, id, type, name, value)) {
            std::cout << "Parameter:\n\tIndex: " << id << "\n\tType: " << type << "\n\tName: " << name << "\n\tValue: " << value << "\n\n";
        }
    }

    // Announce to server that the box has finished initializing and wait for acknowledgement
    while (!client.waitForSyncMessage()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    client.pushLog(Communication::LogLevel_Info, "Received Ping");

    client.pushSync();
    client.pushLog(Communication::LogLevel_Info, "Sent Pong");

    // Process

    while (!didRequestForcedQuit) {
        if (client.isEndReceived()) {
            std::cout << "End message received!\n";
            break;
        }

        if (!client.isConnected()) {
            std::cout << "Disconnected!\n";
            break;
        }

        if (client.isInErrorState()) {
            std::cerr << "Error state " << client.getLastError() << "\n";
            break;
        }

        // EBML

        uint64_t packetId;
        size_t index;
        uint64_t startTime;
        uint64_t endtime;
        std::shared_ptr<const std::vector<uint8_t>> ebml;

        // We wait for a synchronization message, this means that the client box has finished sending
        // all of the data it has received during one process() method call.
        while (!client.waitForSyncMessage()) {
            while (client.popEBML(packetId, index, startTime, endtime, ebml)) {
                // We just push out the received EBML as is
                if (!client.pushEBML(index, startTime, endtime, ebml)) {
                    std::cerr << "Failed to push EBML.\n";
                    std::cerr << "Error " << client.getLastError() << "\n";
                    break;
                }

                /*if (!client.pushLog(Communication::ELogLevel::LogLevel_Info, "EBML received on index: " + std::to_string(index)))
                {
                    std::cout << "Failed to push log.\n";
                    client.close();
                    exit(EXIT_FAILURE);
                }*/
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Sync message was received, this means that we are now sure that the buffer will not receive
        // any additional data as the box is blocked until we acknowledge sending all of the processed
        // data back
        while (client.popEBML(packetId, index, startTime, endtime, ebml)) {
            if (!client.pushEBML(index, startTime, endtime, ebml)) {
                std::cerr << "Failed to push EBML.\n";
                std::cerr << "Error " << client.getLastError() << "\n";
                break;
            }

            /*if (!client.pushLog(Communication::ELogLevel::LogLevel_Info, "EBML received on index: " + std::to_string(index)))
            {
            std::cout << "Failed to push log.\n";
            client.close();
            exit(EXIT_FAILURE);
            }*/
        }

        // Errors

        Communication::EError error;
        uint64_t guiltyId;

        while (client.popError(packetId, error, guiltyId)) {
            std::cerr << "Error received:\n";
            std::cerr << "\tError: " << int(error) << "\n";
            std::cerr << "\tGuilty Id: " << guiltyId << "\n";
        }

        // Here, we send a sync message to tell to the server that we have no more
        // data to send and we can move forward. This will unblock the box.
        if (!client.pushSync()) { return 0; }
    }

    std::cout << "Processing stopped.\n";


    if (!client.close()) { std::cerr << "Failed to close the connection\n"; }

    return 0;
}
