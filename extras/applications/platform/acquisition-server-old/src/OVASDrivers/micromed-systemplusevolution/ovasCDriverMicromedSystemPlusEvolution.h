#pragma once

#if defined(TARGET_HAS_ThirdPartyMicromed)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows
#include <openvibe/ov_all.h>
#include <iostream>
#include <socket/IConnectionServer.h>
#include <list>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverMicromedSystemPlusEvolution
 * \author Yann Renard (INRIA)
 */
class CDriverMicromedSystemPlusEvolution final : public IDriver
{
public:
    explicit CDriverMicromedSystemPlusEvolution(IDriverContext& ctx);
    ~CDriverMicromedSystemPlusEvolution() override;
    const char* getName() override { return "Micromed SD LTM (through SystemPlus Evolution)"; }

    //virtual bool isFlagSet(const EDriverFlag flag) const { return flag==EDriverFlag::IsUnstable; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool loadDLL();

    Socket::IConnectionServer* m_ConnectionServer = nullptr;
    uint32_t m_ServerHostPort                     = 3000;
    Socket::IConnection* m_Connection             = nullptr;
    short myReceive(char* buf, long dataLen);
    bool receiveAllHeader();
    bool loadNextHeader();

protected:
    bool dropData();

    SettingsHelper m_settings;

    bool m_valid = true;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;

    uint32_t m_indexIn     = 0;
    uint32_t m_indexOut    = 0;
    uint32_t m_buffDataIdx = 0;

    uint32_t m_timeOutMilliseconds = 5000;

    char* m_structHeader                    = nullptr;
    char* m_structHeaderInfo                = nullptr;
    unsigned short int* m_structBuffData    = nullptr;
    unsigned char* m_structBuffNote         = nullptr;
    unsigned char* m_structBuffTrigger      = nullptr;
    uint64_t m_posFirstSampleOfCurrentBlock = 0;
    CStimulationSet m_stimSet;

    uint32_t m_nSamplesBlock = 0;
    uint32_t m_sizeInByte    = 0;
    uint32_t m_buffSize      = 0;

    std::list<char*> m_headers;
    std::list<char> m_tempBuffs;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
#endif
