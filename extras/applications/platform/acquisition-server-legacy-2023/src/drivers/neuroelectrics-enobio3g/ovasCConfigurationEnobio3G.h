#pragma once

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationEnobio3G
 * \author Anton Albajes-Eizagirre (NeuroElectrics anton.albajes-eizagirre@neuroelectrics.com)
 * \date Tue Apr 15 09:25:20 2014
 * \brief The CConfigurationEnobio3G handles the configuration dialog specific to the Enobio3G device.
 *
 * TODO: details
 *
 * \sa CDriverEnobio3G
 */
class CConfigurationEnobio3G final : public CConfigurationBuilder
{
public:
    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    //CConfigurationEnobio3G(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId);
    CConfigurationEnobio3G(IDriverContext& ctx, const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;
    unsigned char* getMacAddress();

protected:
    IDriverContext& m_driverCtx;

private:
    /*
     * Insert here all specific attributes, such as a connection ID.
     * use references to directly modify the corresponding attribute of the driver
     * Example:
     */
    // uint32_t& m_connectionID;
    unsigned char m_macAddress[6]; // mac address of the device
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
