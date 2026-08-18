/*
 * OpenBCI driver for OpenViBE
 *
 * \author Jeremy Frey
 * \author Yann Renard
 *
 */

#include "ovasCDriverOpenBCI.h"
#include "ovasCConfigurationOpenBCI.h"

#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>

#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <sstream>

#if defined TARGET_OS_Windows
#include <windows.h>
#include <winbase.h>
#include <cstdio>
#include <commctrl.h>
#include <winsock2.h> // htons and co.
//#define TERM_SPEED 57600
#define TERM_SPEED CBR_115200 // OpenBCI is a bit faster than others
#elif defined TARGET_OS_Linux
 #include <cstdio>
 #include <unistd.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <sys/select.h>
 #include <netinet/in.h> // htons and co.
 #include <unistd.h>
 #define TERM_SPEED B115200
#else
#endif

namespace OpenViBE {
namespace AcquisitionServer {

// packet number at initialization
#define UNINITIALIZED_PACKET_NUMBER -1
#define UNDEFINED_DEVICE_IDENTIFIER uint32_t(-1)
#define READ_ERROR uint32_t(-1)
#define WRITE_ERROR uint32_t(-1)

// start and stop bytes from OpenBCI protocl
#define SAMPLE_START_BYTE 0xA0
#define SAMPLE_STOP_BYTE 0xC0

// some constants related to the sendCommand
#define ADS1299_VREF 4.5  // reference voltage for ADC in ADS1299.  set by its hardware
#define ADS1299_GAIN 24.0  //assumed gain setting for ADS1299.  set by its Arduino code

// configuration tokens
#define Token_MissingSampleDelayBeforeReset       "AcquisitionDriver_OpenBCI_MissingSampleDelayBeforeReset"
#define Token_DroppedSampleCountBeforeReset       "AcquisitionDriver_OpenBCI_DroppedSampleCountBeforeReset"
#define Token_DroppedSampleSafetyDelayBeforeReset "AcquisitionDriver_OpenBCI_DroppedSampleSafetyDelayBeforeReset"

//___________________________________________________________________//
// Heavily inspired by OpenEEG code. Will override channel count and sampling late upon "daisy" selection. If daisy module is attached, will concatenate EEG values and average accelerometer values every two samples.
//                                                                   //

CDriverOpenBCI::CDriverOpenBCI(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_OpenBCI", m_driverCtx.getConfigurationManager())
{
    m_additionalCmds         = "";
    m_readBoardReplyTimeout  = 5000;
    m_flushBoardReplyTimeout = 500;
    m_driverName             = "OpenBCI";

    m_settings.add("Header", &m_header);
    m_settings.add("DeviceIdentifier", &m_deviceID);
    m_settings.add("ComInit", &m_additionalCmds);
    m_settings.add("ReadBoardReplyTimeout", &m_readBoardReplyTimeout);
    m_settings.add("FlushBoardReplyTimeout", &m_flushBoardReplyTimeout);
    m_settings.add("DaisyModule", &m_daisyModule);

    m_settings.load();

    m_missingSampleDelayBeforeReset       = uint32_t(ctx.getConfigurationManager().expandAsUInteger(Token_MissingSampleDelayBeforeReset, 1000));
    m_droppedSampleCountBeforeReset       = uint32_t(ctx.getConfigurationManager().expandAsUInteger(Token_DroppedSampleCountBeforeReset, 5));
    m_droppedSampleSafetyDelayBeforeReset = uint32_t(ctx.getConfigurationManager().expandAsUInteger(Token_DroppedSampleSafetyDelayBeforeReset, 1000));

    // default parameter loaded, update channel count and frequency
    this->updateDaisy(true);
}

//___________________________________________________________________//
//                                                                   //

void CDriverOpenBCI::updateDaisy(const bool quietLogging)
{
    // change channel and sampling rate according to daisy module
    const auto info = CConfigurationOpenBCI::getDaisyInformation(m_daisyModule ? CConfigurationOpenBCI::EDaisyStatus::Active
                                                                     : CConfigurationOpenBCI::EDaisyStatus::Inactive);

    m_header.setSamplingFrequency(info.sampling);
    m_header.setChannelCount(info.nEEGChannel + info.nAccChannel);

    if (!quietLogging) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Status - " << CString(m_daisyModule ? "Daisy" : "** NO ** Daisy") <<
                " module option enabled, " << m_header.getChannelCount() << " channels -- " << int((m_daisyModule ? 2 : 1) * EEG_VALUE_COUNT_PER_SAMPLE) <<
                " EEG and " << int(ACC_VALUE_COUNT_PER_SAMPLE) << " accelerometer -- at " << m_header.getSamplingFrequency() << "Hz.\n";
    }

    // microvolt for EEG channels
    for (int i = 0; i < info.nEEGChannel; ++i) { m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }

    // undefined for accelerometer/extra channels
    for (int i = 0; i < info.nAccChannel; ++i) { m_header.setChannelUnits(info.nEEGChannel + i, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base); }
}

bool CDriverOpenBCI::initialize(const uint32_t /*nSamplePerSentBlock*/, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Configured 'missing sample delay before reset' to " <<
            m_missingSampleDelayBeforeReset << " ; this can be changed in the openvibe configuration file setting the " << CString(
                Token_MissingSampleDelayBeforeReset) << " token\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Configured 'dropped sample count before reset' to " <<
            m_droppedSampleCountBeforeReset << " ; this can be changed in the openvibe configuration file setting the " << CString(
                Token_DroppedSampleSafetyDelayBeforeReset) << " token\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Configured 'dropped sample safety delay before reset' to " <<
            m_droppedSampleSafetyDelayBeforeReset << " ; this can be changed in the openvibe configuration file setting the " << CString(
                Token_DroppedSampleSafetyDelayBeforeReset) << " token\n";

    m_nChannel = m_header.getChannelCount();

    // Initializes buffer data structures
    m_readBuffers.clear();
    m_readBuffers.resize(1024 * 16); // 16 kbytes of read buffer
    m_callbackSamples.clear();

    // change channel and sampling rate according to daisy module
    this->updateDaisy(false);

    // init state
    m_readState        = ParserAutomaton_Default;
    m_extractPosition  = 0;
    m_sampleNumber     = -1;
    m_seenPacketFooter = true; // let's say we will start with header

    if (!this->openDevice(&m_fileDesc, m_deviceID)) { return false; }

    // check board status and print response
    if (!this->resetBoard(m_fileDesc, true)) {
        this->closeDevice(m_fileDesc);
        return false;
    }

    // prepare buffer for samples
    m_sampleEEGBuffers.resize(EEG_VALUE_COUNT_PER_SAMPLE);
    m_sampleEEGBuffersDaisy.resize(EEG_VALUE_COUNT_PER_SAMPLE);
    m_sampleAccBuffers.resize(ACC_VALUE_COUNT_PER_SAMPLE);
    m_sampleAccBuffersTemp.resize(ACC_VALUE_COUNT_PER_SAMPLE);

    // init buffer for 1 EEG value and 1 accel value
    m_eegValueBuffers.resize(EEG_VALUE_BUFFER_SIZE);
    m_accValueBuffers.resize(ACC_VALUE_BUFFER_SIZE);
    m_sampleBuffers.resize(m_nChannel);

    m_callback         = &callback;
    m_lastPacketNumber = UNINITIALIZED_PACKET_NUMBER;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << CString(this->getName()) << " driver initialized.\n";

    // init scale factor
    m_unitsToMicroVolts = float(ADS1299_VREF * 1000000.0 / ((pow(2., 23) - 1) * ADS1299_GAIN));
    m_unitsToRadians    = float(0.002 / pow(2., 4)); // @aj told me - this is undocumented and may have been taken from the OpenBCI plugin for processing

#if 0
    uint32_t unitsToMicroVolts = (float) (ADS1299_VREF/(pow(2.,23)-1)/ADS1299_GAIN*1000000.); // $$$$ The notation here is ambiguous
    ::printf("CHECK THIS OUT : %g %g %g\n", unitsToMicroVolts-m_unitsToMicroVolts, unitsToMicroVolts, m_unitsToMicroVolts);
#endif

    return true;
}

bool CDriverOpenBCI::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << CString(this->getName()) << " driver started.\n";
    return true;
}

bool CDriverOpenBCI::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }

    // try to awake the board if there's something wrong
    if (System::Time::getTime() - m_tick > m_missingSampleDelayBeforeReset) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "No response for " << uint32_t(m_missingSampleDelayBeforeReset) <<
                "ms, will try recovery now (Note this may eventually be hopeless as the board may not reply to any command either).\n";
        if (!this->resetBoard(m_fileDesc, false)) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName << ": Did not succeed reseting board\n";
            return false;
        }
    }

    // read datastream from device
    const uint32_t length = this->readFromDevice(m_fileDesc, &m_readBuffers[0], m_readBuffers.size());
    if (length == READ_ERROR) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName << ": Could not receive data from [" << m_ttyName << "]\n";
        return false;
    }

    // pass bytes one by one to parser to extract samples
    for (uint32_t i = 0; i < length; ++i) {
        // will have effect only if complete sample/packet
        if (this->handleCurrentSample(this->parseByte(m_readBuffers[i]))) {
            // One full packet was processed !
        }
    }

    // now deal with acquired samples
    if (!m_channelBuffers.empty()) {
        if (m_driverCtx.isStarted()) {
            m_callbackSamples.resize(m_nChannel * m_channelBuffers.size());

            for (uint32_t i = 0, k = 0; i < m_nChannel; ++i) {
                for (uint32_t j = 0; j < m_channelBuffers.size(); ++j) { m_callbackSamples[k++] = m_channelBuffers[j][i]; }
            }
            m_callback->setSamples(&m_callbackSamples[0], m_channelBuffers.size());
            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
        }
        m_channelBuffers.clear();
    }
    return true;
}

