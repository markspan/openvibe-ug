#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGenericOscillator
 * \author Yann Renard (INRIA)
 */
class CDriverGenericOscillator final : public IDriver
{
public:
    explicit CDriverGenericOscillator(IDriverContext& ctx);
    void release();
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override;
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:
    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    std::vector<float> m_samples;

    uint32_t m_nTotalSample = 0;
    uint64_t m_nTotalStim   = 0;
    uint64_t m_startTime    = 0;

    CStimulationSet m_stimSet;

private:
    bool m_sendPeriodicStimulations = false;
    double m_stimulationInterval    = 0;    // Seconds
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
