/*
 * \author Christoph Veigl, Yann Renard
 *
 * \copyright AGPL3
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

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverOpenEEGModularEEG
 * \author Christoph Veigl, Yann Renard
 */
class CDriverOpenEEGModularEEG final : public IDriver
{
public:

    explicit CDriverOpenEEGModularEEG(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "OpenEEG Modular EEG P2"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:

    // void logPacket();
    bool parseByteP2(uint8_t actbyte);

    bool initTTY(FD_TYPE* descriptor, uint32_t ttyNumber);
    int readPacketFromTTY(FD_TYPE fileDesc);
    static void closeTTY(FD_TYPE fileDesc);

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nChannel = 6;
    uint32_t m_deviceID = uint32_t(-1);
    float* m_sample     = nullptr;

    FD_TYPE m_fileDesc;
    uint16_t m_readState       = 0;
    uint16_t m_extractPos      = 0;
    uint8_t m_packetNumber     = 0;
    uint8_t m_lastPacketNumber = 0;
    uint16_t m_switches        = 0;

    std::vector<std::vector<int>> m_channelBuffers;
    std::vector<int> m_channelBuffers2;

    CString m_ttyName;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
