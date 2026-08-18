#include "ovasCDriverEGIAmpServer.h"
#include "ovasCConfigurationEGIAmpServer.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCMemory.h>

#include <cstring>
#include <cstdio>
#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverEGIAmpServer::CDriverEGIAmpServer(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_EGIAmpServer", m_driverCtx.getConfigurationManager())
{
    m_serverHostName = "localhost";
    m_commandPort    = 9877;
    m_streamPort     = 9879;

    m_header.setSamplingFrequency(1000);
    m_header.setChannelCount(280);

    m_settings.add("Header", &m_header);
    m_settings.add("AmpServerHostName", &m_serverHostName);
    m_settings.add("CommandPort", &m_commandPort);
    m_settings.add("StreamPort", &m_streamPort);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

#define COMMAND_SIZE 4096

class CCommandConnectionHandler
{
public:
    explicit CCommandConnectionHandler(CDriverEGIAmpServer& driver, Socket::IConnection* connection = nullptr)
        : m_Driver(driver), m_Connection(connection ? *connection : *driver.m_command), m_ListenToResponse(connection == nullptr) { }

    ~CCommandConnectionHandler() { }

    bool send(const char* cmd) const
    {
        m_Driver.m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Sending command >> [" << CString(cmd) << "]\n";
        m_Connection.sendBufferBlocking(cmd, strlen(cmd));
        m_Connection.sendBufferBlocking("\n", 1);
        if (m_ListenToResponse) {
            size_t i, l = 0;
            char buffer[COMMAND_SIZE];
            do {
                i = m_Connection.receiveBuffer(const_cast<char*>(buffer + l), 1);
                if (i > 0) { l += i; }
            } while (i == 0 || (i > 0 && buffer[l - 1] != '\0' && buffer[l - 1] != '\n'));
            buffer[l > 1 ? l - 1 : l] = '\0';
            m_Driver.m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Received answer << [" << CString(buffer) << "]\n";
        }
        return true;
    }

    CDriverEGIAmpServer& m_Driver;
    Socket::IConnection& m_Connection;
    bool m_ListenToResponse;
};

#pragma pack(1)
typedef struct
{
    int64_t amplifierID;
    uint64_t size;
} SAmpServerPacketHeader;
#pragma pack()

// cmdHandler.send("(sendCommand cmd_NumberOfAmps -1 -1 -1)");
// cmdHandler.send("(sendCommand cmd_SetPower 0 -1 1)");
// cmdHandler.send("(sendCommand cmd_GetAmpDetails 0 -1 -1)");
// cmdHandler.send("(sendCommand cmd_GetAmpStatus 0 -1 -1)");
// cmdHandler.send("(sendCommand cmd_DefaultAcquisitionState 0 -1 -1)");
// cmdHandler.send("(sendCommand cmd_DefaultSignalGeneration 0 -1 -1)");
// cmdHandler.send("(sendCommand cmd_SetWaveShape 0 -1 1)");
// cmdHandler.send("(sendCommand cmd_SetCalibrationSignalRange 0 -1 -1)");
// cmdHandler.send("(sendCommand cmd_SetCalibrationSignalAmplitude 0 -1 -1)");
// cmdHandler.send("(sendCommand cmd_SetCalibrationSignalFreq 0 -1 10)");
// cmdHandler.send("(sendCommand cmd_TurnAllDriveSignals 0 -1 1)");
// streamHandler.send("(sendCommand cmd_GetStatus -1 -1 -1)");
// streamHandler.send("(sendCommand cmd_InstallEGINA300TestAmp -1 -1 -1)");

// cmdHandler.send("(sendCommand cmd_SetPower 0 -1 0)");
// cmdHandler.send("(sendCommand cmd_Exit -1 -1 -1)");

bool CDriverEGIAmpServer::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    m_command = Socket::createConnectionClient();
    if (!m_command->connect(m_serverHostName.toASCIIString(), m_commandPort)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not connect to AmpServer command port ["
                << m_serverHostName << ":" << m_commandPort << "]\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Please check the driver configuration and the firewall configuration\n";
        m_command->close();
        m_command->release();
        m_command = nullptr;
        return false;
    }

    m_stream = Socket::createConnectionClient();
    if (!m_stream->connect(m_serverHostName.toASCIIString(), m_streamPort)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not connect to AmpServer stream port ["
                << m_serverHostName << ":" << m_streamPort << "]\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Please check the driver configuration and the firewall configuration\n";
        m_stream->close();
        m_stream->release();
        m_stream = nullptr;
        m_command->close();
        m_command->release();
        m_command = nullptr;
        return false;
    }

    m_buffer = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    if (!m_buffer) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Memory allocation error.\n";
        m_stream->close();
        m_stream->release();
        m_stream = nullptr;
        m_command->close();
        m_command->release();
        m_command = nullptr;
        return false;
    }

    this->execute(Directories::getDataDir() + "/applications/acquisition-server/scripts/egi-default-initialize.script");

    const CCommandConnectionHandler cmdHandler(*this);
    cmdHandler.send("(sendCommand cmd_Start 0 -1 -1)");

    const CCommandConnectionHandler streamHandler(*this, m_stream);
    streamHandler.send("(sendCommand cmd_ListenToAmp 0 -1 -1)");

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_sampleIdx           = 0;
    m_nChannel            = m_header.getChannelCount();

    return true;
}

bool CDriverEGIAmpServer::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    this->execute(Directories::getDataDir() + "/applications/acquisition-server/scripts/egi-default-start.script");

    return true;
}