bool CDriverOpenBCI::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << CString(this->getName()) << " driver stopped.\n";
    return true;
}

bool CDriverOpenBCI::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    this->closeDevice(m_fileDesc);

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << CString(this->getName()) << " driver closed.\n";

    // Uninitializes data structures
    m_readBuffers.clear();
    m_callbackSamples.clear();
    m_ttyName = "";

#if 0
    delete [] m_sample;
    m_sample= nullptr;
#endif
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverOpenBCI::configure()
{
    CConfigurationOpenBCI config(Directories::getDataDir() + "/applications/acquisition-server/interface-OpenBCI.ui", m_deviceID);

    config.setAdditionalCommands(m_additionalCmds);
    config.setReadBoardReplyTimeout(m_readBoardReplyTimeout);
    config.setFlushBoardReplyTimeout(m_flushBoardReplyTimeout);
    config.setDaisyModule(m_daisyModule);

    if (!config.configure(m_header)) { return false; }

    m_additionalCmds         = config.getAdditionalCommands();
    m_readBoardReplyTimeout  = config.getReadBoardReplyTimeout();
    m_flushBoardReplyTimeout = config.getFlushBoardReplyTimeout();
    m_daisyModule            = config.getDaisyModule();
    m_settings.save();

    this->updateDaisy(false);

    return true;
}

