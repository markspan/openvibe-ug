#pragma once

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationGenericTimeSignal
 * \sa CDriverGenericTimeSignal
 */
class CConfigurationGenericTimeSignal final : public CConfigurationBuilder
{
public:
    CConfigurationGenericTimeSignal(IDriverContext& ctx, const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    IDriverContext& m_driverCtx;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
