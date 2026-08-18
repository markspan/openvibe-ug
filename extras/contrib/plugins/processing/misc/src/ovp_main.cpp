#include <vector>
#include <openvibe/ov_all.h>
#include "ovp_defines.h"

#include "box-algorithms/ovpCMouseControl.h"                            // inserm

namespace OpenViBE {
namespace Plugins {
namespace Tools {

OVP_Declare_Begin()
    // @BEGIN inserm
    OVP_Declare_New(CMouseControlDesc)
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());
    // @END inserm

OVP_Declare_End()

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