bool CDriverEGIAmpServer::loop()
{
    bool completedBuffer = false;

    if (!m_driverCtx.isConnected()) { return false; }
    // if(!m_driverCtx.isStarted()) { return true; }

    SAmpServerPacketHeader header;
    SAmpServerPacketHeader headerSwap;

    while (m_stream->isReadyToReceive() && !completedBuffer) {
        m_stream->receiveBufferBlocking(reinterpret_cast<char*>(&headerSwap), sizeof(headerSwap));
        System::Memory::bigEndianToHost(reinterpret_cast<const uint8_t*>(&headerSwap.amplifierID), &header.amplifierID);
        System::Memory::bigEndianToHost(reinterpret_cast<const uint8_t*>(&headerSwap.size), &header.size);

        if (header.size) {
            float* bufferSwap = new float[size_t(header.size / sizeof(float))];
            m_stream->receiveBufferBlocking(reinterpret_cast<char*>(bufferSwap), uint32_t(header.size));

            if (m_driverCtx.isStarted()) {
                const uint32_t nSample = uint32_t(header.size / 1152);
                for (uint32_t i = 0; i < nSample; ++i) {
                    for (uint32_t j = 0; j < m_nChannel; ++j) {
                        System::Memory::bigEndianToHost(reinterpret_cast<const uint8_t*>(bufferSwap + 8 + i * 288 + j),
                                                        m_buffer + j * m_nSamplePerSentBlock + m_sampleIdx);
                    }

                    m_sampleIdx++;
                    if (m_sampleIdx == m_nSamplePerSentBlock) {
                        completedBuffer = true;
                        m_sampleIdx     = 0;
                        m_callback->setSamples(m_buffer);
                        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
                    }
                }
            }

            delete [] bufferSwap;
        }
    }

    return true;
}

bool CDriverEGIAmpServer::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    this->execute(Directories::getDataDir() + "/applications/acquisition-server/scripts/egi-default-stop.script");

    return true;
}

bool CDriverEGIAmpServer::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_buffer) {
        delete [] m_buffer;
        m_buffer = nullptr;
    }

    if (m_stream) {
        const CCommandConnectionHandler handler(*this, m_stream);
        handler.send("(sendCommand cmd_StopListeningToAmp 0 -1 -1)");

        m_stream->close();
        m_stream->release();
        m_stream = nullptr;
    }

    if (m_command) {
        const CCommandConnectionHandler handler(*this);
        handler.send("(sendCommand cmd_Stop 0 -1 -1)");

        this->execute(Directories::getDataDir() + "/applications/acquisition-server/scripts/egi-default-uninitialize.script");

        m_command->close();
        m_command->release();
        m_command = nullptr;
    }

    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverEGIAmpServer::configure()
{
    CConfigurationEGIAmpServer config(Directories::getDataDir() + "/applications/acquisition-server/interface-egi-ampserver.ui");

    config.setHostName(m_serverHostName);
    config.setCommandPort(m_commandPort);
    config.setStreamPort(m_streamPort);

    if (config.configure(m_header)) {
        m_serverHostName = config.getHostName();
        m_commandPort    = config.getCommandPort();
        m_streamPort     = config.getStreamPort();
        m_settings.save();
        return true;
    }
    return false;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverEGIAmpServer::execute(const char* sScriptFilename)
{
    FILE* file = fopen(sScriptFilename, "rb");
    if (!file) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not find script [" << CString(sScriptFilename) << "]\n";
        return false;
    }

    const CCommandConnectionHandler cmdHandler(*this);
    while (!feof(file)) {
        char buffer[4096];
        char* line = fgets(buffer, sizeof(buffer), file);
        if (line) {
            line[strlen(line) - 1] = '\0'; // replaces \n with \0 (\n is automatically added by the command handler)
            cmdHandler.send(line);
        }
    }

    fclose(file);

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
