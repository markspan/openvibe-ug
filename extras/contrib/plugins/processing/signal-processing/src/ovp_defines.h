#pragma once


// @BEGIN inserm-gpl

// Boxes
//---------------------------------------------------------------------------------------------------
#define OVP_ClassId_SpectralAnalysis                            OpenViBE::CIdentifier(0x1491AFA8, 0xF81E49D5)
#define OVP_ClassId_SpectralAnalysisDesc                        OpenViBE::CIdentifier(0xD011A66C, 0x61EF37D0)
#define OVP_ClassId_Algorithm_DetectingMinMax                   OpenViBE::CIdentifier(0x46C14A64, 0xE00541DD)
#define OVP_ClassId_Algorithm_DetectingMinMaxDesc               OpenViBE::CIdentifier(0x5B194CDA, 0x54E6DEC7)
#define OVP_ClassId_Box_DetectingMinMaxBoxAlgorithm             OpenViBE::CIdentifier(0xD647A2C4, 0xD4833160)
#define OVP_ClassId_Box_DetectingMinMaxBoxAlgorithmDesc         OpenViBE::CIdentifier(0xEF9E296A, 0x10285AE1)
#define OVP_ClassId_Algorithm_Downsampling                      OpenViBE::CIdentifier(0xBBBB4E18, 0x17695604)
#define OVP_ClassId_Algorithm_DownsamplingDesc                  OpenViBE::CIdentifier(0xC08BA8C1, 0x3A3B6E26)
#define OVP_ClassId_Box_DownsamplingBoxAlgorithm                OpenViBE::CIdentifier(0x6755FD0F, 0xE4857EA8)
#define OVP_ClassId_Box_DownsamplingBoxAlgorithmDesc            OpenViBE::CIdentifier(0xC8A99636, 0x81EF1AAD)
#define OVP_ClassId_Algorithm_ComputeTemporalFilterCoefs        OpenViBE::CIdentifier(0x55BAD77B, 0x5D8563A7)
#define OVP_ClassId_Algorithm_ComputeTemporalFilterCoefsDesc    OpenViBE::CIdentifier(0xD871BD98, 0x705ED068)
#define OVP_ClassId_Algorithm_ApplyTemporalFilter               OpenViBE::CIdentifier(0x9662518A, 0xE301A6FF)
#define OVP_ClassId_Algorithm_ApplyTemporalFilterDesc           OpenViBE::CIdentifier(0xAC0D004F, 0x0CFC5D9E)
#define OVP_ClassId_Box_TemporalFilterBoxAlgorithm              OpenViBE::CIdentifier(0x4469F0B2, 0x1DA995E5)
#define OVP_ClassId_Box_TemporalFilterBoxAlgorithmDesc          OpenViBE::CIdentifier(0x8BF6DD60, 0xBF02FA77)
#define OVP_ClassId_Box_ModTemporalFilterBoxAlgorithm           OpenViBE::CIdentifier(0xBF49D042, 0x9D79FE52)
#define OVP_ClassId_Box_ModTemporalFilterBoxAlgorithmDesc       OpenViBE::CIdentifier(0x7BF4BA62, 0xAF829A73)
#define OVP_ClassId_WindowingFunctions                          OpenViBE::CIdentifier(0x0B2F38AE, 0x6B0CF98F)
#define OVP_ClassId_WindowingFunctionsDesc                      OpenViBE::CIdentifier(0x40BFF79E, 0xA7BA6EAE)
#define OVP_ClassId_FastICA                                     OpenViBE::CIdentifier(0x00649B6E, 0x6C88CD17)
#define OVP_ClassId_FastICADesc                                 OpenViBE::CIdentifier(0x00E9436C, 0x41C904CA)
#define OVP_ClassId_AlgoUnivariateStatistic                     OpenViBE::CIdentifier(0x07A71212, 0x53D93D1C)
#define OVP_ClassId_AlgoUnivariateStatisticDesc                 OpenViBE::CIdentifier(0x408157F7, 0x4F1209F7)
#define OVP_ClassId_BoxAlgorithm_CSPSpatialFilterTrainer        OpenViBE::CIdentifier(0x51DB0D64, 0x2109714E)
#define OVP_ClassId_BoxAlgorithm_CSPSpatialFilterTrainerDesc    OpenViBE::CIdentifier(0x05120978, 0x14E061CD)
#define OVP_ClassId_BoxAlgorithm_Synchro                        OpenViBE::CIdentifier(0x7D8C1A18, 0x4C273A91)
#define OVP_ClassId_BoxAlgorithm_SynchroDesc                    OpenViBE::CIdentifier(0x4E806E5E, 0x5035290D)
#define OVP_ClassId_BoxAlgorithm_UnivariateStatistic            OpenViBE::CIdentifier(0x6118159D, 0x600C40B9)
#define OVP_ClassId_BoxAlgorithm_UnivariateStatisticDesc        OpenViBE::CIdentifier(0x36F742D9, 0x6D1477B2)


