#pragma once

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationDriverGenericOscillator
 * \author Jozef Legeny (Inria)
 * \date 28 jan 2013
 * \brief The CConfigurationDriverGenericOscillator handles the configuration dialog specific to the Generic Oscillator driver
 *
 * \sa CDriverGenericOscillator
 */

class CConfigurationDriverGenericOscillator final : public CConfigurationBuilder
{
public:
    CConfigurationDriverGenericOscillator(IDriverContext& ctx, const char* gtkBuilderFilename, bool& sendPeriodicStimulations, double& stimulationInterval);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    IDriverContext& m_driverCtx;

    bool& m_sendPeriodicStimulations;
    double& m_stimulationInterval;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
