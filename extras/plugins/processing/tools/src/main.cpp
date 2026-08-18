#include "defines.hpp"

//// #include "box-algorithms/ovpCBoxAlgorithmLatencyEvaluation.h"
#include "box-algorithms/CBoxAlgorithmMouseTracking.hpp"
#include "box-algorithms/CBoxAlgorithmKeypressEmulator.hpp"

#include "openvibe/CIdentifier.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Tools {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, Box_FlagIsUnstable.toString(), Box_FlagIsUnstable.id());
    // OVP_Declare_New(Tools::CBoxAlgorithmLatencyEvaluationDesc)
#if defined(TARGET_HAS_ThirdPartyGTK)
    OVP_Declare_New(CBoxAlgorithmMouseTrackingDesc)
#endif

    OVP_Declare_New(CBoxAlgorithmKeypressEmulatorDesc)
    // @note the following code is a bit long so we've implemented it inside the class
    CBoxAlgorithmKeypressEmulator::RegisterEnums(context);

OVP_Declare_End()

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
