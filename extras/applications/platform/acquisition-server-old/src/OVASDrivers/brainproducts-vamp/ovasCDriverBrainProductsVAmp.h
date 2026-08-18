#pragma once

#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

#include "ovasIDriver.h"
#include "ovasCHeaderBrainProductsVAmp.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <vector>
#include <deque>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverBrainProductsVAmp
 * \author Laurent Bonnet (INRIA)
 * \date 16 nov 2009
 * \erief The CDriverBrainProductsVAmp allows the acquisition server to acquire data from a USB-VAmp-16 amplifier (BrainProducts GmbH).
 *
 * The driver allows 2 different acquisition modes: normal (2kHz sampling frequency - max 16 electrodes)
 * or fast (20kHz sampling frequency, 4 monopolar or differential channels).
 * The driver uses a dedicated Header.
 *
 * \sa CHeaderBrainProductsVAmp
 */
class CDriverBrainProductsVAmp final : public IDriver
{
public:

    explicit CDriverBrainProductsVAmp(IDriverContext& ctx);
    ~CDriverBrainProductsVAmp() override;
    const char* getName() override;

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

    bool m_acquireAuxiliaryAsEEG = false;
    bool m_acquireTriggerAsEEG   = false;

    CHeaderBrainProductsVAmp m_header;

    EAcquisitionModes m_acquisitionMode = VAmp16;
    uint32_t m_nSamplePerSentBlock      = 0;
    uint32_t m_nTotalSample             = 0;
    uint32_t m_nEEGChannel              = 0;
    uint32_t m_nAuxiliaryChannel        = 0;
    uint32_t m_nTriggerChannel          = 0;

    std::vector<size_t> m_stimulationIDs;
    std::vector<size_t> m_stimulationDates;
    std::vector<size_t> m_stimulationSamples;
    CStimulationSet m_stimSet;
    uint32_t m_lastTrigger = 0;

    std::deque<std::vector<float>> m_sampleCaches;
    std::vector<float> m_samples;
    std::vector<double> m_filters;
    std::vector<float> m_resolutions;

    int64_t m_nDriftOffsetSample  = 0;
    uint32_t m_physicalSamplingHz = 0;
    uint64_t m_counterStep        = 0;
    uint64_t m_counter            = 0;

private:

    bool m_firstStart = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyUSBFirstAmpAPI
