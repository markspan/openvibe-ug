#pragma once

namespace OpenViBE {
// Boxes
//---------------------------------------------------------------------------------------------------
#define Box_LevelMeasure                                            CIdentifier(0x657138E4, 0x46D6586F)
#define Box_LevelMeasureDesc                                        CIdentifier(0x4D061428, 0x11B02233)
#define Box_SignalDisplay                                           CIdentifier(0x0055BE5F, 0x087BDD12)
#define Box_SignalDisplayDesc                                       CIdentifier(0x00C4F2D5, 0x58810276)
#define Box_GrazVisualization                                       CIdentifier(0x00DD290D, 0x5F142820)
#define Box_GrazVisualizationDesc                                   CIdentifier(0x00F1955D, 0x38813A6A)
#define Box_PowerSpectrumDisplay                                    CIdentifier(0x004C0EA4, 0x713EC6D9)
#define Box_PowerSpectrumDisplayDesc                                CIdentifier(0x00116B40, 0x69E1B00D)
#define Box_TopographicMap2DDisplay                                 CIdentifier(0x0B104632, 0x451C265F)
#define Box_TopographicMap2DDisplayDesc                             CIdentifier(0x7154037A, 0x4BC52A9F)
#define Box_P300IdentifierCardVisualisation                         CIdentifier(0x3AF7FF20, 0xA68745DB)
#define Box_P300IdentifierCardVisualisationDesc                     CIdentifier(0x84F146EF, 0x4AA712A4)
#define Box_MatrixDisplay                                           CIdentifier(0x54F0796D, 0x3EDE2CC0)
#define Box_MatrixDisplayDesc                                       CIdentifier(0x63AB4BA7, 0x022C1524)

#define Algorithm_LevelMeasure                                      CIdentifier(0x63C71764, 0x34A9717F)
#define Algorithm_LevelMeasureDesc                                  CIdentifier(0x3EB6754F, 0x22FB1722)
#define Algorithm_SphericalSplineInterpolation                      CIdentifier(0x4F112803, 0x661D4029)
#define Algorithm_SphericalSplineInterpolationDesc                  CIdentifier(0x00D67A20, 0x3D3D4729)

// Type definitions
//---------------------------------------------------------------------------------------------------
#define TypeId_SphericalLinearInterpolation                         CIdentifier(0x44B76D9E, 0x618229BC)
#define TypeId_SignalDisplayMode                                    CIdentifier(0x5DE046A6, 0x086340AA)

enum class EInterpolationType { Spline = 1, Laplacian = 2 };

enum class ESignalDisplayMode { Scroll, Scan };

enum class EDisplayMode { ZoomIn, ZoomOut, GlobalBestFit };

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define SplineInterpolation_InputParameterId_SplineOrder            CIdentifier(0x3B8200F6, 0x205162C7)
#define SplineInterpolation_InputParameterId_ControlPointsCount     CIdentifier(0x2ABF11FC, 0x174A2CFE)
#define SplineInterpolation_InputParameterId_ControlPointsCoord     CIdentifier(0x36F743FE, 0x37897AB9)
#define SplineInterpolation_InputParameterId_ControlPointsValues    CIdentifier(0x4EA55599, 0x670274A7)
#define SplineInterpolation_InputParameterId_SamplePointsCoord      CIdentifier(0x280A531D, 0x339C18AA)
#define SplineInterpolation_OutputParameterId_SamplePointsValues    CIdentifier(0x12D0319C, 0x51ED4D8B)
#define SplineInterpolation_OutputParameterId_MinSamplePointValue   CIdentifier(0x0CEE2041, 0x79455EED)
#define SplineInterpolation_OutputParameterId_MaxSamplePointValue   CIdentifier(0x1ECB03E3, 0x40EF757F)
#define SplineInterpolation_InputTriggerId_PrecomputeTables         CIdentifier(0x42A650DA, 0x62B35F76)
#define SplineInterpolation_InputTriggerId_ComputeSplineCoefs       CIdentifier(0x5B353712, 0x069F3D3B)
#define SplineInterpolation_InputTriggerId_ComputeLaplacianCoefs    CIdentifier(0x7D8C545E, 0x7C086660)
#define SplineInterpolation_InputTriggerId_InterpolateSpline        CIdentifier(0x1241610E, 0x03CB1AD9)
#define SplineInterpolation_InputTriggerId_InterpolateLaplacian     CIdentifier(0x11CE0AC3, 0x0FD85469)
#define SplineInterpolation_OutputTriggerId_Error                   CIdentifier(0x08CB0679, 0x3A6F3C3A)
}  // namespace OpenViBE
