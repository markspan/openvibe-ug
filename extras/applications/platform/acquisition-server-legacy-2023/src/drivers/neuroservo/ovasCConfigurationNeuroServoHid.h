/*
* NeuroServo driver for OpenViBE
*
* \author (NeuroServo)
* \date Wed Nov 23 00:24:00 2016
*
*/

#pragma once

#if defined TARGET_OS_Windows
#if defined TARGET_HAS_ThirdPartyNeuroServo

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationNeuroServoHid
 * \author  (NeuroServo)
 * \date Wed Nov 23 00:24:00 2016
 * \brief The CConfigurationNeuroServoHid handles the configuration dialog specific to the NeuroServo device.
 *
 * TODO: details
 *
 * \sa CDriverNeuroServoHid
 */
class CConfigurationNeuroServoHid final : public CConfigurationBuilder
{
public:
    CConfigurationNeuroServoHid(IDriverContext& ctx, const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;

    // Automatic Shutdown
    void checkRadioAutomaticShutdown(const bool state) { m_automaticShutdown = state; }
    bool getAutomaticShutdownStatus() { return m_automaticShutdown; }
    void setRadioAutomaticShutdown(const bool state) { m_automaticShutdown = state; }

    // Shutdown on driver disconnect
    void checkRadioShutdownOnDriverDisconnect(const bool state) { m_shutdownOnDriverDisconnect = state; }
    bool getShutdownOnDriverDisconnectStatus() { return m_shutdownOnDriverDisconnect; }
    void setRadioShutdownOnDriverDisconnect(const bool state) { m_shutdownOnDriverDisconnect = state; }

    // Device Light Enable
    void checkRadioDeviceLightEnable(const bool state) { m_deviceLightEnable = state; }
    bool getDeviceLightEnableStatus() { return m_deviceLightEnable; }
    void setRadioDeviceLightEnable(const bool state) { m_deviceLightEnable = state; }

protected:
    IDriverContext& m_driverCtx;

private:
    bool m_automaticShutdown          = false;
    bool m_shutdownOnDriverDisconnect = false;
    bool m_deviceLightEnable          = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
#endif // TARGET_OS_Windows
