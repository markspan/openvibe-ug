#pragma once

#include "../../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationMKSNVXDriver
 * \author mkochetkov (MKS)
 * \date Tue Jan 21 23:21:03 2014
 * \brief The CConfigurationMKSNVXDriver handles the configuration dialog specific to the MKSNVXDriver device.
 *
 * TODO: details
 *
 * \sa CDriverMKSNVXDriver
 */
class CConfigurationMKSNVXDriver final : public CConfigurationBuilder
{
    uint32_t& dataMode_;
    bool& showAuxChannels_;
protected:
    IDriverContext& m_driverCtx;
public:
    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    //CConfigurationMKSNVXDriver(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId);
    CConfigurationMKSNVXDriver(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& dataMode, bool& auxChannels);

    bool preConfigure() override;
    bool postConfigure() override;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
