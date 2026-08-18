#pragma once

#if defined TARGET_HAS_ThirdPartyOpenAL
#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationOpenALAudioCapture
 * \author Aurelien Van Langhenhove (CIC-IT Garches)
 * \date Mon May 16 16:55:49 2011
 * \erief The CConfigurationOpenALAudioCapture handles the configuration dialog specific to the OpenAL audio capture device.
 *
 * TODO: details
 *
 * \sa CDriverOpenALAudioCapture
 */
class CConfigurationOpenALAudioCapture final : public CConfigurationBuilder
{
public:
    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    //CConfigurationOpenALAudioCapture(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId);
    CConfigurationOpenALAudioCapture(IDriverContext& ctx, const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    IDriverContext& m_driverCtx;

    /*
     * Insert here all specific attributes, such as a connection ID.
     * use references to directly modify the corresponding attribute of the driver
     * Example:
     */
    // uint32_t& m_connectionID;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif //TARGET_HAS_ThirdPartyOpenAL
