#include "ovp_defines.h"

#include "box-algorithms/BoxAcquisitionClientLegacy.h"
#include "box-algorithms/BoxAcquisitionClient.h"

namespace OpenViBE {
namespace Plugins {
namespace Acquisition {

OVP_Declare_Begin()
	OVP_Declare_New(CBoxAlgorithmAcquisitionClientLegacyDesc)
	OVP_Declare_New(CBoxAcquisitionClientDesc)
OVP_Declare_End()

}  // namespace Acquisition
}  // namespace Plugins
}  // namespace OpenViBE
