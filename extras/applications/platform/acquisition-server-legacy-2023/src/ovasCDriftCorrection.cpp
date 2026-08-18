// @note in this code, conversion in time or in second are sometimes used simply to convert between floating and fixed point even if the units are not seconds.
#include "ovasCDriftCorrection.h"

#include <toolkit/ovtk_all.h>

#include <ovp_global_defines.h>

#include <system/ovCTime.h>
#include <cmath> // std::abs
#include <algorithm> // std::min, std::max
#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

CDriftCorrection::CDriftCorrection(const Kernel::IKernelContext& ctx)
    : m_kernelCtx(ctx), m_driftCorrectionPolicy(EDriftCorrectionPolicies::DriverChoice)
{
    const std::string policy = m_kernelCtx.getConfigurationManager().expand("${AcquisitionServer_DriftCorrectionPolicy}").toASCIIString();
    if (policy == "Forced") { this->setDriftCorrectionPolicy(EDriftCorrectionPolicies::Forced); }
    else if (policy == "Disabled") { this->setDriftCorrectionPolicy(EDriftCorrectionPolicies::Disabled); }
    else { this->setDriftCorrectionPolicy(EDriftCorrectionPolicies::DriverChoice); }

    this->setDriftToleranceDurationMs(m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriftToleranceDuration}", 5));
    this->setJitterEstimationCountForDrift(m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_JitterEstimationCountForDrift}", 128));

    m_initialSkipPeriod = m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriftInitialSkipPeriodMs}", 0);
    m_initialSkipPeriod = (m_initialSkipPeriod << 32) / 1000; // ms to fixed point sec

    reset();
}

bool CDriftCorrection::start(const uint32_t sampling, const uint64_t startTime)
{
    if (sampling == 0) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Drift correction doesn't support sampling rate of 0.\n";
        return false;
    }

    reset();

    m_sampling              = sampling;
    m_nDriftToleranceSample = (m_driftToleranceDurationMs * m_sampling) / 1000;

    m_startTime = startTime + m_initialSkipPeriod;

    m_lastEstimationTime = startTime;

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Drift correction is set to ";
    switch (m_driftCorrectionPolicy) {
        default: case EDriftCorrectionPolicies::DriverChoice: m_kernelCtx.getLogManager() << "DriverChoice\n";
            break;
        case EDriftCorrectionPolicies::Forced: m_kernelCtx.getLogManager() << "Forced\n";
            break;
        case EDriftCorrectionPolicies::Disabled: m_kernelCtx.getLogManager() << "Disabled\n";
            break;
    }

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Driver monitoring drift estimation on " << m_nJitterEstimationForDrift << " jitter measures\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Driver monitoring drift tolerance set to " << m_driftToleranceDurationMs << " milliseconds - eq "
            << m_nDriftToleranceSample << " samples\n";

    m_isStarted = true;

    return true;
}

void CDriftCorrection::stop()
{
    m_isStarted = false;
    m_isActive  = false;
}

void CDriftCorrection::reset()
{
    m_jittersEstimate.clear();

    m_receivedSampleCount  = 0;
    m_correctedSampleCount = 0;
    m_nInnerLatencySample  = 0;

    m_driftEstimate                 = 0;
    m_driftEstimateTooSlowMax       = 0;
    m_driftEstimateTooFastMax       = 0;
    m_nDriftCorrectionSampleAdded   = 0;
    m_nDriftCorrectionSampleRemoved = 0;

    m_nDriftCorrection = 0;

    // Don't know the sampling rate yet, so cannot put a good value. Put something.
    m_nDriftToleranceSample = 1;

    m_isActive                = false;
    m_initialSkipPeriodPassed = (m_initialSkipPeriod <= 0 ? true : false);
}


//___________________________________________________________________//
//                                                                   //

