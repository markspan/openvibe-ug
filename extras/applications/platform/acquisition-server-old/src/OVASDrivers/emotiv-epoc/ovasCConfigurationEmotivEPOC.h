#pragma once

#if defined TARGET_HAS_ThirdPartyEmotivAPI

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include <gtk/gtk.h>

#if defined TARGET_OS_Windows
#include <windows.h>
#endif

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationEmotivEPOC
 * \author Laurent Bonnet (INRIA)
 * \date 21 july 2010
 * \erief The CConfigurationEmotivEPOC handles the configuration dialog specific to the Emotiv EPOC headset.
 *
 * \sa CDriverEmotivEPOC
 */
class CConfigurationEmotivEPOC final : public CConfigurationBuilder
{
public:

    CConfigurationEmotivEPOC(IDriverContext& ctx, const char* gtkBuilderFilename, bool& rUseGyroscope, CString& rPathToEmotivResearchSDK, uint32_t& rUserID);

    bool preConfigure() override;
    bool postConfigure() override;

protected:

    IDriverContext& m_driverCtx;
    bool& m_useGyroscope;
    CString& m_rPathToEmotivResearchSDK;
    uint32_t& m_userID;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyEmotivAPI
