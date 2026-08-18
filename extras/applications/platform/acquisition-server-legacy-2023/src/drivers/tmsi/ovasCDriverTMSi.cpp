///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#include "ovasCDriverTMSi.h"
#include "ovasCConfigurationTMSi.h"
#include "ovasCTMSiAccess.h"

#if defined TARGET_HAS_ThirdPartyTMSi

#include <toolkit/ovtk_all.h>

#include <iostream>
#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

// Public driver functions

CDriverTMSi::CDriverTMSi(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_TMSi", m_driverCtx.getConfigurationManager())
{
    // default parameters, will be overriden by saved settings
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(32);
    m_sConnectionProtocol       = "USB";
    m_deviceID                  = "";
    m_bCommonAverageReference   = true;
    m_activeEEGChannels         = 0;
    m_sActiveAdditionalChannels = ";";
    m_impedanceLimit            = 1;

    m_settings.add("Header", &m_header);
    m_settings.add("ConnectionProtocol", &m_sConnectionProtocol);
    m_settings.add("DeviceIdentifier", &m_deviceID);
    //  m_settings.add("CommonAverageReference", &m_bCommonAverageReference);
    m_settings.add("ActiveEEGChannels", &m_activeEEGChannels);
    m_settings.add("ActiveAdditionalChannels", &m_sActiveAdditionalChannels);
    m_settings.add("ImpedanceLimit", &m_impedanceLimit);
    m_settings.load();

    m_valid = true;

    if (!m_valid) { return; }

    m_pTMSiAccess = new CTMSiAccess(m_driverCtx);
}


bool CDriverTMSi::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (!m_valid || m_driverCtx.isConnected()) { return false; }

    // Show a window prompting the user to wait, this window is automatically closed when the Configuration object is destroyed
    CConfigurationTMSi config(Directories::getDataDir() + "/applications/acquisition-server/interface-TMSi.ui", this);
    config.showWaitWindow();


    if (m_deviceID == CString("")) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "No TMSi device selected" << "\n";
        return false;
    }
    if (m_header.getSamplingFrequency() == 0) { m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "No Sampling Frequency selected" << "\n"; }

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    m_pTMSiAccess->initializeTMSiLibrary(m_sConnectionProtocol.toASCIIString());
    m_pTMSiAccess->openFrontEnd(m_deviceID.toASCIIString());


    // by default CAR (called Common Mode Rejection in TMSi documents) is enabled, one can disable it by token
    m_bCommonAverageReference = !(m_driverCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_TMSI_DisableCommonModeRejection}", false));

    if (!m_pTMSiAccess->setCommonModeRejection(m_bCommonAverageReference)) {
        m_pTMSiAccess->closeFrontEnd();
        return false;
    }

    if (!m_pTMSiAccess->runDiagnostics()) {
        m_pTMSiAccess->closeFrontEnd();
        return false;
    }
    /*
        // Get Time
        SYSTEMTIME time;
        if(m_fpGetRtcTime( m_libraryHandle, &time ))
        {
            char str[255];
            sprintf(str, "Current Device Time Weekday %d on %d-%d-%d Time %d:%d:%d", time.wDayOfWeek, time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond );
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) " << str << "\n";
        }
        else
        {
            int code = m_fpGetErrorCode(m_libraryHandle);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) " << "Error getting time from device, errorcode = " << code << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, code) << "\"" << "\n";
            return false;
        }
    
        // Set Time
        GetLocalTime(&time);
        if(m_fpSetRtcTime(m_libraryHandle, &time ))
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) " << "Time set" << "\n";
        }
        else
        {
            int code = m_fpGetErrorCode(m_libraryHandle);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(TMSi) " << "Error setting time on device, errorcode = " << code << ", message=\"" << m_fpGetErrorCodeMessage(m_libraryHandle, code) << "\"" << "\n";
            return false;
        }
    */

    // Get Signal Format
    if (m_pTMSiAccess->calculateSignalFormat(m_deviceID)) { m_pTMSiAccess->printSignalFormat(); }
    else {
        m_pTMSiAccess->closeFrontEnd();
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) " << "Number of Channels on [" << m_deviceID << "]" << " = " << uint64_t(
        m_pTMSiAccess->getActualChannelCount()) << "\n";

    if (!m_pTMSiAccess->setSignalBuffer(m_header.getSamplingFrequency() * 1000, m_header.getSamplingFrequency() * 10)) {
        m_pTMSiAccess->closeFrontEnd();
        return false;
    }

    m_pTMSiAccess->setActiveChannels(&m_header, m_sActiveAdditionalChannels);

    m_sample = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    if (!m_sample) {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }

    if (!m_pTMSiAccess->getConnectionProperties()) {
        m_pTMSiAccess->closeFrontEnd();
        return false;
    }

    if (m_driverCtx.isImpedanceCheckRequested()) {
        // Acquisition on the device must be started in order to set the acquiring mode to impedance
        if (!m_pTMSiAccess->startAcquisition()) { return false; }
        if (!m_pTMSiAccess->setSignalMeasuringModeToImpedanceCheck(int(m_impedanceLimit))) {
            m_pTMSiAccess->stopAcquisition();
            m_bIgnoreImpedanceCheck = true;
        }
        else {
            m_impedances.resize(m_header.getChannelCount());
            m_bIgnoreImpedanceCheck = false;
        }
    }

    // @todo modify the API to provide this info (depends on the device)
    for (size_t i = 0; i < m_header.getChannelCount(); ++i) { m_header.setChannelUnits(i, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base); }

    return true;
}

bool CDriverTMSi::start()
{
    if (!m_valid || !m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_driverCtx.isImpedanceCheckRequested()) {
        m_pTMSiAccess->setSignalMeasuringModeToNormal();
        m_pTMSiAccess->stopAcquisition();
    }
    // sometimes the driver might refuse to start, some sampling frequencies can be set but not used for example
    if (!m_pTMSiAccess->startAcquisition()) { return false; }

    // TODO_JL Do impedance check when applicable

    m_totalSampleReceived = 0;
    return true;
}

bool CDriverTMSi::loop()
{
    if (!m_valid || !m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted()) {
        const int bytesReceived = m_pTMSiAccess->getSamples(m_sample, m_callback, m_nSamplePerSentBlock, m_header.getSamplingFrequency());

        if (bytesReceived >= 0) { m_totalSampleReceived += bytesReceived; }
        else { return false; }
    }
    else {
        if (m_driverCtx.isImpedanceCheckRequested() && !m_bIgnoreImpedanceCheck) {
            if (!m_pTMSiAccess->getImpedanceValues(&m_impedances)) { return false; }

            for (size_t i = 0; i < m_header.getChannelCount(); ++i) { m_driverCtx.updateImpedance(i, m_impedances[i]); }
        }
    }

    return true;
}

bool CDriverTMSi::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) " << ">Stop TMSI\n";

    m_pTMSiAccess->stopAcquisition();

    m_totalSampleReceived = 0;
    return true;
}

bool CDriverTMSi::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "(TMSi) " << ">Uninit TMSI\n";

    m_pTMSiAccess->stopAcquisition();
    m_pTMSiAccess->freeSignalFormat();

    if (!m_pTMSiAccess->closeFrontEnd()) { return false; }

    return true;
}

bool CDriverTMSi::configure()
{
    CConfigurationTMSi config(Directories::getDataDir() + "/applications/acquisition-server/interface-TMSi.ui", this);

    const bool result = config.configure(m_header);
    if (!result) { return false; }

    m_settings.save();

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