void CDriftCorrection::printStats() const
{
    if (!m_isStarted) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Drift correction is stopped, no statistics were collected.\n";
        return;
    }

    const uint64_t elapsedTime  = m_lastEstimationTime - m_startTime;
    const double elapsedTimeSec = CTime(elapsedTime).toSeconds();

    const uint64_t theoreticalSampleCountFixedPoint = m_sampling * elapsedTime;
    const double theoreticalSampleCount             = CTime(theoreticalSampleCountFixedPoint).toSeconds();

    const double addedRatio   = (theoreticalSampleCount != 0 ? (m_nDriftCorrectionSampleAdded / theoreticalSampleCount) : 0);
    const double removedRatio = (theoreticalSampleCount != 0 ? (m_nDriftCorrectionSampleRemoved / theoreticalSampleCount) : 0);

    const uint64_t driftToleranceDurationMs = getDriftToleranceDurationMs();
    const double driftRatio                 = getDriftMs() / double(driftToleranceDurationMs);
    const double driftRatioTooFastMax       = getDriftTooFastMax() / double(driftToleranceDurationMs);
    const double driftRatioTooSlowMax       = getDriftTooSlowMax() / double(driftToleranceDurationMs);

    const double estimatedSamplingRate = m_receivedSampleCount / elapsedTimeSec;
    const double deviationPercent      = 100.0 * (estimatedSamplingRate / double(m_sampling));


    if (driftRatioTooFastMax > 1.0 || driftRatioTooSlowMax > 1.0 || std::abs(driftRatio) > 1.0) {
        // Drift tolerance was exceeded, print some stats

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Stats after " << elapsedTimeSec << " second session of " << m_sampling <<
                "hz sampling (declared rate),\n";

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Estimate : Driver samples at " << std::round(estimatedSamplingRate * 10.0) / 10.0
                << "hz (" << std::round(deviationPercent * 10.0) / 10.0 << "% of declared)\n";

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Received : " << m_receivedSampleCount << " samples\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Expected : " << theoreticalSampleCount << " samples\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Returned : " << m_correctedSampleCount << " samples "
                << (m_driftCorrectionPolicy == EDriftCorrectionPolicies::Disabled ? "(drift correction disabled)" : "(after drift correction)") << "\n";

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Added    : " << m_nDriftCorrectionSampleAdded << " samples (" << addedRatio << "%)\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Removed  : " << m_nDriftCorrectionSampleRemoved << " samples (" << removedRatio << "%)\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "  Operated : " << m_nDriftCorrection << " times (interventions)\n";

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Estimated drift (tolerance = " << driftToleranceDurationMs << "ms),\n";

        m_kernelCtx.getLogManager() << (driftRatioTooSlowMax > 1.0 ? Kernel::LogLevel_Warning : Kernel::LogLevel_Info)
                << "  Slow peak  : " << -m_driftEstimateTooSlowMax << " samples (" << getDriftTooSlowMax() << "ms late, " << 100 * driftRatioTooSlowMax <<
                "% of tol.)\n";

        m_kernelCtx.getLogManager() << (driftRatioTooFastMax > 1.0 ? Kernel::LogLevel_Warning : Kernel::LogLevel_Info)
                << "  Fast peak  : " << m_driftEstimateTooFastMax << " samples (" << getDriftTooFastMax() << "ms early, " << 100 * driftRatioTooFastMax <<
                "% of tol.)\n";

        m_kernelCtx.getLogManager() << (std::abs(driftRatio) > 1.0 ? Kernel::LogLevel_Warning : Kernel::LogLevel_Info)
                << "  Last estim : " << m_driftEstimate << " samples (" << getDriftMs() << "ms, " << 100 * driftRatio << "% of tol., "
                << std::round(100.0 * (getDriftMs() / 1000.0) / elapsedTimeSec * 10.0) / 10.0 << "% of session length)"
                << (m_driftCorrectionPolicy == EDriftCorrectionPolicies::Disabled ? "" : ", after corr.")
                << "\n";

        const double remainingDriftCount = m_correctedSampleCount - theoreticalSampleCount;
        const double remainingDriftMs    = 1000.0 * remainingDriftCount / double(m_sampling);
        m_kernelCtx.getLogManager() << (std::abs(remainingDriftMs) > driftToleranceDurationMs ? Kernel::LogLevel_Warning : Kernel::LogLevel_Info)
                << "  Remaining  : " << remainingDriftCount << " samples (" << remainingDriftMs << "ms, " << 100 * remainingDriftMs / driftToleranceDurationMs
                << "% of tol., "
                << std::round(100.0 * (remainingDriftMs / 1000.0) / elapsedTimeSec * 10.0) / 10.0 << "% of session length)"
                << (m_driftCorrectionPolicy == EDriftCorrectionPolicies::Disabled ? "" : ", after corr.")
                << "\n";

        if (m_driftCorrectionPolicy == EDriftCorrectionPolicies::DriverChoice && m_nDriftCorrectionSampleAdded == 0 && m_nDriftCorrectionSampleRemoved == 0) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning <<
                    "  The driver did not try to correct the drift. This may be a feature of the driver.\n";
        }
    }
}


