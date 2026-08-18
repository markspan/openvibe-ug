#include "defines.hpp"

#include "box-algorithms/CBoxAlgorithmModifiableSettings.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Examples {
OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, Box_FlagIsUnstable.toString(), Box_FlagIsUnstable.id());

    OVP_Declare_New(CBoxAlgorithmModifiableSettingsDesc)
OVP_Declare_End()

}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