// Convert EEG value format from int24 MSB (network order) to int host
// TODO: check on big endian architecture
int CDriverOpenBCI::interpret24bitAsInt32(const std::vector<uint8_t>& byteBuffer)
{
    // create a big endian so that we could adapt to host architecture later on
    int newInt = (byteBuffer[2] << 24) | (byteBuffer[1] << 16) | byteBuffer[0] << 8;
    // depending on most significant byte, set positive or negative value
    if ((newInt & 0x00008000) > 0) { newInt |= 0x000000FF; }
    else { newInt &= 0xFFFFFF00; }
    // convert back from big endian (network order) to host
    return htonl(newInt);
}

// Convert EEG value format from int16_t MSB (network order) to int host
int CDriverOpenBCI::interpret16bitAsInt32(const std::vector<uint8_t>& byteBuffer)
{
    // create a big endian so that we could adapt to host architecture later on
    int newInt = (byteBuffer[1] << 24) | byteBuffer[0] << 16;
    // depending on most significant byte, set positive or negative value
    if ((newInt & 0x00800000) > 0) { newInt |= 0x0000FFFF; }
    else { newInt &= 0xFFFF0000; }
    // convert back from big endian (network order) to host
    return htonl(newInt);
}

// return sample number once one is received (between 0 and 255, -1 if none)
// NB: will wait to get footer and then header in a row, may miss a packet but will prevent a bad sync with stream (thx BrainBay for the tip!)
int16_t CDriverOpenBCI::parseByte(const uint8_t actbyte)
{
    // finished to read sample or not
    bool status = false;

    switch (m_readState) {
        // Default state: wait for Start byte
        case ParserAutomaton_Default:
            // if first byte is not the one expected, won't go further
            if (actbyte == SAMPLE_STOP_BYTE) { m_seenPacketFooter = true; }
            else {
                if (actbyte == SAMPLE_START_BYTE && m_seenPacketFooter) { m_readState = ParserAutomaton_StartByteReceived; }
                m_seenPacketFooter = false;
            }
            // reset sample info
            m_sampleNumber         = -1;
            m_extractPosition      = 0;
            m_sampleBufferPosition = 0;
            break;

        // Start byte received, consider next byte as sample number
        case ParserAutomaton_StartByteReceived:
            m_sampleNumber = actbyte;
            m_readState = ParserAutomaton_SampleNumberReceived;
            break;

        // Sample number received, next bytes hold the EEG data
        /*
         * Note: values are 24-bit signed, MSB first
         * Bytes 3-5: Data value for EEG channel 1
         * Bytes 6-8: Data value for EEG channel 2
         * Bytes 9-11: Data value for EEG channel 3
         * Bytes 12-14: Data value for EEG channel 4
         * Bytes 15-17: Data value for EEG channel 5
         * Bytes 18-20: Data value for EEG channel 6
         * Bytes 21-23: Data value for EEG channel 6
         * Bytes 24-26: Data value for EEG channel 8
         */
        case ParserAutomaton_SampleNumberReceived:
            if (m_extractPosition < EEG_VALUE_COUNT_PER_SAMPLE) {
                // fill EEG buffer
                if (m_sampleBufferPosition < EEG_VALUE_BUFFER_SIZE) {
                    m_eegValueBuffers[m_sampleBufferPosition] = actbyte;
                    m_sampleBufferPosition++;
                }

                // we got EEG value
                if (m_sampleBufferPosition == EEG_VALUE_BUFFER_SIZE) {
                    // fill EEG channel buffer, converting at the same time from 24 to 32 bits + scaling
                    m_sampleEEGBuffers[m_extractPosition] = float(this->interpret24bitAsInt32(m_eegValueBuffers)) * m_unitsToMicroVolts;

                    // reset for next value
                    m_sampleBufferPosition = 0;
                    m_extractPosition++;
                }
            }

            // finished with EEG
            if (m_extractPosition == EEG_VALUE_COUNT_PER_SAMPLE) {
                // next step: accelerometer
                m_readState = ParserAutomaton_EEGSamplesReceived;
                // re-use the same variable to know position inside accelerometer block (I know, I'm bad!).
                m_extractPosition     = 0;
                m_nValidAccelerometer = 0;
            }
            break;

        // EEG Samples received, next bytes hold the Accelerometer data
        /*
         * Note: values are 16-bit signed, MSB first
         * Bytes 27-28: Data value for accelerometer channel X
         * Bytes 29-30: Data value for accelerometer channel Y
         * Bytes 31-32: Data value for accelerometer channel Z
         */
        case ParserAutomaton_EEGSamplesReceived: 
            if (m_extractPosition < ACC_VALUE_COUNT_PER_SAMPLE) {
                // fill Acc buffer
                if (m_sampleBufferPosition < ACC_VALUE_BUFFER_SIZE) {
                    m_accValueBuffers[m_sampleBufferPosition] = actbyte;
                    m_sampleBufferPosition++;
                }

                // we got Acc value
                if (m_sampleBufferPosition == ACC_VALUE_BUFFER_SIZE) {
                    // fill Acc channel buffer, converting at the same time from 16 to 32 bits
                    m_sampleAccBuffersTemp[m_extractPosition] = float(this->interpret16bitAsInt32(m_accValueBuffers)) * m_unitsToRadians;
                    if (m_sampleAccBuffersTemp[m_extractPosition] != 0) { m_nValidAccelerometer++; }

                    // reset for next value
                    m_sampleBufferPosition = 0;
                    m_extractPosition++;
                }
            }

            // finished with acc
            if (m_extractPosition == ACC_VALUE_COUNT_PER_SAMPLE) {
                // next step: footer
                m_readState = ParserAutomaton_AccelerometerSamplesReceived;

                if (m_nValidAccelerometer == ACC_VALUE_COUNT_PER_SAMPLE) { m_sampleAccBuffers.swap(m_sampleAccBuffersTemp); }
            }
            break;

        // Accelerometer Samples received, now expect a stop byte to terminate the frame
        case ParserAutomaton_AccelerometerSamplesReceived:
            // expected footer: perfect, returns sample number
            if (actbyte == SAMPLE_STOP_BYTE) {
                // we shall pass
                status = true;
                // we're ockay for next time
                m_seenPacketFooter = true;
            }
            // if last byte is not the one expected, discard whole sample
            else { }
            // whatever happened, it'll be the end of this journey
            m_readState = ParserAutomaton_Default;
            break;

        // uh-oh, should not be there
        default: 
            if (actbyte == SAMPLE_STOP_BYTE) {
                // we're ockay for next time
                m_seenPacketFooter = true;
            }
            m_readState = ParserAutomaton_Default;
            break;
    }

    // if it's a GO, returns sample number, may trigger channel push
    if (status) { return m_sampleNumber; }
    // by default we're not ready
    return -1;
}

