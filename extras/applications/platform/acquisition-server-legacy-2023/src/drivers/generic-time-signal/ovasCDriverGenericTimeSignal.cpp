#include "ovasCDriverGenericTimeSignal.h"
#include "../ovasCConfigurationBuilder.h"

#include "ovasCConfigurationGenericTimeSignal.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericTimeSignal::CDriverGenericTimeSignal(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_GenericTimeSignal", m_driverCtx.getConfigurationManager())
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::CDriverGenericTimeSignal\n";

    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(1);
    m_header.setChannelName(0, "Time(s)");
    m_header.setChannelUnits(0, OVTK_UNIT_Second, OVTK_FACTOR_Base);

    m_settings.add("Header", &m_header);
    m_settings.load();
}

void CDriverGenericTimeSignal::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::release\n";
    delete this;
}

const char* CDriverGenericTimeSignal::getName()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::getName\n";
    return "Generic Time Signal";
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericTimeSignal::initialize(const uint32_t /*nSamplePerSentBlock*/, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::initialize\n";
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    m_callback = &callback;
    return true;
}

bool CDriverGenericTimeSignal::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::start\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_nTotalSample = 0;
    m_startTime    = System::Time::zgetTime();
    return true;
}

bool CDriverGenericTimeSignal::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverGenericTimeSignal::loop\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return true; }

    // Find out how many samples to send; note that we always just send 1 at a time with this driver
    const uint64_t now         = System::Time::zgetTime();
    const uint64_t elapsed     = now - m_startTime;
    const uint64_t neededSoFar = (m_header.getSamplingFrequency() * elapsed) >> 32;
    if (neededSoFar <= m_nTotalSample) { return true; } // Too early

    const float timeNow = float(CTime(now).toSeconds());
    m_callback->setSamples(&timeNow, 1);
    m_nTotalSample++;
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    return true;
}

bool CDriverGenericTimeSignal::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::stop\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverGenericTimeSignal::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::uninitialize\n";
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_callback = nullptr;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericTimeSignal::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::isConfigurable\n";
    return true;
}

bool CDriverGenericTimeSignal::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverGenericTimeSignal::configure\n";
    CConfigurationGenericTimeSignal config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-TimeSignal.ui");
    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
