#pragma once

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationCognionics
 * \author Mike Chi (Cognionics, Inc.)
 * \copyright AGPL3
 * \date Thu Apr 18 21:19:49 2013
 * \brief The CConfigurationCognionics handles the configuration dialog specific to the Cognionics device.
 *
 * TODO: details
 *
 * \sa CDriverCognionics
 */
class CConfigurationCognionics final : public CConfigurationBuilder
{
public:
    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    //CConfigurationCognionics(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId);
    CConfigurationCognionics(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& comport);

    bool preConfigure() override;
    bool postConfigure() override;

    uint32_t& m_COMPORT;


protected:
    IDriverContext& m_driverCtx;

    /*
     * Insert here all specific attributes, such as a connection ID.
     * use references to directly modify the corresponding attribute of the driver
     * Example:
     */
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