//___________________________________________________________________//
//                                                                   //

// if waitForResponse, will wait response before leaving the function (until timeout is reached)
// if logResponse, the actual response is sent to log manager
// timeout: time to sleep between each character written (in ms)
bool CDriverOpenBCI::sendCommand(const FD_TYPE fileDesc, const char* cmd, const bool waitForResponse, const bool logResponse, const uint32_t timeout,
                                 std::string& reply)
{
    const uint32_t size = strlen(cmd);
    reply               = "";

    // no command: don't go further
    if (size == 0) { return true; }

    // write command to the board
    for (size_t i = 0; i < size; ++i) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ": Sending sequence to OpenBCI board [" << std::string(1, cmd[i])
                <<
                "]\n";
        if (this->writeToDevice(fileDesc, &cmd[i], 1) == WRITE_ERROR) { return false; }

        // wait for response
        if (waitForResponse) {
            // buffer for serial reading
            std::ostringstream readStream;

            uint64_t out          = timeout;
            const uint64_t tStart = System::Time::getTime();
            bool finished         = false;
            while (System::Time::getTime() - tStart < out && !finished) {
                const uint32_t readLength = this->readFromDevice(fileDesc, &m_readBuffers[0], m_readBuffers.size(), 10);
                if (readLength == READ_ERROR) { return false; }
                readStream.write(reinterpret_cast<const char*>(&m_readBuffers[0]), readLength);

                // early stop when the "$$$" pattern is detected
                const std::string& content  = readStream.str();
                const std::string earlyStop = "$$$";
                if (content.size() >= earlyStop.size()) {
                    if (content.substr(content.size() - earlyStop.size(), earlyStop.size()) == earlyStop) { finished = true; }
                }
            }

            if (!finished) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ": After " << out <<
                        "ms, timed out while waiting for board response !\n";
            }

            // now log response to log manager
            if (logResponse) {
                // readStream stream to std::string and then to const to please log manager
                if (!readStream.str().empty()) {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ": " << (
                        finished ? "Board response" : "Partial board response") << " was (size=" << readStream.str().size() << ") :\n";
                    m_driverCtx.getLogManager() << readStream.str() << "\n";
                }
                else { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ": Board did not reply !\n"; }
            }

            // saves reply
            reply += readStream.str();
        }
        else {
            // When no reply is expected, wait at least 100ms that the commands hits the device
            System::Time::sleep(100);
        }
    }

    return true;
}

