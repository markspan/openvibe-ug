#include <vector>
#include <openvibe/ov_all.h>
#include "ovp_defines.h"

#include "box-algorithms/osc-controller/ovpCBoxAlgorithmOSCController.h"

// @BEGIN gipsa

#include "box-algorithms/ovpCBoxLSLExportGipsa.h"

// @END gipsa

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

OVP_Declare_Begin()
    OVP_Declare_New(CBoxAlgorithmOSCControllerDesc)
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());
    // @BEGIN gipsa
#if defined TARGET_HAS_ThirdPartyLSL
    OVP_Declare_New(CBoxAlgorithmLSLExportGipsaDesc)
#endif // TARGET_HAS_ThirdPartyLSL
    // @END gipsa

OVP_Declare_End()

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE
