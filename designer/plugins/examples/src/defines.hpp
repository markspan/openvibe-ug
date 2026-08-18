#pragma once

namespace OpenViBE {
// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_ModifiableSettings                                  CIdentifier(0x4AB0DD05, 0x32155D41)
#define Box_ModifiableSettingsDesc                              CIdentifier(0x3808515D, 0x97C7F9B6)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define Box_FlagIsUnstable                                      CIdentifier(0x666FFFFF, 0x666FFFFF)
}  // namespace OpenViBE