bool CDriverOpenBCI::resetBoard(const FD_TYPE fileDescriptor, const bool regularInitialization)
{
    const uint32_t startTime = System::Time::getTime();
    std::string reply;

    // stop/reset/default board
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Stopping board streaming...\n";
    if (!this->sendCommand(fileDescriptor, "s", true, false, m_flushBoardReplyTimeout, reply)
    ) // the waiting serves to flush pending samples after stopping the streaming
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName << ": Did not succeed in stopping board !\n";
        return false;
    }

    // regular initialization of the board (not for recovery)
    if (regularInitialization) {
        std::string line;

        // reset 32-bit board (no effect with 8bit board)
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Soft reseting of the board...\n";
        if (!this->sendCommand(fileDescriptor, "v", true, true, m_readBoardReplyTimeout, reply)) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName << ": Did not succeed in soft reseting board !\n";
            return false;
        }

        // gets meaningful information from reply
        // WARNING - The parsing here is not very robust
        //           The parsing here is very dependant on the current protocol definition
        std::istringstream cmdReplyInputStringStream(reply);
        memset(&m_deviceInfo, 0, sizeof(m_deviceInfo));
        while (std::getline(cmdReplyInputStringStream, line, '\n')) {
            if (line != "$$$") {
                sscanf(line.c_str(), "OpenBCI V%u %u channel", &m_deviceInfo.deviceVersion, &m_deviceInfo.deviceChannelCount);
                sscanf(line.c_str(), "On Board %s Device ID: 0x%x", m_deviceInfo.boardChipset, &m_deviceInfo.boardId);
                sscanf(line.c_str(), "On Daisy %s Device ID: 0x%x", m_deviceInfo.daisyChipset, &m_deviceInfo.daisyId);
                sscanf(line.c_str(), "%[^ ] Device ID: 0x%x", m_deviceInfo.mocapChipset, &m_deviceInfo.mocapId);
            }
        }

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ": Got board config\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ":   Version: " << m_deviceInfo.deviceVersion << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ":   Channel count: " << m_deviceInfo.deviceChannelCount << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ":   Board (devid:" << m_deviceInfo.boardId << ", devchip:" <<
                m_deviceInfo.
                boardChipset << ")\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ":   Daisy (devid:" << m_deviceInfo.daisyId << ", devchip:" <<
                m_deviceInfo.
                daisyChipset << ")\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName << ":   Mocap (devid:" << m_deviceInfo.mocapId << ", devchip:" <<
                m_deviceInfo.
                mocapChipset << ")\n";

        // verifies openbci board presence
        if (m_deviceInfo.deviceVersion == 0x00) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName <<
                    ": The attached device does not look like an OpenBCI device\n";
            return false;
        }

        // verifies daisy module presence
        if (m_deviceInfo.daisyId == 0x00 && m_daisyModule) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName <<
                    ": The attached device does not match current configuration with Daisy module requested while not connected on the board\n";
            return false;
        }

        // After discussin with @Aj May 2016 it has been decided to disable
        // the daisy module when it was present and not requested instead of
        // checking its presence

#if 0
        // verifies daisy module absence
        if(m_deviceInfo.daisyId!=0x00 && !m_daisyModule)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName << ": Configured without daisy module while present on the board, please unplug\n";
            return false;
        }
