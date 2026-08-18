#pragma once

#if defined TARGET_HAS_ThirdPartyThinkGearAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverNeuroskyMindset
 * \author Laurent Bonnet (INRIA)
 * \date 03 may 2010
 * \erief The CDriverNeuroskyMindset allows the acquisition server to acquire data from a MindSet device (Neurosky)).
 *
 * The driver opens a connection to the device through a dedicated API called ThinkGear, part of the MindSet Development Tools (MDT).
 * The MDT are available for free on the official Neurosky website (http://store.neurosky.com/products/mindset-development-tools).
 *
 */
class CDriverNeuroskyMindset final : public IDriver
{
public:

    explicit CDriverNeuroskyMindset(IDriverContext& ctx);
    ~CDriverNeuroskyMindset() override { }
    const char* getName() override { return "NeuroSky MindSet (MindSet Dev. Tools 3.2)"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:
    SettingsHelper m_settings;
    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    uint32_t m_nTotalSample        = 0;
    float* m_sample                = nullptr;

private:

    int m_connectionID          = 0;
    uint32_t m_comPort          = 0;
    bool m_eSenseChannels       = false;
    bool m_bandPowerChannels    = false;
    bool m_blinkStimulations    = false;
    bool m_blinkStrengthChannel = false;

    uint32_t m_nWarning = -1;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyThinkGearAPI
