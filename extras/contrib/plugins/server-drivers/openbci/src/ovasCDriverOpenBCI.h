/*
 * OpenBCI driver for OpenViBE
 *
 * \author Jeremy Frey
 * \author Yann Renard
 *
 */
#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows
typedef void* FD_TYPE;
#elif defined TARGET_OS_Linux
 typedef int FD_TYPE;
#else
 typedef int FD_TYPE;
#endif

#include <vector>
#include <deque>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverOpenBCI
 * \author Jeremy Frey
 * \author Yann Renard
 */
class CDriverOpenBCI final : public IDriver
{
public:

    explicit CDriverOpenBCI(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return m_driverName.toASCIIString(); }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    typedef enum
    {
        ParserAutomaton_Default,
        ParserAutomaton_StartByteReceived,
        ParserAutomaton_SampleNumberReceived,
        ParserAutomaton_EEGSamplesReceived,
        ParserAutomaton_AccelerometerSamplesReceived,
    } EParserAutomaton;

protected:

    int interpret24bitAsInt32(const std::vector<uint8_t>& byteBuffer);
    int interpret16bitAsInt32(const std::vector<uint8_t>& byteBuffer);
    int16_t parseByte(uint8_t actbyte);

    bool sendCommand(FD_TYPE fileDesc, const char* cmd, bool waitForResponse, bool logResponse, uint32_t timeout, std::string& reply);
    bool resetBoard(FD_TYPE fileDescriptor, bool regularInitialization);
    bool handleCurrentSample(int packetNumber); // will take car of samples fetch from OpenBCI board, dropping/merging packets if necessary
    void updateDaisy(bool quietLogging); // update internal state regarding daisy module

    bool openDevice(FD_TYPE* fileDesc, uint32_t ttyNumber);
    static void closeDevice(FD_TYPE fileDesc);
    static uint32_t writeToDevice(FD_TYPE fileDesc, const void* buffer, uint32_t size);
    static uint32_t readFromDevice(FD_TYPE fileDesc, void* buffer, uint32_t size, uint64_t timeOut = 0);

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    FD_TYPE m_fileDesc;

    CString m_driverName = "OpenBCI";
    CString m_ttyName;
    CString m_additionalCmds;                       // string to send possibly upon initialisation
    uint32_t m_nChannel               = EEG_VALUE_BUFFER_SIZE + ACC_VALUE_BUFFER_SIZE;
    uint32_t m_deviceID               = uint32_t(-1);
    uint32_t m_readBoardReplyTimeout  = 5000;       // parameter com init string
    uint32_t m_flushBoardReplyTimeout = 500;        // parameter com init string
    bool m_daisyModule                = false;      // daisy module attached or not

    // OpenBCI protocol related
    int16_t m_sampleNumber         = 0;                     // returned by the board
    uint32_t m_readState           = 0;                     // position in the sample (see doc)
    uint8_t m_sampleBufferPosition = 0;                     // position in the buffer
    std::vector<uint8_t> m_eegValueBuffers;                 // buffer for one EEG value (int24)
    std::vector<uint8_t> m_accValueBuffers;                 // buffer for one accelerometer value (int16_t)
    const static uint8_t EEG_VALUE_BUFFER_SIZE      = 3;    // int24 == 3 bytes
    const static uint8_t ACC_VALUE_BUFFER_SIZE      = 2;    // int16_t == 2 bytes
    const static uint8_t EEG_VALUE_COUNT_PER_SAMPLE = 8;    // the board send EEG values 8 by 8 (will concatenate 2 samples with daisy module)
    const static uint8_t ACC_VALUE_COUNT_PER_SAMPLE = 3;    // 3 accelerometer data per sample

    uint32_t m_missingSampleDelayBeforeReset       = 0;     // in ms - value acquired from configuration manager
    uint32_t m_droppedSampleCountBeforeReset       = 0;     // in samples - value acquired from configuration manager
    uint32_t m_droppedSampleSafetyDelayBeforeReset = 0;     // in ms - value acquired from configuration manager

    std::deque<uint32_t> m_droppedSampleTimes;

    float m_unitsToMicroVolts      = 0;     // convert from int to microvolt
    float m_unitsToRadians         = 0;     // converts from int16_t to radians
    uint32_t m_extractPosition     = 0;     // used to situate sample reading both with EEG and accelerometer data
    uint32_t m_nValidAccelerometer = 0;
    int m_lastPacketNumber         = 0;     // used to detect consecutive packets when daisy module is used

    // buffer for multibyte reading over serial connection
    std::vector<uint8_t> m_readBuffers;
    // buffer to store sample coming from OpenBCI -- filled by parseByte(), passed to handleCurrentSample()
    std::vector<float> m_sampleEEGBuffers;
    std::vector<float> m_sampleEEGBuffersDaisy;
    std::vector<float> m_sampleAccBuffers;
    std::vector<float> m_sampleAccBuffersTemp;

    // buffer to store aggregated samples
    std::vector<std::vector<float>> m_channelBuffers; // buffer to store channels & chunks
    std::vector<float> m_sampleBuffers;

    bool m_seenPacketFooter = false;    // extra precaution to sync packets

    // mechanism to call resetBoard() if no data are received
    uint32_t m_tick      = 0;           // last tick for polling
    uint32_t m_startTime = 0;           // actual time since connection

    // sample storing for device callback and serial i/o
    std::vector<float> m_callbackSamples;

private:

    typedef struct
    {
        uint32_t deviceVersion;
        uint32_t deviceChannelCount;

        uint32_t boardId;
        uint32_t daisyId;
        uint32_t mocapId;

        char boardChipset[64];
        char daisyChipset[64];
        char mocapChipset[64];
    } device_information_t;

    device_information_t m_deviceInfo;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