#else
        // Disables daisy module when necessary
        if (m_deviceInfo.daisyId != 0x00 && !m_daisyModule) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << this->m_driverName <<
                    ": Daisy module present but not requested, will now be disabled\n";
            if (!this->sendCommand(fileDescriptor, "c", true, true, m_readBoardReplyTimeout, reply)) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName <<
                        ": Did not succeed in disabling daisy module !\n";
                return false;
            }
        }
#endif

        // sends additional commands if necessary
        std::istringstream ss(m_additionalCmds.toASCIIString());
        while (std::getline(ss, line, '\255')) {
            if (line.length() > 0) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Additional custom commands for initialization : [" <<
                        line
                        << "]\n";
                if (!this->sendCommand(fileDescriptor, line.c_str(), true, true, m_readBoardReplyTimeout, reply)) {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName <<
                            ": Did not succeed sending additional command [" << line
                            << "] !\n";
                    return false;
                }
            }
        }
    }

    // start stream
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Starting stream...\n";
    if (!this->sendCommand(fileDescriptor, "b", false, false, m_readBoardReplyTimeout, reply)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName << ": Did not succeed starting stream\n";
        return false;
    }

    // should start streaming!
    m_startTime        = System::Time::getTime();
    m_tick             = m_startTime;
    m_lastPacketNumber = UNINITIALIZED_PACKET_NUMBER;
    m_droppedSampleTimes.clear();
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Status ready (initialization took " << m_tick - startTime
            << "ms)\n";

    return true;
}

