/**
 * The gMobilab driver was contributed
 * by Lucie Daubigney from Supelec Metz
 */

#pragma once

#include "../ovasCConfigurationBuilder.h"

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

// #ifdef TARGET_OS_Windows
//  #include <Windows.h>
// #endif

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationGTecGMobiLabPlus final : public CConfigurationBuilder
{
public:

    CConfigurationGTecGMobiLabPlus(const char* gtkBuilderFilename, std::string& portName, bool& testMode);

    bool preConfigure() override;
    bool postConfigure() override;

private:

    std::string& m_portName;
    bool& m_testMode;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI
