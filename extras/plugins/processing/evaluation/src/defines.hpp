///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_ClassifierAccuracyMeasure                   OpenViBE::CIdentifier(0x48395CE7, 0x17D62550)
#define Box_ClassifierAccuracyMeasureDesc               OpenViBE::CIdentifier(0x067F38CC, 0x084A6ED3)
#define Algorithm_ConfusionMatrix                       OpenViBE::CIdentifier(0x699F416B, 0x3BAE4324)
#define Algorithm_ConfusionMatrixDesc                   OpenViBE::CIdentifier(0x4CDD225D, 0x6C9A59DB)
#define Box_ConfusionMatrix                             OpenViBE::CIdentifier(0x1AB625DA, 0x3B2502CE)
#define Box_ConfusionMatrixDesc                         OpenViBE::CIdentifier(0x52237A64, 0x63555613)
#define Box_KappaCoef                                   OpenViBE::CIdentifier(0x160D8F1B, 0xD864C5BB)
#define Box_KappaCoefDesc                               OpenViBE::CIdentifier(0xD8BA2199, 0xD252BECB)
#define Box_ROCCurve                                    OpenViBE::CIdentifier(0x06FE5B1B, 0xDE066FEC)
#define Box_ROCCurveDesc                                OpenViBE::CIdentifier(0xCB5DFCEA, 0xAF41EAB2)
#define Box_StatisticGenerator                          OpenViBE::CIdentifier(0x83EDA40B, 0x425FBFFE)
#define Box_StatisticGeneratorDesc                      OpenViBE::CIdentifier(0x35A0CB63, 0x78882C28)

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums                        OpenViBE::CIdentifier(0x75502E8E, 0x05D838EE)
#define Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage                  OpenViBE::CIdentifier(0x7E504E8E, 0x058858EE)
#define Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet        OpenViBE::CIdentifier(0x7E504E8F, 0x058858EF)
#define Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet    OpenViBE::CIdentifier(0x45220B61, 0x13FD7491)
#define Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes                  OpenViBE::CIdentifier(0x67780C91, 0x2A556C51)
#define Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix            OpenViBE::CIdentifier(0x67780C91, 0x2A556C51)

#define Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget                   OpenViBE::CIdentifier(0x4D390BDA, 0x6A180667)
#define Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier               OpenViBE::CIdentifier(0x3C132C38, 0x557D2503)
#define Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget                    OpenViBE::CIdentifier(0x6B1E76B3, 0x06741B21)
#define Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier                OpenViBE::CIdentifier(0x3EFC64B8, 0x5ACC3125)
#define Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed           OpenViBE::CIdentifier(0x790C2277, 0x3D041A63)


#define FIRST_CLASS_SETTING_INDEX 2

#define OV_AttributeId_Box_FlagIsUnstable                                                   OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)
