#pragma once

#ifdef TARGET_OS_Windows

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationDriverMensiaAcquisition
 * \author Jozef Legeny (Mensia)
 * \date 28 jan 2013
 * \brief The CConfigurationDriverMensiaAcquisition loads the configuration using the mensia-acquisition dynamic library
 *
 * \sa CDriverGenericOscillator
 */

class CConfigurationDriverMensiaAcquisition final : public CConfigurationBuilder
{
public:
    CConfigurationDriverMensiaAcquisition(IDriverContext& ctx, const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    IDriverContext& m_driverCtx;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
