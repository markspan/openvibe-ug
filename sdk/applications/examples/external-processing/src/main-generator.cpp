#include <csignal>
#include <cstring>

#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>
#include <map>

#include <ebml/IWriterHelper.h>
#include <ebml/IWriter.h>
#include <ebml/TWriterCallbackProxy.h>
#include <communication/ovCMessagingClient.h>

#include <toolkit/ovtk_defines.h>
#include <openvibe/CMemoryBuffer.hpp>
#include <openvibe/CTime.hpp>

static bool didRequestForcedQuit = false;
static void signalHandler(int /* signal */) { didRequestForcedQuit = true; }

class EBMLWriterCallback
{
public:
    void write(const void* buffer, const size_t size)
    {
        const uint8_t* data = static_cast<const uint8_t*>(buffer);
        m_buffer.insert(m_buffer.end(), data, data + size);
    }

    void clear() { m_buffer.clear(); }

    const std::vector<uint8_t>& data() const { return m_buffer; }

private:
    std::vector<uint8_t> m_buffer;
};

int main(const int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);

    std::string connectionID;
    size_t port = 49687;

    for (int i = 0; i < argc; ++i) {
        if (std::strcmp(argv[i], "--connection-id") == 0) { if (argc > i + 1) { connectionID = argv[i + 1]; } }
        else if (std::strcmp(argv[i], "--port") == 0) { if (argc > i + 1) { port = size_t(std::stoi(argv[i + 1])); } }
    }

    // EBML

    EBMLWriterCallback callback;
    EBML::TWriterCallbackProxy1<EBMLWriterCallback> callbackProxy(callback, &EBMLWriterCallback::write);
    EBML::IWriter* writer       = createWriter(callbackProxy);
    EBML::IWriterHelper* helper = EBML::createWriterHelper();
    helper->connect(writer);

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

    if (client.getInputCount() > 0) {
        std::cerr << "The test generator can not take any inputs, was given " << client.getInputCount() << std::endl;
        client.close();
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < client.getOutputCount(); ++i) {
        uint64_t id;
        uint64_t type;
        std::string name;

        if (client.getOutput(i, id, type, name)) { std::cout << "Output:\n\tIndex: " << id << "\n\tType: " << type << "\n\tName: " << name << "\n\n"; }
    }

    std::map<std::string, std::string> parameters;

    for (size_t i = 0; i < client.getParameterCount(); ++i) {
        uint64_t id;
        uint64_t type;
        std::string name;
        std::string value;

        client.getParameter(i, id, type, name, value);
        parameters[name] = value;
    }

    if (!(parameters.count("Channel Count"))) {
        std::cerr << "Missing parameter" << std::endl;
        client.close();
        exit(EXIT_FAILURE);
    }

    const size_t nChannel         = size_t(std::stoul(parameters.at("Channel Count")));
    const size_t samplingRate     = size_t(std::stoul(parameters.at("Sampling Rate")));
    const size_t samplesPerBuffer = size_t(std::stoul(parameters.at("Samples Per Buffer")));
    const size_t samplesToSend    = size_t(std::stoul(parameters.at("Amount of Samples to Generate")));

    std::vector<double> matrix;
    matrix.resize(nChannel * samplesPerBuffer);

    // Announce to server that the box has finished initializing and wait for acknowledgement
    while (!client.waitForSyncMessage()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    client.pushLog(Communication::LogLevel_Info, "Received Ping");

    client.pushSync();
    client.pushLog(Communication::LogLevel_Info, "Sent Pong");

    // Process

    // Send the header
    callback.clear();
    helper->openChild(OVTK_NodeId_Header);
    {
        helper->openChild(OVTK_NodeId_Header_StreamType);
        {
            helper->setUInt(0);
            helper->closeChild();
        }
        helper->openChild(OVTK_NodeId_Header_StreamVersion);
        {
            helper->setUInt(0);
            helper->closeChild();
        }

        helper->openChild(OVTK_NodeId_Header_Signal);
        {
            helper->openChild(OVTK_NodeId_Header_Signal_Sampling);
            {
                helper->setUInt(samplingRate);
                helper->closeChild();
            }
            helper->closeChild();
        }

        helper->openChild(OVTK_NodeId_Header_StreamedMatrix);
        {
            helper->openChild(OVTK_NodeId_Header_StreamedMatrix_DimensionCount);
            {
                helper->setUInt(2);
                helper->closeChild();
            }
            helper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
            {
                helper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
                {
                    helper->setUInt(nChannel);
                    helper->closeChild();
                }
                helper->closeChild();
            }
            helper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
            {
                helper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
                {
                    helper->setUInt(samplesPerBuffer);
                    helper->closeChild();
                }
                helper->closeChild();
            }
            helper->closeChild();
        }
        helper->closeChild();
    }
    if (!client.pushEBML(0, 0, 0, std::make_shared<const std::vector<uint8_t>>(callback.data()))) {
        std::cerr << "Failed to push EBML.\n";
        std::cerr << "Error " << client.getLastError() << "\n";
        exit(EXIT_FAILURE);
    }

    client.pushSync();

    size_t sentSamples = 0;
    while (!didRequestForcedQuit || (samplesToSend != 0 && sentSamples < samplesToSend)) {
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

        // Send data

        while (!client.waitForSyncMessage()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

        const uint64_t expectedSamples = OpenViBE::CTime(client.getTime()).toSampleCount(samplingRate);

        while (sentSamples < expectedSamples && (samplesToSend == 0 || sentSamples < samplesToSend)) {
            for (size_t channel = 0; channel < nChannel; ++channel) {
                for (size_t sample = 0; sample < samplesPerBuffer; ++sample) {
                    matrix[channel * samplesPerBuffer + sample] = sin((sentSamples + sample) / double(samplingRate));
                }
            }

            callback.clear();
            helper->openChild(OVTK_NodeId_Buffer);
            {
                helper->openChild(OVTK_NodeId_Buffer_StreamedMatrix);
                {
                    helper->openChild(OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer);
                    {
                        helper->setBinary(matrix.data(), matrix.size() * sizeof(double));
                        helper->closeChild();
                    }
                    helper->closeChild();
                }
                helper->closeChild();
            }


            const uint64_t tStart = OpenViBE::CTime(samplingRate, sentSamples).time();
            const uint64_t tEnd   = OpenViBE::CTime(samplingRate, sentSamples + samplesPerBuffer).time();

            if (!client.pushEBML(0, tStart, tEnd, std::make_shared<const std::vector<uint8_t>>(callback.data()))) {
                std::cerr << "Failed to push EBML.\n";
                std::cerr << "Error " << client.getLastError() << "\n";
                exit(EXIT_FAILURE);
            }

            sentSamples += samplesPerBuffer;
        }

        // Errors
        uint64_t packetId;
        Communication::EError error;
        uint64_t guiltyId;

        while (client.popError(packetId, error, guiltyId)) { std::cerr << "Error received:\n\tError: " << int(error) << "\n\tGuilty Id: " << guiltyId << "\n"; }
        // Here, we send a sync message to tell to the server that we have no more data to send and we can move forward.
        if (!client.pushSync()) { exit(EXIT_FAILURE); }
    }

    std::cout << "Processing stopped.\n";

    helper->disconnect();
    helper->release();
    writer->release();

    if (!client.close()) { std::cerr << "Failed to close the connection\n"; }

    return 0;
}