// Type definitions
//---------------------------------------------------------------------------------------------------
#define OVP_TypeId_SpectralComponent                    OpenViBE::CIdentifier(0x764E148A, 0xC704D4F5)
#define OVP_TypeId_FilterMethod                         OpenViBE::CIdentifier(0x2F2C606C, 0x8512ED68)
#define OVP_TypeId_FilterType                           OpenViBE::CIdentifier(0xFA20178E, 0x4CBA62E9)
#define OVP_TypeId_WindowMethod                         OpenViBE::CIdentifier(0x0A430FE4, 0x4F318280)
#define OVP_TypeId_FrequencyCutOffRatio                 OpenViBE::CIdentifier(0x709FC9DF, 0x30A2CB2A)
#define OVP_TypeId_MinMax                               OpenViBE::CIdentifier(0x4263AC45, 0x0AF5E07E)
#define OVP_TypeId_FastICA_OperatingMode                OpenViBE::CIdentifier(0x43A71032, 0x4AF96B9F)
#define OVP_TypeId_FastICA_DecompositionType            OpenViBE::CIdentifier(0x7B876033, 0x13590B93)
#define OVP_TypeId_FastICA_Nonlinearity                 OpenViBE::CIdentifier(0x4313472F, 0x37FD5961)

enum class ESpectralComponent { Amplitude = 1, Phase = 2, RealPart = 4, ImaginaryPart = 8 };

enum class EFilterMethod { Butterworth, Chebyshev, YuleWalker };

enum class EFilterType { LowPass, BandPass, HighPass, BandStop };

enum class EWindowMethod { None, Hamming, Hanning, Hann, Blackman, Triangular, SquareRoot };

enum class EFrequencyCutOffRatio { R14, R13, R12 };

enum class EMinMax { Min, Max };

enum class EFastICAMode { PCA, Whiten, ICA };

enum class EFastICADecomposition { Symmetric = 1, Deflate = 2 };                // Symmetric Must match ITPP 

enum class EFastICANonlinearity { Pow3 = 10, Tanh = 20, Gauss= 30, Skew = 40 }; // Use x^3/tanh(x)/Gaussian/skew non-linearity. Pow3 Must match ITPP.

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define OV_AttributeId_Box_FlagIsUnstable                                           OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

#define OVP_Algorithm_DetectingMinMax_InputParameterId_SignalMatrix                 OpenViBE::CIdentifier(0x9CA3B6BB, 0x6E24A3E3)
#define OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowStart              OpenViBE::CIdentifier(0xB3DED659, 0xD8A85CFA)
#define OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowEnd                OpenViBE::CIdentifier(0x9F55A091, 0xA042E9C0)
#define OVP_Algorithm_DetectingMinMax_InputParameterId_Sampling                     OpenViBE::CIdentifier(0x8519915D, 0xB6BE506D)
#define OVP_Algorithm_DetectingMinMax_OutputParameterId_SignalMatrix                OpenViBE::CIdentifier(0x853F2DE5, 0x628237CE)
#define OVP_Algorithm_DetectingMinMax_InputTriggerId_Initialize                     OpenViBE::CIdentifier(0x6B43B69D, 0xDA1EAE30)
#define OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMin                     OpenViBE::CIdentifier(0xFCB3CFC2, 0x980E3085)
#define OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMax                     OpenViBE::CIdentifier(0x24926194, 0x086E6C2A)

#define OVP_Algorithm_Downsampling_InputParameterId_Sampling                        OpenViBE::CIdentifier(0x7C510AFB, 0x4F2B9FB7)
#define OVP_Algorithm_Downsampling_InputParameterId_NewSampling                     OpenViBE::CIdentifier(0x8617E5FA, 0xC39CDBE7)
#define OVP_Algorithm_Downsampling_InputTriggerId_Initialize                        OpenViBE::CIdentifier(0x82D96F84, 0x9479A701)
#define OVP_Algorithm_Downsampling_InputTriggerId_Resample                          OpenViBE::CIdentifier(0x2A88AFF5, 0x79ECAEB3)
#define OVP_Algorithm_Downsampling_InputTriggerId_ResampleWithHistoric              OpenViBE::CIdentifier(0xD5740B33, 0x3785C886)
#define OVP_Algorithm_Downsampling_InputParameterId_SignalMatrix                    OpenViBE::CIdentifier(0xBB09054A, 0xEF13B2C6)
#define OVP_Algorithm_Downsampling_OutputParameterId_SignalMatrix                   OpenViBE::CIdentifier(0x4B9BE135, 0x14C10757)

