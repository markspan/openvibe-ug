#pragma once

#include "ovasIDriver.h"


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

    bool configure() override;

protected:
    IDriverCallback* m_callback = nullptr;

    uint32_t m_nSamplePerSentBlock = 0;
    std::vector<float> m_samples;

    uint32_t m_nTotalSample = 0;
    uint64_t m_nTotalStim   = 0;
    uint64_t m_startTime    = 0;

    CStimulationSet m_stimSet;

private:
    OpenViBE::Parameters::ParameterBool m_paramSendPeriodicStimulations;
    OpenViBE::Parameters::ParameterDouble m_paramStimulationInterval;
    OpenViBE::Parameters::ParameterBool m_paramCheckImpedance;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
