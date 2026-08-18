#pragma once

#include "ovas_base.h"

#include <string>
#include <vector>
#include <list>
#include <deque>

namespace OpenViBE {
namespace AcquisitionServer {
enum class EDriftCorrectionPolicies { DriverChoice = 0, Forced, Disabled };

inline std::string toString(const EDriftCorrectionPolicies& policy)
{
    switch (policy) {
        case EDriftCorrectionPolicies::Disabled: return "Disabled";
        case EDriftCorrectionPolicies::DriverChoice: return "Driver Choice";
        case EDriftCorrectionPolicies::Forced: return "Forced";
    }
}

/*
 * \class CDriftCorrection
 * \author Jussi T. Lindgren / Inria
 * 
 * \brief A refactoring of the old drift correction code that was originally mixed into the acquisition server main code.
 *
 */
class CDriftCorrection final
{
public:
    explicit CDriftCorrection(const Kernel::IKernelContext& ctx);
    ~CDriftCorrection() { }

    // To start a new drift estimation and stop it.
    bool start(uint32_t sampling, uint64_t startTime);
    void stop();

    // Estimate the drift
    // 
    // \param newSamples [in] : How many samples the driver returned
    bool estimateDrift(uint64_t newSamples);

    // Request a drift correction
    //
    // \param i64correction [in] : the number of samples to correct
    // \param totalSamples [out] : Number of total samples after correction for drift
    // \param pendingBuffers [in/out] : The sample buffer to be corrected
    // \param pendingStimSet [in/out] : The stimulation set to be realigned
    // \param paddingBuffer[in] : The sample to repeatedly add if correction > 0
    bool correctDrift(int64_t correction, size_t& totalSamples, std::deque<std::vector<float>>& pendingBuffers,
                      CStimulationSet& pendingStimSet, const std::vector<float>& paddingBuffer);

    // Status functions
    bool isActive() const { return m_isActive; }

    // Prints various statistics but only if drift tolerance was exceeded
    void printStats() const;

    // Result getters
    // current drift, in ms.
    double getDriftMs() const { return (m_sampling == 0) ? 0 : m_driftEstimate * 1000.0 / m_sampling; }
    // maximum positive drift observed, in ms. (driver gave too many samples)
    double getDriftTooFastMax() const { return (m_sampling == 0) ? 0 : m_driftEstimateTooFastMax * 1000.0 / m_sampling; }
    // maximum negative drift observed, in ms. (driver gave too few samples)
    double getDriftTooSlowMax() const { return (m_sampling == 0) ? 0 : m_driftEstimateTooSlowMax * 1000. / m_sampling; }
    // current drift, in samples
    int64_t getDriftSampleCount() const { return int64_t(m_driftEstimate); }
    int64_t getDriftToleranceSampleCount() const { return m_nDriftToleranceSample; }
    // number of samples to correct drift with
    int64_t getSuggestedDriftCorrectionSampleCount() const;

    // Parameter getters and setters
    int64_t getInnerLatencySampleCount() const { return m_nInnerLatencySample; }
    void setInnerLatencySampleCount(const int64_t count) { m_nInnerLatencySample = count; }

    EDriftCorrectionPolicies getDriftCorrectionPolicy() const { return m_driftCorrectionPolicy; }
    CString getDriftCorrectionPolicyStr() const { return toString(m_driftCorrectionPolicy).c_str(); }
    void setDriftCorrectionPolicy(const EDriftCorrectionPolicies policy) { m_driftCorrectionPolicy = policy; }

    uint64_t getDriftToleranceDurationMs() const { return m_driftToleranceDurationMs; }
    bool setDriftToleranceDurationMs(uint64_t ms);

    uint64_t getJitterEstimationCountForDrift() const { return m_nJitterEstimationForDrift; }
    bool setJitterEstimationCountForDrift(uint64_t count);

protected:
    void reset();

    // Computes jitter in fractional samples
    double computeJitter(uint64_t currentTime) const;

    const Kernel::IKernelContext& m_kernelCtx;

    // State of the drift correction
    bool m_isInitialized           = false;
    bool m_isStarted               = false;
    bool m_isActive                = false;
    bool m_initialSkipPeriodPassed = false;

    // Parameters
    EDriftCorrectionPolicies m_driftCorrectionPolicy;
    uint64_t m_driftToleranceDurationMs  = 0;
    int64_t m_nDriftToleranceSample      = 0;
    int64_t m_nInnerLatencySample        = 0;
    uint64_t m_nJitterEstimationForDrift = 0;
    uint32_t m_sampling                  = 0;
    uint64_t m_initialSkipPeriod         = 0;

    // Results
    double m_receivedSampleCount     = 0;
    double m_correctedSampleCount    = 0;
    double m_driftEstimate           = 0;   // In subsample accuracy, e.g. 1.2 samples.
    double m_driftEstimateTooFastMax = 0;   // maximum over time
    double m_driftEstimateTooSlowMax = 0;   // minimum over time

    // Stats
    int64_t m_nDriftCorrectionSampleAdded   = 0;
    int64_t m_nDriftCorrectionSampleRemoved = 0;
    uint64_t m_nDriftCorrection             = 0;

    // Timekeeping
    uint64_t m_startTime          = 0;
    uint64_t m_lastEstimationTime = 0;

    // Jitter estimation buffer. Each entry is the difference between the expected number of 
    // samples and the received number of samples per each call to estimateDrift(). The buffer has subsample accuracy to avoid rounding errors.
    // -> The average of the buffer items is the current aggregated drift estimate (in samples), convertable to ms by getDrift().
    std::list<double> m_jittersEstimate;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
