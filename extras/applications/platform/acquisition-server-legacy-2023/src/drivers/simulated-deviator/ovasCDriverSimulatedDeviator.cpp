#include "ovasCDriverSimulatedDeviator.h"
#include "ovasCConfigurationDriverSimulatedDeviator.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <system/ovCMath.h>

#include <cmath>
#include <algorithm>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverSimulatedDeviator::CDriverSimulatedDeviator(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_SimulatedDeviator", m_driverCtx.getConfigurationManager()), m_gen{ m_rd() }
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::CDriverSimulatedDeviator\n";

    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(3);

    m_settings.add("Header", &m_header);
    m_settings.add("SendPeriodicStimulations", &m_sendPeriodicStimulations);
    m_settings.add("Offset", &m_Offset);
    m_settings.add("Spread", &m_Spread);
    m_settings.add("MaxDev", &m_MaxDev);
    m_settings.add("Pullback", &m_Pullback);
    m_settings.add("Update", &m_Update);
    m_settings.add("Wavetype", &m_Wavetype);
    m_settings.add("FreezeFrequency", &m_FreezeFrequency);
    m_settings.add("FreezeDuration", &m_FreezeDuration);
    m_settings.load();
}

void CDriverSimulatedDeviator::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::release\n";
    delete this;
}

const char* CDriverSimulatedDeviator::getName()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::getName\n";
    return "Simulated Deviator";
}

//___________________________________________________________________//
//                                                                   //

bool CDriverSimulatedDeviator::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::initialize\n";

    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    m_header.setChannelCount(3);

    m_header.setChannelName(0, (m_Wavetype == 0 ? "SquareWave" : "SineWave"));
    m_header.setChannelName(1, "SamplingRate");
    m_header.setChannelName(2, "DriftMs");

    m_header.setChannelUnits(0, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
    m_header.setChannelUnits(1, OVTK_UNIT_Hertz, OVTK_FACTOR_Base);
    m_header.setChannelUnits(2, OVTK_UNIT_Second, OVTK_FACTOR_Milli);

    m_samples.resize(m_header.getChannelCount() * nSamplePerSentBlock);

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverSimulatedDeviator::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_nTotalSample         = 0;
    m_totalSampleCountReal = 0;
    m_startTime            = System::Time::zgetTime();
    m_lastAdjustment       = m_startTime;

    m_usedSamplingFrequency = m_header.getSamplingFrequency() + m_Offset;
    m_header.setChannelCount(3);

    return true;
}