//___________________________________________________________________//
//                                                                   //

double CDriftCorrection::computeJitter(const uint64_t currentTime) const
{
    // Compute the jitter. To get a bit cleaner code, instead of basing jitter estimate
    // on difference of estimated sample counts, we compute the diffs in the elapsed 
    // and the expected time based on the amount of samples received.
    const uint64_t expectedTime = m_startTime + CTime(m_correctedSampleCount).time() / uint64_t(m_sampling);

    double timeDiff; // time in seconds that our expectation differs from the measured clock
    if (expectedTime >= currentTime) {
        // The driver is early
        timeDiff = CTime(expectedTime - currentTime).toSeconds();
    }
    else {
        // The driver is late
        timeDiff = -CTime(currentTime - expectedTime).toSeconds();
    }

    // Jitter in fractional samples
    return timeDiff * m_sampling + double(m_nInnerLatencySample);
}


bool CDriftCorrection::estimateDrift(const uint64_t newSamples)
{
    if (!m_isStarted) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Drift correction: estimateDrift() called before start()\n";
        return false;
    }

    const uint64_t currentTime = System::Time::zgetTime();
    if (currentTime < m_startTime) {
        // We can ignore some sets of samples for the driver to stabilize. For example, 
        // a delay in delivering the first set would cause a permanent drift (offset) unless corrected. 
        // Yet this offset would be totally harmless to any client connecting to AS after first 
        // set of samples have been received, and if drift correction is disabled, it would 
        // stay there in the measure. 
        //
        // With conf token AcquisitionServer_DriftInitialSkipPeriodMs set to 0 no sets will be skipped.

        return true;
    }
    if (!m_initialSkipPeriodPassed) {
        // The next call will be the first estimation
        m_initialSkipPeriodPassed = true;
        m_startTime               = currentTime;
        m_lastEstimationTime      = currentTime;
        return true;
    }

    m_receivedSampleCount += newSamples;        // How many samples have arrived from the driver
    m_correctedSampleCount += newSamples;       // The "corrected" amount of samples

    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Drift measured at " << CTime(currentTime - m_startTime).toSeconds() * 1000 << "ms.\n";

    const double jitter = computeJitter(currentTime);

    m_jittersEstimate.push_back(jitter);
    if (m_jittersEstimate.size() > m_nJitterEstimationForDrift) { m_jittersEstimate.pop_front(); }

    // Estimate the drift after we have a full buffer of jitter estimates
    if (m_jittersEstimate.size() == m_nJitterEstimationForDrift) {
        double newDriftEstimate = 0;

        for (auto j = m_jittersEstimate.begin(); j != m_jittersEstimate.end(); ++j) { newDriftEstimate += *j; }

        m_driftEstimate = newDriftEstimate / m_nJitterEstimationForDrift;

        if (m_driftEstimate > 0) { m_driftEstimateTooFastMax = std::max<double>(m_driftEstimateTooFastMax, m_driftEstimate); }
        else { m_driftEstimateTooSlowMax = std::max<double>(m_driftEstimateTooSlowMax, -m_driftEstimate); }

        if (std::abs(m_driftEstimate) > this->getDriftToleranceSampleCount()) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug <<
                    "At " << CTime(currentTime).toSeconds() * 1000 << "ms,"
                    << " Acq mon [drift:" << getDriftMs() << "][jitter:" << jitter << "] samples,"
                    << " dsc " << getDriftSampleCount()
                    << " (inner lat. samples " << m_nInnerLatencySample << ")\n";
        }
    }

    m_lastEstimationTime = currentTime;

    return true;
}


