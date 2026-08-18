#pragma once

// Boxes
//---------------------------------------------------------------------------------------------------
#define OVP_ClassId_MouseControl                    OpenViBE::CIdentifier(0xDA4B4EEB, 0x64FC6A16)
#define OVP_ClassId_MouseControlDesc                OpenViBE::CIdentifier(0xB6B65C98, 0xA756ED0E)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define OV_AttributeId_Box_FlagIsUnstable           OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)