bool CDriverSimulatedDeviator::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverSimulatedDeviator::loop\n";

    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted()) {
        // Generate the contents we want to send next
        CStimulationSet stimSet;
        if (m_sendPeriodicStimulations) {
            stimSet.resize(1);
            stimSet.setId(0, 0);
            stimSet.setDate(0, 0);
            stimSet.setDuration(0, 0);
        }

        const uint64_t now = System::Time::zgetTime();

        // Is it time to freeze?
        if (m_FreezeDuration > 0 && m_FreezeFrequency > 0) {
            if (m_NextFreezeTime > 0 && now >= m_NextFreezeTime) {
                // Simulate a freeze by setting all samples as sent up to the de-freeze point
                const uint64_t freezeDurationFixedPoint = CTime(m_FreezeDuration).time();
                const uint64_t samplesToSkip            = CTime(freezeDurationFixedPoint).toSampleCount(m_header.getSamplingFrequency());
                m_nTotalSample += samplesToSkip;
                m_totalSampleCountReal += samplesToSkip;
            }
            if (now >= m_NextFreezeTime) {
                // Compute the next time to freeze; simulate a Poisson process
                const double secondsUntilNext             = -std::log(1.0 - System::Math::random0To1()) / m_FreezeFrequency;
                const uint64_t secondsUntilNextFixedPoint = CTime(secondsUntilNext).time();

                m_NextFreezeTime = now + secondsUntilNextFixedPoint;
            }
        }

        // Drift the sampling frequency?
        if (now - m_lastAdjustment > CTime(m_Update).time()) {
            // Make the sampling frequency random walk up or down
            if (m_Spread > 0) {
                std::normal_distribution<> distro{ 0, m_Spread };
                const double jitter = distro(m_gen);
                m_usedSamplingFrequency += jitter;
            }

            // Use linear interpolation to pull the drifting sample frequency towards the center frequency
            m_usedSamplingFrequency = m_Pullback * (m_header.getSamplingFrequency() + m_Offset) + (1 - m_Pullback) * m_usedSamplingFrequency;

            // Make sure the result stays bounded
            m_usedSamplingFrequency = std::min(m_usedSamplingFrequency, m_header.getSamplingFrequency() + m_Offset + m_MaxDev);
            m_usedSamplingFrequency = std::max(m_usedSamplingFrequency, m_header.getSamplingFrequency() + m_Offset - m_MaxDev);
            m_usedSamplingFrequency = std::max(m_usedSamplingFrequency, 0.0);

            m_lastAdjustment = now;
            // std::cout << "freq " << m_usedSamplingFrequency << "\n";
        }

        const uint64_t elapsed                = now - m_startTime;
        const uint64_t samplesNeededSoFar     = uint64_t(m_usedSamplingFrequency * CTime(elapsed).toSeconds());
        const uint64_t samplesNeededSoFarReal = uint64_t(m_header.getSamplingFrequency() * CTime(elapsed).toSeconds());
        if (samplesNeededSoFar <= m_nTotalSample) {
            if (samplesNeededSoFarReal > m_totalSampleCountReal) { m_totalSampleCountReal = samplesNeededSoFarReal; }
            // Too early
            return true;
        }
        const uint32_t remainingSamples = uint32_t(samplesNeededSoFar - m_nTotalSample);
        if (remainingSamples * m_header.getChannelCount() > m_samples.size()) { m_samples.resize(remainingSamples * m_header.getChannelCount()); }

        const double driftMeasure = samplesNeededSoFarReal > samplesNeededSoFar
                                        ? - CTime(m_header.getSamplingFrequency(), samplesNeededSoFarReal - samplesNeededSoFar).toSeconds()
                                        : CTime(m_header.getSamplingFrequency(), samplesNeededSoFar - samplesNeededSoFarReal).toSeconds();

        for (uint32_t i = 0; i < remainingSamples; ++i) {
            double value;
            if (m_Wavetype == 0) {
                const uint64_t sampleTimeInSeconds = ((CTime(m_header.getSamplingFrequency(), m_totalSampleCountReal)).time() >> 32);
                value                              = (sampleTimeInSeconds % 2 == 0 ? -1 : 1);
            }
            else {
                const double pi = 3.14159265358979323846;
                value           = std::sin(2 * pi * m_totalSampleCountReal / double(m_header.getSamplingFrequency()));
            }

            m_samples[0 * remainingSamples + i] = float(value);
            m_samples[1 * remainingSamples + i] = float(m_usedSamplingFrequency);
            m_samples[2 * remainingSamples + i] = float(driftMeasure * 1000.0);

            if (samplesNeededSoFarReal >= samplesNeededSoFar) { m_totalSampleCountReal++; }
            else {
                // nop: we don't move the 'real' process forward if the sampler is in advance, it gets a duplicate sample on purpose, 
                // we assume that the hardware sampling process has not had time to change state.
                // @todo this implementation is not quite correct... if we take twice the amount of samples (512hz->1024hz), 
                // we'd expect each sample to be replicated twice but we observe that only approximately. The sinusoid 1hz stays correct nevertheless.
            }

            m_nTotalSample++;
        }

        m_callback->setSamples(&m_samples[0], remainingSamples);
        m_callback->setStimulationSet(stimSet);
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    else { if (m_driverCtx.isImpedanceCheckRequested()) { for (size_t j = 0; j < m_header.getChannelCount(); ++j) { m_driverCtx.updateImpedance(j, 1); } } }

    return true;
}

bool CDriverSimulatedDeviator::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::stop\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverSimulatedDeviator::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::uninitialize\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_callback = nullptr;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverSimulatedDeviator::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::isConfigurable\n";
    return true;
}

bool CDriverSimulatedDeviator::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverSimulatedDeviator::configure\n";

    CConfigurationDriverSimulatedDeviator config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Simulated-Deviator.ui",
                                                 m_sendPeriodicStimulations, m_Offset, m_Spread, m_MaxDev, m_Pullback, m_Update, m_Wavetype, m_FreezeFrequency,
                                                 m_FreezeDuration);

    if (config.configure(m_header)) {
        m_settings.save();
        return true;
    }

    return false;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