// update internal state (lastPacket, number of packet processed, etc.).
// returns true if a new sample is created
bool CDriverOpenBCI::handleCurrentSample(const int packetNumber)
{
    bool ok = false; // true if a sample is added to m_channelBuffers
    // if == -1, current sample is incomplete or corrupted
    if (packetNumber >= 0) {
        bool hasDroppedSamples = false;

        // check packet drop
        if ((m_lastPacketNumber != UNINITIALIZED_PACKET_NUMBER) && ((m_lastPacketNumber + 1) % 256 != packetNumber)) {
            const uint32_t sampleTime = System::Time::getTime() - m_startTime;
            while (!m_droppedSampleTimes.empty() && sampleTime - m_droppedSampleTimes.front() > 1000) { m_droppedSampleTimes.pop_front(); }
            hasDroppedSamples = true;

            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << this->m_driverName << ": Packet dropped! [Last,Current]=[" << int(
                        m_lastPacketNumber)
                    <<
                    "," << packetNumber << "] (that was " << uint32_t(m_droppedSampleTimes.size() + 1) << " dropped samples lately)\n";

            if (sampleTime > m_droppedSampleSafetyDelayBeforeReset) {
                m_droppedSampleTimes.push_back(sampleTime);
                if (m_droppedSampleTimes.size() > m_droppedSampleCountBeforeReset) {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << this->m_driverName <<
                            ": Too many samples dropped, will now attempt to recover with a reset of the board\n";
                    if (!this->resetBoard(m_fileDesc, false)) {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << this->m_driverName << ": Did not succeed reseting board\n";
                        return false;
                    }
                }
            }
        }

        // no daisy module: push directly values
        if (!m_daisyModule) {
            // concatenate EEG and Acc
            // YRD NOTE: OPENBCI ACCELEROMETERS ARE SAMPLED WAY BELOW EEG (~25Hz, depends upon firmware)
            // YRD NOTE: CONSEQUENTLY, THIS CODE CREATES BLOCKS OF DUPLICATES ACCELEROMETERS SAMPLES

            std::copy(m_sampleEEGBuffers.begin(), m_sampleEEGBuffers.end(), m_sampleBuffers.begin());
            std::copy(m_sampleAccBuffers.begin(), m_sampleAccBuffers.end(), m_sampleBuffers.begin() + m_sampleEEGBuffers.size());

            // copy them to current chunk
            m_channelBuffers.push_back(m_sampleBuffers);
            ok = true;
        }
        // even: daisy, odd: first 8 channels
        else {
            // on odd packet, got complete sample
            if (packetNumber % 2) {
                // won't concatenate if there was packet drop
                if (!hasDroppedSamples) {
                    // Concatenate standard and daisy EEG values and Acc
                    // YRD NOTE: OPENBCI ACCELEROMETERS ARE SAMPLED WAY BELOW EEG (~25Hz, depends upon firmware)
                    // YRD NOTE: CONSEQUENTLY, THIS CODE CREATES BLOCKS OF DUPLICATES ACCELEROMETERS SAMPLES

                    std::copy(m_sampleEEGBuffers.begin(), m_sampleEEGBuffers.end(), m_sampleBuffers.begin());
                    std::copy(m_sampleEEGBuffersDaisy.begin(), m_sampleEEGBuffersDaisy.end(), m_sampleBuffers.begin() + m_sampleEEGBuffers.size());
                    std::copy(m_sampleAccBuffers.begin(), m_sampleAccBuffers.end(),
                              m_sampleBuffers.begin() + m_sampleEEGBuffers.size() + m_sampleEEGBuffersDaisy.size());

                    // at last, add to chunk
                    m_channelBuffers.push_back(m_sampleBuffers);
                    ok = true;
                }
            }
            // an even packet: it's Daisy, store values for later
            else {
                // swap may modify origin, but it's faster
                m_sampleEEGBuffersDaisy.swap(m_sampleEEGBuffers);
            }
        }

        m_lastPacketNumber = packetNumber;
    }

    // something to read: won't have to poll before "long"
    if (ok) { m_tick = System::Time::getTime(); }

    return ok;
}

bool CDriverOpenBCI::openDevice(FD_TYPE* fileDesc, const uint32_t ttyNumber)
{
    CString ttyName;

    if (ttyNumber == UNDEFINED_DEVICE_IDENTIFIER) {
        // Tries to find an existing port to connect to
        uint32_t i   = 0;
        bool success = false;
        do {
            ttyName = CConfigurationOpenBCI::getTTYFileName(i++);
            if (CConfigurationOpenBCI::isTTYFile(ttyName)) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Automatically picked port [" << i << ":" << ttyName <<
                        "]\n";
                success = true;
            }
        } while (!success && i < CConfigurationOpenBCI::getMaximumTtyCount());

        if (!success) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName <<
                    ": Port has not been configure and driver could not open any port\n";
            return false;
        }
    }
    else { ttyName = CConfigurationOpenBCI::getTTYFileName(ttyNumber); }

#if defined TARGET_OS_Windows

    DCB dcb   = { 0 };
    *fileDesc = ::CreateFile(LPCSTR(ttyName.toASCIIString()), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                             nullptr);

    if (*fileDesc == INVALID_HANDLE_VALUE) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName << ": Could not open port [" << ttyName << "]\n";
        return false;
    }

    if (!GetCommState(*fileDesc, &dcb)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName << ": Could not get comm state on port [" << ttyName << "]\n";
        return false;
    }

    // update DCB rate, byte size, parity, and stop bits size
    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate  = TERM_SPEED;
    dcb.ByteSize  = 8;
    dcb.Parity    = NOPARITY;
    dcb.StopBits  = ONESTOPBIT;
    dcb.EvtChar   = '\0';

    // update flow control settings
    dcb.fDtrControl       = DTR_CONTROL_ENABLE;
    dcb.fRtsControl       = RTS_CONTROL_ENABLE;
    dcb.fOutxCtsFlow      = FALSE;
    dcb.fOutxDsrFlow      = FALSE;
    dcb.fDsrSensitivity   = FALSE;
    dcb.fOutX             = FALSE;
    dcb.fInX              = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.XonChar           = 0;
    dcb.XoffChar          = 0;
    dcb.XonLim            = 0;
    dcb.XoffLim           = 0;
    dcb.fParity           = FALSE;

    SetCommState(*fileDesc, &dcb);
    SetupComm(*fileDesc, 64/*1024*/, 64/*1024*/);
    EscapeCommFunction(*fileDesc, SETDTR);
    SetCommMask(*fileDesc, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_RING);