bool CDriftCorrection::correctDrift(const int64_t correction, size_t& totalSamples, std::deque<std::vector<float>>& pendingBuffers,
                                    CStimulationSet& pendingStimSet, const std::vector<float>& paddingBuffer)
{
    if (!m_isStarted) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Drift correction: correctDrift() called before start()\n";
        return false;
    }

    if (m_driftCorrectionPolicy == EDriftCorrectionPolicies::Disabled) {
        // Not an error, we just don't correct
        return false;
    }

    m_isActive = true;

    if (correction == 0) { return true; }

    if (totalSamples != uint64_t(m_correctedSampleCount)) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Server and drift correction class disagree on the number of samples\n";
    }

    const uint64_t elapsedTime  = System::Time::zgetTime() - m_startTime;
    const double elapsedTimeSec = CTime(elapsedTime).toSeconds();

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "At time " << elapsedTimeSec << "s : Correcting drift by " << correction << " samples\n";

    if (correction > 0) {
        for (int64_t i = 0; i < correction; ++i) { pendingBuffers.push_back(paddingBuffer); }

        const uint64_t timeOfIncorrect     = CTime(m_correctedSampleCount - 1).time() / uint64_t(m_sampling);
        const uint64_t durationOfIncorrect = CTime(m_sampling, correction).time();
        const uint64_t timeOfCorrect       = CTime(m_correctedSampleCount - 1 + correction).time() / uint64_t(m_sampling);
        pendingStimSet.push_back(OVTK_StimulationId_AddedSamplesBegin, timeOfIncorrect, durationOfIncorrect);
        pendingStimSet.push_back(OVTK_StimulationId_AddedSamplesEnd, timeOfCorrect, 0);

        m_driftEstimate += correction;

        m_correctedSampleCount += correction;
        m_nDriftCorrectionSampleAdded += correction;
        m_nDriftCorrection++;
    }
    else if (correction < 0) {
        const size_t samplesToRemove = std::min<size_t>(size_t(-correction), pendingBuffers.size());

        pendingBuffers.erase(pendingBuffers.begin() + pendingBuffers.size() - int(samplesToRemove), pendingBuffers.begin() + pendingBuffers.size());

        const size_t lastSampleDate = CTime(m_correctedSampleCount - samplesToRemove).time() / size_t(m_sampling);
        for (size_t i = 0; i < pendingStimSet.size(); ++i) { if (pendingStimSet.getDate(i) > lastSampleDate) { pendingStimSet.setDate(i, lastSampleDate); } }

        pendingStimSet.push_back(OVTK_StimulationId_RemovedSamples, lastSampleDate, 0);

        m_driftEstimate -= samplesToRemove;

        m_correctedSampleCount -= samplesToRemove;
        m_nDriftCorrectionSampleRemoved += samplesToRemove;
        m_nDriftCorrection++;
    }

    // correct the jitter estimate to match the correction we made. For example, if we had
    // a jitter estimate of [-1,-1,-1,...] and correct the drift by adding 1 sample, the new
    // estimate should become [0,0,0,0,...] with relation to the adjustment. This changes jitter estimates
    // in the past. The other alternative would be to reset the estimate. Adjusting 
    // might keep some detail the reset would lose.
    for (auto j = m_jittersEstimate.begin(); j != m_jittersEstimate.end(); ++j) { (*j) += correction; }

    totalSamples = uint64_t(m_correctedSampleCount);

    return true;
}

// Note that we cannot do actual correction with subsample accuracy, so here we truncate the drift estimate to integer in getDriftSampleCount().
int64_t CDriftCorrection::getSuggestedDriftCorrectionSampleCount() const
{
    const double toleranceMs    = double(this->getDriftToleranceDurationMs());
    const double currentDriftMs = this->getDriftMs();

    if (std::abs(currentDriftMs) > toleranceMs) {
        // The correction is always to the opposite direction of the drift
        return -this->getDriftSampleCount();
    }

    return 0;
}

// ____________________________________________________________________________
//

bool CDriftCorrection::setDriftToleranceDurationMs(const uint64_t ms)
{
    if (ms == 0) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Minimum accepted drift tolerance limit is 1ms\n";
        m_driftToleranceDurationMs = 1;
        return true;
    }

    m_driftToleranceDurationMs = ms;
    return true;
}

bool CDriftCorrection::setJitterEstimationCountForDrift(const uint64_t count)
{
    m_nJitterEstimationForDrift = count;
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
