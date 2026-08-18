#include "ovp_defines.h"

#include "box-algorithms/ovpCHelloWorld.h"
#include "box-algorithms/ovpCHelloWorldWithInput.h"

#include "box-algorithms/ovpCLog.h"
#include "box-algorithms/ovpCBoxAlgorithmNothing.h"
#include "box-algorithms/ovpCBoxAlgorithmClock.h"

namespace OpenViBE {
namespace Plugins {
namespace Examples {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

    OVP_Declare_New(CHelloWorldDesc)
    OVP_Declare_New(CHelloWorldWithInputDesc)

    OVP_Declare_New(CLogDesc)
    OVP_Declare_New(CBoxAlgorithmNothingDesc)
    OVP_Declare_New(CBoxAlgorithmClockDesc)

OVP_Declare_End()

}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
