#include "ovasCConfigurationGenericTimeSignal.h"

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationGenericTimeSignal::CConfigurationGenericTimeSignal(IDriverContext& ctx, const char* gtkBuilderFilename)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx) {}

bool CConfigurationGenericTimeSignal::preConfigure() { return CConfigurationBuilder::preConfigure(); }

// normal header is filled (Subject ID, Age, Gender, channels, sampling frequency)
bool CConfigurationGenericTimeSignal::postConfigure() { return CConfigurationBuilder::postConfigure(); }

}  // namespace AcquisitionServer
}  // namespace OpenViBE