#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_Sampling          OpenViBE::CIdentifier(0x25A9A0FF, 0x168F1B50)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterMethod      OpenViBE::CIdentifier(0xCFB7CDC9, 0x3EFF788E)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterType        OpenViBE::CIdentifier(0x1B7BCB2C, 0xE235A6E7)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterOrder       OpenViBE::CIdentifier(0x8DA1E555, 0x17E17828)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency   OpenViBE::CIdentifier(0x3175B774, 0xA15AEEB2)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency  OpenViBE::CIdentifier(0xE36387B7, 0xFB766612)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_BandPassRipple    OpenViBE::CIdentifier(0xB1500ED4, 0x0E558759)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_OutputParameterId_Matrix           OpenViBE::CIdentifier(0xE5B2A753, 0x150500B4)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_Initialize          OpenViBE::CIdentifier(0x3D2CBA61, 0x3FCF0DAC)
#define OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_ComputeCoefs        OpenViBE::CIdentifier(0x053A2C6E, 0x3A878825)
#define OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefsMatrix        OpenViBE::CIdentifier(0xD316C4E7, 0xE4E89FD3)
#define OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix             OpenViBE::CIdentifier(0xD5339105, 0x1D1293F0)
#define OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix    OpenViBE::CIdentifier(0x463276D1, 0xEAEE8AAD)
#define OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize                 OpenViBE::CIdentifier(0x3DAE69C7, 0x7CFCBE2C)
#define OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter                OpenViBE::CIdentifier(0xBC1F5655, 0x9807B400)
#define OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric    OpenViBE::CIdentifier(0xB7B7D546, 0x6000FF51)


#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_Mean                    OpenViBE::CIdentifier(0x2E1E6A87, 0x17F37568)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_Var                     OpenViBE::CIdentifier(0x479E18C9, 0x34A561AC)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_Range                   OpenViBE::CIdentifier(0x3CBC7D63, 0x5BF90946)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_Med                     OpenViBE::CIdentifier(0x2B236D6C, 0x4A37734F)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_IQR                     OpenViBE::CIdentifier(0x7A4E5C6E, 0x16EA324E)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_Percent                 OpenViBE::CIdentifier(0x77443BEF, 0x687B139F)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_PercentValue            OpenViBE::CIdentifier(0x2E9B5EEA, 0x58BC5AB6)
#define OVP_Algorithm_UnivariateStatistic_OutputParameterId_Compression             OpenViBE::CIdentifier(0x2A9C502C, 0x582959DA)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_Matrix                   OpenViBE::CIdentifier(0x1769269C, 0x41910DB9)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_MeanActive               OpenViBE::CIdentifier(0x6CE22614, 0x3BFD4A7A)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_VarActive                OpenViBE::CIdentifier(0x304B052D, 0x04F51601)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_RangeActive              OpenViBE::CIdentifier(0x4EA54A91, 0x69B90629)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_MedActive                OpenViBE::CIdentifier(0x6B0F55F1, 0x30015B5B)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_IQRActive                OpenViBE::CIdentifier(0x4F99672C, 0x7DFF3192)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentActive            OpenViBE::CIdentifier(0x3CA94023, 0x44E450C6)
#define OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentValue             OpenViBE::CIdentifier(0x0CB41979, 0x1CFF5A9C)
#define OVP_Algorithm_UnivariateStatistic_InputTriggerId_SpecialInitialize          OpenViBE::CIdentifier(0x38274F8D, 0x5FB938D2)
#define OVP_Algorithm_UnivariateStatistic_InputTriggerId_Initialize                 OpenViBE::CIdentifier(0x42CC2481, 0x70300F6D)
#define OVP_Algorithm_UnivariateStatistic_InputTriggerId_Process                    OpenViBE::CIdentifier(0x6CCD1D92, 0x02043C21)
#define OVP_Algorithm_UnivariateStatistic_OutputTriggerId_ProcessDone               OpenViBE::CIdentifier(0x34630103, 0x3F5F0A43)
