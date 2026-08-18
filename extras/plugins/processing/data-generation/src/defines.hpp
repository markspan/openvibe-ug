#pragma once

// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_SinusSignalGenerator                OpenViBE::CIdentifier(0x7E33BDB8, 0x68194A4A)
#define Box_SinusSignalGeneratorDesc            OpenViBE::CIdentifier(0x2633AFA2, 0x6974E32F)
#define Box_TimeSignalGenerator                 OpenViBE::CIdentifier(0x28A5E7FF, 0x530095DE)
#define Box_TimeSignalGeneratorDesc             OpenViBE::CIdentifier(0x57AD8655, 0x1966B4DC)
#define Box_NoiseGenerator                      OpenViBE::CIdentifier(0x0E3929F1, 0x15AF76B9)
#define Box_NoiseGeneratorDesc                  OpenViBE::CIdentifier(0x7237458A, 0x1F312C4A)
#define Box_ChannelUnitsGenerator               OpenViBE::CIdentifier(0x42B09186, 0x582C8422)
#define Box_ChannelUnitsGeneratorDesc           OpenViBE::CIdentifier(0x4901A752, 0xD8578577)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define TypeId_NoiseType                        OpenViBE::CIdentifier(0x2E85E95E, 0x8A1A8365)
#define TypeId_NoiseType_Uniform                1
#define TypeId_NoiseType_Gaussian               2
