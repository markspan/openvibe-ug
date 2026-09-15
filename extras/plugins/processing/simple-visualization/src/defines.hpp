/**-------------------------------------------------------------------------------------------------
 * \file defines.hpp
 * \brief Defines of simple visualization related identifiers.
 * \copyright Copyright (C) 2022 Inria
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *-------------------------------------------------------------------------------------------------
 */

#pragma once

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

namespace OpenViBE {

// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_SignalDisplay                                       CIdentifier(0x0055BE5F, 0x087BDD12)
#define Box_SignalDisplayDesc                                   CIdentifier(0x00C4F2D5, 0x58810276)
#define Box_DisplayCueImage                                     CIdentifier(0x005789A4, 0x3AB78A36)
#define Box_DisplayCueImageDesc                                 CIdentifier(0x086185A4, 0x796A854C)
#define Box_GrazVisualization                                   CIdentifier(0x00DD290D, 0x5F142820)
#define Box_GrazVisualizationDesc                               CIdentifier(0x00F1955D, 0x38813A6A)
#define Box_GeneralizedGrazVisualization                        CIdentifier(0xf0d1b4b9, 0xb420c213)
#define Box_GeneralizedGrazVisualizationDesc                    CIdentifier(0x0d414c51, 0xb0ed32f2)
#define Box_PowerSpectrumDisplay                                CIdentifier(0x004C0EA4, 0x713EC6D9)
#define Box_PowerSpectrumDisplayDesc                            CIdentifier(0x00116B40, 0x69E1B00D)
#define Box_TopographicMap2DDisplay                             CIdentifier(0x0B104632, 0x451C265F)
#define Box_TopographicMap2DDisplayDesc                         CIdentifier(0x7154037A, 0x4BC52A9F)
#define Box_Simple3DDisplay                                     CIdentifier(0x31A00483, 0x35924E6B)
#define Box_Simple3DDisplayDesc                                 CIdentifier(0x443E145F, 0x77205DA0)
#define Box_TopographicMap3DDisplay                             CIdentifier(0x36F95BE4, 0x0EF06290)
#define Box_TopographicMap3DDisplayDesc                         CIdentifier(0x6AD52C48, 0x6E1C1746)
#define Box_VoxelDisplay                                        CIdentifier(0x76E42EA2, 0x66FB5265)
#define Box_VoxelDisplayDesc                                    CIdentifier(0x79321659, 0x642D3D0C)
#define Box_TimeFrequencyMapDisplay                             CIdentifier(0x3AE63330, 0x76532117)
#define Box_TimeFrequencyMapDisplayDesc                         CIdentifier(0x1BAE74F3, 0x20FB7C89)
#define Box_P300SpellerVisualization                            CIdentifier(0x195E41D6, 0x6E684D47)
#define Box_P300SpellerVisualizationDesc                        CIdentifier(0x31DE2B0D, 0x028202E7)
#define Box_P300IdentifierCardVisualization                     CIdentifier(0x3AF7FF20, 0xA68745DB)
#define Box_P300IdentifierCardVisualizationDesc                 CIdentifier(0x84F146EF, 0x4AA712A4)
#define Box_P300MagicCardVisualization                          CIdentifier(0x841F46EF, 0x471AA2A4)
#define Box_P300MagicCardVisualizationDesc                      CIdentifier(0x37FAFF20, 0xA74685DB)
#define Box_ErpPlot                                             CIdentifier(0x10DC6917, 0x2B29B2A0)
#define Box_ErpPlotDesc                                         CIdentifier(0x10DC6917, 0x2B29B2A0)
#define Box_LevelMeasure                                        CIdentifier(0x657138E4, 0x46D6586F)
#define Box_LevelMeasureDesc                                    CIdentifier(0x4D061428, 0x11B02233)
#define Algorithm_LevelMeasure                                  CIdentifier(0x63C71764, 0x34A9717F)
#define Algorithm_LevelMeasureDesc                              CIdentifier(0x3EB6754F, 0x22FB1722)

// Types
//---------------------------------------------------------------------------------------------------
#define OV_AttributeId_Box_FlagIsUnstable                       CIdentifier(0x666FFFFF, 0x666FFFFF)

#define TypeId_SphericalLinearInterpolationType                 CIdentifier(0x44B76D9E, 0x618229BC)
#define TypeId_SignalDisplayMode                                CIdentifier(0x5DE046A6, 0x086340AA)
#define TypeId_SignalDisplayScaling                             CIdentifier(0x33A30739, 0x00D5299B)
#define TypeId_FeedbackMode                                     CIdentifier(0x5261636B, 0x464d4f44)


// Some enumerations
//---------------------------------------------------------------------------------------------------
enum EDisplayMode { ZoomIn, ZoomOut, GlobalBestFit };

enum EInterpolationType { Spline = 1, Laplacian = 2 };

enum class ESignalDisplayMode { Scroll, Scan };

enum class ESignalDisplayScaling { PerChannel, Global, None };

enum EFeedbackMode { Positive, Best, All, No };
}   // namespace OpenViBE
