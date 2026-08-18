#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include <random>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverSimulatedDeviator
 * \brief Simulates a drifting acquisition device by changing the sample rate by a random process.
 * \author Jussi T. Lindgren (Inria)
 *
 * The driver simulates a square wave attempting to model a steady, analog process. The square wave changes
 * sign every 1 sec. This process is then sampled using a sampling rate that is changing during the 
 * recording according to the parameters given to the driver. These parameters are
 *
 * Offset   - The center sampling frequency deviation from the declared driver sampling rate. Can be negative.
 * Spread   - How big jumps the random walk takes; related to the sigma of the normal distribution
 * MaxDev   - The maximum allowed deviation in Hz from the true sampling rate + Offset
 * Pullback - How strongly the random walk is pulled towards the true sampling rate + Offset
 * Update   - How often the sampling rate is changed (in seconds)
 *
 * The MaxDev and Pullback are used to keep the stochastic process from diverging.
 *
 */
class CDriverSimulatedDeviator final : public IDriver
{
public:
    explicit CDriverSimulatedDeviator(IDriverContext& ctx);
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

    uint64_t m_nTotalSample         = 0;    // Number of samples sent by the drifting sampling process
    uint64_t m_totalSampleCountReal = 0;    // Number of samples of some imaginary, steady 'real' process, here a square wave

    uint64_t m_startTime           = 0;
    uint64_t m_lastAdjustment      = 0;
    double m_usedSamplingFrequency = 0;

private:
    OpenViBE::Parameters::ParameterBool m_sendPeriodicStimulations;
    OpenViBE::Parameters::ParameterDouble m_Offset;
    OpenViBE::Parameters::ParameterDouble m_Spread;
    OpenViBE::Parameters::ParameterDouble m_MaxDev;          
    OpenViBE::Parameters::ParameterDouble m_Pullback;        
    OpenViBE::Parameters::ParameterDouble m_Update;          
    OpenViBE::Parameters::ParameterListString m_Wavetype;
    OpenViBE::Parameters::ParameterDouble m_FreezeFrequency;  
    OpenViBE::Parameters::ParameterDouble m_FreezeDuration;   
    uint64_t m_NextFreezeTime = 0;

    std::random_device m_rd;
    std::mt19937 m_gen;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
