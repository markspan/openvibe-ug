#ifdef TARGET_OS_Windows

#include "ovasCConfigurationDriverMensiaAcquisition.h"

namespace OpenViBE {
namespace AcquisitionServer {

//TODO_JL Add the URL as parameter for configuration so it can be saved and loaded

CConfigurationDriverMensiaAcquisition::CConfigurationDriverMensiaAcquisition(IDriverContext& ctx, const char* gtkBuilderFilename)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx) {}

bool CConfigurationDriverMensiaAcquisition::preConfigure()
{
    //TODO_JL call preConfigure from DLL
    return CConfigurationBuilder::preConfigure();
}

bool CConfigurationDriverMensiaAcquisition::postConfigure()
{
    //if (m_applyConfig) { }    //TODO_JL call postConfigure from DLL
    return CConfigurationBuilder::postConfigure();
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
