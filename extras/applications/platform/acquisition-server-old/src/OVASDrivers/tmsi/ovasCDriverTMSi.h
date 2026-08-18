///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include "ovas_base.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows

// Get Signal info


namespace OpenViBE {
namespace AcquisitionServer {
class CTMSiAccess;

/**
 * \class CDriverTMSi
 * \author Mensia Technologies
 */
class CDriverTMSi final : public IDriver
{
public:
    explicit CDriverTMSi(IDriverContext& ctx);
    ~CDriverTMSi() override { this->uninitialize(); }
    void release() { delete this; }
    const char* getName() override { return "TMSi amplifiers"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    // saved parameters
    CString m_sConnectionProtocol;
    CString m_deviceID;
    bool m_bCommonAverageReference = false;
    uint64_t m_activeEEGChannels   = 0;
    CString m_sActiveAdditionalChannels;
    uint64_t m_impedanceLimit = 0;

    CTMSiAccess* m_pTMSiAccess = nullptr;
    CHeader m_header;

protected:
    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;
    std::vector<double> m_impedances;

    bool m_valid = false;

    uint32_t m_totalSampleReceived = 0;
    CStimulationSet m_stimSet;

private:
    bool m_bIgnoreImpedanceCheck = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
