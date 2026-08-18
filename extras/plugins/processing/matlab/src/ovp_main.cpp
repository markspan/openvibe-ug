#include <openvibe/ov_all.h>

#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmMatlabScripting.h"

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Matlab {

OVP_Declare_Begin()
#if defined TARGET_HAS_ThirdPartyMatlab

    OVP_Declare_New(CBoxAlgorithmMatlabScriptingDesc)
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

#endif // TARGET_HAS_ThirdPartyMatlab

OVP_Declare_End()

}  // namespace Matlab
}  // namespace Plugins
}  // namespace OpenViBE
