#pragma once

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationDriverSimulatedDeviator
 * \author Jussi T. Lindgren (Inria)
 * \brief The CConfigurationDriverSimulatedDeviator handles the configuration dialog specific to the Simulated Deviator driver
 *
 * \sa CDriverSimulatedDeviator
 */

class CConfigurationDriverSimulatedDeviator final : public CConfigurationBuilder
{
public:
    CConfigurationDriverSimulatedDeviator(IDriverContext& ctx, const char* gtkBuilderFilename, bool& sendPeriodicStims, double& offset,
                                          double& spread, double& maxDev, double& pullback, double& update, uint64_t& wavetype,
                                          double& freezeFrequency, double& freezeDuration);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    IDriverContext& m_driverCtx;

    bool& m_sendPeriodicStimulations;
    double& m_Offset;
    double& m_Spread;
    double& m_MaxDev;
    double& m_Pullback;
    double& m_Update;
    uint64_t& m_Wavetype;
    double& m_FreezeFrequency;
    double& m_FreezeDuration;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