#elif defined TARGET_OS_Linux

    struct termios terminalAttributes;

    if((*fileDesc=::open(ttyName.toASCIIString(), O_RDWR))==-1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName << ": Could not open port [" << ttyName << "]\n";
        return false;
    }

    if(tcgetattr(*fileDesc, &terminalAttributes)!=0)
    {
        ::close(*fileDesc);
        *fileDesc=-1;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName << ": terminal: tcgetattr() failed - did you use the right port [" << ttyName << "] ?\n";
        return false;
    }

    /* terminalAttributes.c_cflag = TERM_SPEED | CS8 | CRTSCTS | CLOCAL | CREAD; */
    terminalAttributes.c_cflag = TERM_SPEED | CS8 | CLOCAL | CREAD;
    terminalAttributes.c_iflag = 0;
    terminalAttributes.c_oflag = OPOST | ONLCR;
    terminalAttributes.c_lflag = 0;
    if(tcsetattr(*fileDesc, TCSAFLUSH, &terminalAttributes)!=0)
    {
        ::close(*fileDesc);
        *fileDesc=-1;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << this->m_driverName << ": terminal: tcsetattr() failed - did you use the right port [" << ttyName << "] ?\n";
        return false;
    }

#else
    return false;
#endif

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << this->m_driverName << ": Successfully opened port [" << ttyName << "]\n";
    m_ttyName = ttyName;
    return true;
}

void CDriverOpenBCI::closeDevice(const FD_TYPE fileDesc)
{
#if defined TARGET_OS_Windows
    CloseHandle(fileDesc);
#elif defined TARGET_OS_Linux
    ::close(fileDesc);
#else
#endif
}

uint32_t CDriverOpenBCI::writeToDevice(const FD_TYPE fileDesc, const void* buffer, const uint32_t size)
{
#if defined TARGET_OS_Windows
    DWORD length = 0;
    if (FALSE == WriteFile(fileDesc, buffer, size, &length, nullptr)) { return WRITE_ERROR; }
    const int count = int(length);
#elif defined TARGET_OS_Linux
    const int count = ::write(fileDesc, buffer, size);
    if(count < 0) { return WRITE_ERROR; }
#else
  const int count = ::write(fileDesc, buffer, size);
  if(count < 0) { return WRITE_ERROR; }
//  return WRITE_ERROR;
#endif

    return uint32_t(count);
}

uint32_t CDriverOpenBCI::readFromDevice(const FD_TYPE fileDesc, void* buffer, const uint32_t size, const uint64_t timeOut)
{
#if defined TARGET_OS_Windows

    uint32_t readLength = 0;
    uint32_t readOk     = 0;
    struct _COMSTAT status;
    DWORD state;

    if (ClearCommError(fileDesc, &state, &status)) { readLength = (status.cbInQue < size ? status.cbInQue : size); }

    if (readLength > 0) {
        if (FALSE == ReadFile(fileDesc, buffer, readLength, LPDWORD(&readOk), nullptr)) { return READ_ERROR; }
        return readLength;
    }
    return 0;

#elif defined TARGET_OS_Linux

    fd_set  inputFileDescSet;
    struct timeval val;
    bool finished=false;

    val.tv_sec=0;
    val.tv_usec=((timeOut>>20)*1000*1000)>>12;

    uint32_t bytesLeftToRead=size;
    do
    {
        FD_ZERO(&inputFileDescSet);
        FD_SET(fileDesc, &inputFileDescSet);

        switch(select(fileDesc + 1, &inputFileDescSet, nullptr, nullptr, &val))
        {
            case -1: // error
                return READ_ERROR;

            case  0: // timeout
                finished = true;
                break;

            default:
                if(FD_ISSET(fileDesc, &inputFileDescSet))
                {
                    size_t readLength=::read(fileDesc, reinterpret_cast<uint8_t*>(buffer)+size-bytesLeftToRead, bytesLeftToRead);
                    if(readLength <= 0) { finished = true; }
                    else { bytesLeftToRead-=uint32_t(readLength); }
                }
                else { finished = true; }
                break;
        }
    }
    while(!finished && bytesLeftToRead != 0);

    return size - bytesLeftToRead;
#else
    return 0;
#endif
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
