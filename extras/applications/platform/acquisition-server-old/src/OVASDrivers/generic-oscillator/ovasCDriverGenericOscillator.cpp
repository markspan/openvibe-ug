#include "ovasCDriverGenericOscillator.h"

#include "ovasParameterHelper.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <cmath>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericOscillator::CDriverGenericOscillator(IDriverContext& ctx)
    : IDriver(ctx),
      m_paramSendPeriodicStimulations("SendPeriodicStimulations", "Send periodic stimulations", false),
      m_paramStimulationInterval("StimulationInterval", "Stimulation interval", 0.0001, 0.0001, DBL_MAX , 4),
      m_paramCheckImpedance("CheckImpedance", "Check impedance", false)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericOscillator::CDriverGenericOscillator\n";

    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(4);

    m_parameters.push_back(&m_paramSendPeriodicStimulations);
    m_parameters.push_back(&m_paramStimulationInterval);
    m_parameters.push_back(&m_paramCheckImpedance);
    
    loadParametersFromConfig(m_driverCtx.getConfigurationManager(),
                             {&m_paramSendPeriodicStimulations, &m_paramStimulationInterval},
                             "AcquisitionServer_Driver_GenericOscillator",
                             &m_header);
}

void CDriverGenericOscillator::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericOscillator::release\n";
    delete this;
}

const char* CDriverGenericOscillator::getName()
{
    return "Generic Oscillator";
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericOscillator::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericOscillator::initialize\n";
    m_header.setImpedanceCheckRequested(m_paramCheckImpedance.value());
    if (m_driverCtx.isConnected() || m_header.getChannelCount() == 0 || m_header.getSamplingFrequency() == 0) {
        return false;
    }

    for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
        m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Base);
        if (CString(m_header.getChannelName(i)) == CString("")) {
            std::stringstream ss;
            ss << "Oscillator " << (i + 1);
            m_header.setChannelName(i, ss.str().c_str());
        }
    }

    m_samples.resize(m_header.getChannelCount() * nSamplePerSentBlock);

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    m_stimSet.resize(1);
    m_stimSet.setId(0, OVTK_StimulationId_Label_00);
    m_stimSet.setDate(0, 0);
    m_stimSet.setDuration(0, 0);

    return true;
}

bool CDriverGenericOscillator::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericOscillator::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_nTotalSample = 0;
    m_startTime    = System::Time::zgetTime();
    m_nTotalStim   = 0;

    return true;
}

bool CDriverGenericOscillator::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverGenericOscillator::loop\n";

    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted()) {
        // Generate the contents we want to send next
        const uint64_t elapsed            = System::Time::zgetTime() - m_startTime;
        const uint64_t samplesNeededSoFar = CTime(elapsed).toSampleCount(m_header.getSamplingFrequency());
        if (samplesNeededSoFar <= m_nTotalSample) {
            // Too early
            return true;
        }
        const size_t remainingSamples = size_t(samplesNeededSoFar - m_nTotalSample);
        if (remainingSamples * m_header.getChannelCount() > m_samples.size()) {
            m_samples.resize(remainingSamples * m_header.getChannelCount());
        }

        // std::cout << "At " << CTime(elapsed).toSeconds() * 1000 << "ms, remaining " << remainingSamples << " samples\n";
        for (size_t i = 0; i < remainingSamples; ++i) {
            for (size_t j = 0; j < m_header.getChannelCount(); ++j) {
                const double value = sin((m_nTotalSample * (j + 1) * 12.3) / m_header.getSamplingFrequency())
                                   + sin((m_nTotalSample * (j + 1) * 4.5) / m_header.getSamplingFrequency())
                                   + sin((m_nTotalSample * (j + 1) * 67.8) / m_header.getSamplingFrequency());
                m_samples[j * remainingSamples + i] = float(value);
            }
            m_nTotalSample++;
        }

        m_callback->setSamples(&m_samples[0], remainingSamples);

        if (m_paramSendPeriodicStimulations.value() && elapsed >= m_nTotalStim * CTime(m_paramStimulationInterval.value()).time()) {
            m_callback->setStimulationSet(m_stimSet);
            m_nTotalStim++;
        }

        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    else {
        if (m_driverCtx.isImpedanceCheckRequested()) {
            for (size_t j = 0; j < m_header.getChannelCount(); ++j) {
                m_driverCtx.updateImpedance(j, 1);
            }
        }
    }

    return true;
}

bool CDriverGenericOscillator::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericOscillator::stop\n";
    return (m_driverCtx.isConnected() && m_driverCtx.isStarted());
}

bool CDriverGenericOscillator::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericOscillator::uninitialize\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) {
        return false;
    }
    m_callback = nullptr;
    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverGenericOscillator::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverGenericOscillator::configure\n";

    return saveParametersToConfig(m_driverCtx.getConfigurationManager(),
                                  {&m_paramSendPeriodicStimulations, &m_paramStimulationInterval},
                                   "AcquisitionServer_Driver_GenericOscillator",
                                   &m_header);
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
