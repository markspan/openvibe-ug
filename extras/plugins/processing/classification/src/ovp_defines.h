#pragma once

#define OVP_Classification_BoxTrainerFormatVersion                  4
#define OVP_Classification_BoxTrainerFormatVersionRequired          4

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define OVP_TypeId_ClassificationPairwiseStrategy                   OpenViBE::CIdentifier(0x0DD51C74, 0x3C4E74C9)
#define OVP_TypeId_OneVsOne_DecisionAlgorithms                      OpenViBE::CIdentifier(0xDEC1510, 0xDEC1510)

bool OVFloatEqual(double first, double second);
