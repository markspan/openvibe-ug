#include "ovasCDriverGenericSawTooth.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericSawTooth::CDriverGenericSawTooth(IDriverContext& ctx)
    : IDriver(ctx)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::CDriverGenericSawTooth\n";

    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(1);
    m_header.setChannelName(0, "Sawtooth");
    m_header.setChannelUnits(0, OVTK_UNIT_Volts, OVTK_FACTOR_Base);
}

void CDriverGenericSawTooth::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::release\n";
    delete this;
}

const char* CDriverGenericSawTooth::getName()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::getName\n";
    return "Generic Saw Tooth";
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericSawTooth::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::initialize\n";
    if (m_driverCtx.isConnected() || m_header.getChannelCount() == 0 || m_header.getSamplingFrequency() == 0) {
        return false;
    }

    m_samples.resize(m_header.getChannelCount() * nSamplePerSentBlock);
    m_callback          = &callback;
    m_externalBlockSize = nSamplePerSentBlock;

    return true;
}

bool CDriverGenericSawTooth::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_nTotalSample = 0;
    m_startTime    = System::Time::zgetTime();

    return true;
}

bool CDriverGenericSawTooth::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverGenericSawTooth::loop\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return true; }

    // Find out how many samples to send
    const uint64_t elapsed            = System::Time::zgetTime() - m_startTime;
    const uint64_t samplesNeededSoFar = CTime(elapsed).toSampleCount(m_header.getSamplingFrequency());
    if (samplesNeededSoFar <= m_nTotalSample) {
        // Too early
        return true;
    }
    const size_t remainingSamples = size_t(samplesNeededSoFar - m_nTotalSample);
    if (remainingSamples * m_header.getChannelCount() > m_samples.size()) { m_samples.resize(remainingSamples * m_header.getChannelCount()); }

    // Generate the data
    // The result should be a linear ramp between [0,1] for each block sent *out* by the acquisition server
    for (size_t i = 0; i < remainingSamples; ++i) {
        for (size_t j = 0; j < m_header.getChannelCount(); ++j) {
            m_samples[j * remainingSamples + i] = float(m_nTotalSample % m_externalBlockSize) / float(m_externalBlockSize - 1);
        }
        m_nTotalSample++;
    }

    m_callback->setSamples(&m_samples[0], remainingSamples);
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    return true;
}

bool CDriverGenericSawTooth::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::stop\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverGenericSawTooth::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::uninitialize\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_callback = nullptr;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericSawTooth::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::isConfigurable\n";
    return false;
}

bool CDriverGenericSawTooth::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericSawTooth::configure\n";
    return false;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
