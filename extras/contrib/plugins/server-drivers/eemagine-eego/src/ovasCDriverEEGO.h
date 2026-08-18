#pragma once

#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#include <memory>

#include "ovasIDriver.h"
#include "ovasCHeaderEEGO.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// forward declarations
namespace eemagine {
namespace sdk {
class amplifier;
class stream;
class factory;
}
}

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverEEGO
 * \author Steffen Heimes (eemagine GmbH)
 * \date Mon Oct 20 14:40:33 2014
 * \brief The CDriverEEGO allows the acquisition server to acquire data from an EEGO device.
 *
 * \sa CConfigurationEEGO
 */
class CDriverEEGO final : public IDriver
{
public:

    explicit CDriverEEGO(IDriverContext& ctx);
    ~CDriverEEGO() override;
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override;
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag /*flag*/) const
    override { return false; }  // The only currently used flag is for checking for unstability. eego is stable now.

private:

    bool loop_wrapped();

    /**
     * Check if the configuration makes sense and tries to fix it, informing the user.
     */
    bool check_configuration();
    uint64_t getRefChannelMask() const;
    uint64_t getBipChannelMask() const;
    eemagine::sdk::factory& factory();

protected:

    SettingsHelper m_settings;
    IDriverCallback* m_callback = nullptr;
    CHeaderEEGO m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    std::unique_ptr<float[]> m_sample;

    std::unique_ptr<eemagine::sdk::factory> m_pFactory;
    std::unique_ptr<eemagine::sdk::amplifier> m_pAmplifier;
    std::unique_ptr<eemagine::sdk::stream> m_pStream;

private:

    uint32_t m_samplesInBuffer = 0;
    uint32_t m_triggerChannel  = 0;
    CStimulationSet m_stimSet;  // Storing the samples over time

    // To detect flanks in the trigger signal. The last state on the trigger input.
    uint32_t m_lastTriggerValue = 0;

    // For setting store/load
    uint32_t m_iBIPRange = 0;   // [mV]
    uint32_t m_iEEGRange = 0;   // [mV]
    CString m_sEEGMask;         // String interpreted as value to be interpreted as bitfield
    CString m_sBIPMask;         // String interpreted as value to be interpreted as bitfield
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE


#endif
