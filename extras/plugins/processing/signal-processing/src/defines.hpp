#pragma once

// Boxes
//---------------------------------------------------------------------------------------------------
#define OVP_ClassId_Identity                                        OpenViBE::CIdentifier(0x5DFFE431, 0x35215C50)
#define OVP_ClassId_IdentityDesc                                    OpenViBE::CIdentifier(0x54743810, 0x6A1A88CC)
#define OVP_ClassId_TimeBasedEpoching                               OpenViBE::CIdentifier(0x00777FA0, 0x5DC3F560)
#define OVP_ClassId_TimeBasedEpochingDesc                           OpenViBE::CIdentifier(0x00ABDABE, 0x41381683)
#define OVP_ClassId_BoxAlgorithm_EOG_Denoising                      OpenViBE::CIdentifier(0xC223FF12, 0x069A987E)
#define OVP_ClassId_BoxAlgorithm_EOG_DenoisingDesc                  OpenViBE::CIdentifier(0x4F9BE623, 0xF2027046)
#define OVP_ClassId_BoxAlgorithm_EOG_Denoising_Calibration          OpenViBE::CIdentifier(0xE8DFE002, 0x70389932)
#define OVP_ClassId_BoxAlgorithm_EOG_Denoising_CalibrationDesc      OpenViBE::CIdentifier(0xF4D74831, 0x88B80DCF)
#define OVP_ClassId_BoxAlgorithm_Inverse_DWT                        OpenViBE::CIdentifier(0x5B5B8468, 0x212CF963)
#define OVP_ClassId_BoxAlgorithm_Inverse_DWTDesc                    OpenViBE::CIdentifier(0x01B9BC9A, 0x34766AE9)
#define OVP_ClassId_BoxAlgorithm_DiscreteWaveletTransform           OpenViBE::CIdentifier(0x824194C5, 0x46D7FDE9)
#define OVP_ClassId_BoxAlgorithm_DiscreteWaveletTransformDesc       OpenViBE::CIdentifier(0x6744711B, 0xF21B59EC)
#define OVP_ClassId_BoxAlgorithm_EpochAverage                       OpenViBE::CIdentifier(0x21283D9F, 0xE76FF640)
#define OVP_ClassId_BoxAlgorithm_EpochAverageDesc                   OpenViBE::CIdentifier(0x95F5F43E, 0xBE629D82)
#define OVP_ClassId_Algorithm_MatrixAverage                         OpenViBE::CIdentifier(0x5E5A6C1C, 0x6F6BEB03)
#define OVP_ClassId_Algorithm_MatrixAverageDesc                     OpenViBE::CIdentifier(0x1992881F, 0xC938C0F2)
#define OVP_ClassId_BoxAlgorithm_Crop                               OpenViBE::CIdentifier(0x7F1A3002, 0x358117BA)
#define OVP_ClassId_BoxAlgorithm_CropDesc                           OpenViBE::CIdentifier(0x64D619D7, 0x26CC42C9)
#define OVP_ClassId_BoxAlgorithm_DifferentialIntegral               OpenViBE::CIdentifier(0xCE490CBF, 0xDF7BA2E2)
#define OVP_ClassId_BoxAlgorithm_DifferentialIntegralDesc           OpenViBE::CIdentifier(0xCE490CBF, 0xDF7BA2E3)
#define OVP_ClassId_BoxAlgorithm_MatrixTranspose                    OpenViBE::CIdentifier(0x5E0F04B5, 0x5B5005CF)
#define OVP_ClassId_BoxAlgorithm_MatrixTransposeDesc                OpenViBE::CIdentifier(0x119249F7, 0x556C7E0D)
#define OVP_ClassId_BoxAlgorithm_ERSPAverage                        OpenViBE::CIdentifier(0x3CDB4B72, 0x295D51F7)
#define OVP_ClassId_BoxAlgorithm_ERSPAverageDesc                    OpenViBE::CIdentifier(0x32C45B7E, 0x2F1B7D58)
#define OVP_ClassId_BoxAlgorithm_ARCoefficients                     OpenViBE::CIdentifier(0xBAADC2F3, 0xB556A07B)
#define OVP_ClassId_BoxAlgorithm_ARCoefficientsDesc                 OpenViBE::CIdentifier(0xBAADC2F3, 0xB556A07A)
#define OVP_ClassId_BoxAlgorithm_ConnectivityMeasure                OpenViBE::CIdentifier(0x994a9a45, 0x4181a048)
#define OVP_ClassId_BoxAlgorithm_ConnectivityMeasureDesc            OpenViBE::CIdentifier(0xaf5e56f9, 0xcbc54c18)
#define OVP_ClassId_ReferenceChannel                                OpenViBE::CIdentifier(0xEFA8E95B, 0x4F22551B)
#define OVP_ClassId_ReferenceChannelDesc                            OpenViBE::CIdentifier(0x1873B151, 0x969DD4E4)
#define OVP_ClassId_ChannelSelector                                 OpenViBE::CIdentifier(0x39484563, 0x46386889)
#define OVP_ClassId_ChannelSelectorDesc                             OpenViBE::CIdentifier(0x34893489, 0x44934897)
#define OVP_ClassId_SimpleDSP                                       OpenViBE::CIdentifier(0x00E26FA1, 0x1DBAB1B2)
#define OVP_ClassId_SimpleDSPDesc                                   OpenViBE::CIdentifier(0x00C44BFE, 0x76C9269E)
#define OVP_ClassId_SignalAverage                                   OpenViBE::CIdentifier(0x00642C4D, 0x5DF7E50A)
#define OVP_ClassId_SignalAverageDesc                               OpenViBE::CIdentifier(0x007CDCE9, 0x16034F77)
#define OVP_ClassId_SignalConcatenation                             OpenViBE::CIdentifier(0x6568D29B, 0x0D753CCA)
#define OVP_ClassId_SignalConcatenationDesc                         OpenViBE::CIdentifier(0x3921BACD, 0x1E9546FE)
#define OVP_ClassId_BoxAlgorithm_QuadraticForm                      OpenViBE::CIdentifier(0x54E73B81, 0x1AD356C6)
#define OVP_ClassId_BoxAlgorithm_QuadraticFormDesc                  OpenViBE::CIdentifier(0x31C11856, 0x3E4F7B67)
#define OVP_ClassId_Algorithm_HilbertTransform                      OpenViBE::CIdentifier(0x344B79DE, 0x89EAAABB)
#define OVP_ClassId_Algorithm_HilbertTransformDesc                  OpenViBE::CIdentifier(0x8CAB236A, 0xA789800D)
#define OVP_ClassId_BoxAlgorithm_Hilbert                            OpenViBE::CIdentifier(0x7878A47F, 0x9A8FE349)
#define OVP_ClassId_BoxAlgorithm_HilbertDesc                        OpenViBE::CIdentifier(0x2DB54E2F, 0x435675EF)
#define OVP_ClassId_Algorithm_ARBurgMethod                          OpenViBE::CIdentifier(0x3EC6A165, 0x2823A034)
#define OVP_ClassId_Algorithm_ARBurgMethodDesc                      OpenViBE::CIdentifier(0xD7234DFF, 0x55447A14)
#define OVP_ClassId_BoxAlgorithm_Matrix3dTo2d                       OpenViBE::CIdentifier(0x3ab2b81e, 0x73ef01a5)
#define OVP_ClassId_BoxAlgorithm_Matrix3dTo2dDesc                   OpenViBE::CIdentifier(0xb9099590, 0xae33d758)
#define OVP_ClassId_BoxAlgorithm_Matrix2dToVector                   OpenViBE::CIdentifier(0x04ef4593, 0x6838130a)
#define OVP_ClassId_BoxAlgorithm_Matrix2dToVectorDesc               OpenViBE::CIdentifier(0xf40bf934, 0x3589c71e)
#define OVP_ClassId_BoxAlgorithm_ConnectivitySpectrumExtract        OpenViBE::CIdentifier(0x860ae55c, 0x836a6271)
#define OVP_ClassId_BoxAlgorithm_ConnectivitySpectrumExtractDesc    OpenViBE::CIdentifier(0xb6e4b7c1, 0x7331662d)
#define OVP_ClassId_BoxAlgorithm_PulseRateCalculator                OpenViBE::CIdentifier(0x034466a7, 0x8059bcf3)
#define OVP_ClassId_BoxAlgorithm_PulseRateCalculatorDesc            OpenViBE::CIdentifier(0x04d32034, 0x096c0b42)
#define OVP_ClassId_BoxAlgorithm_EntropyMeasure                     OpenViBE::CIdentifier(0x46911275, 0xEC146FA3)
#define OVP_ClassId_BoxAlgorithm_EntropyMeasureDesc                 OpenViBE::CIdentifier(0xC7482EBA, 0x31772D60)

#define Box_EpochVariance                                           OpenViBE::CIdentifier(0x335384EA, 0x88C917D0)
#define Box_EpochVarianceDesc                                       OpenViBE::CIdentifier(0xA15EAEC5, 0xAB0CE730)
#define Box_IFFT                                                    OpenViBE::CIdentifier(0xD533E997, 0x4AFD2423)
#define Box_IFFTDesc                                                OpenViBE::CIdentifier(0xD533E997, 0x4AFD2423)

// Type definitions
//---------------------------------------------------------------------------------------------------

#define OVP_TypeId_EpochAverageMethod                               OpenViBE::CIdentifier(0x6530BDB1, 0xD057BBFE)
#define OVP_TypeId_CropMethod                                       OpenViBE::CIdentifier(0xD0643F9E, 0x8E35FE0A)
#define OVP_TypeId_SelectionMethod                                  OpenViBE::CIdentifier(0x3BCF9E67, 0x0C23994D)
#define OVP_TypeId_MatchMethod                                      OpenViBE::CIdentifier(0x666F25E9, 0x3E5738D6)
#define OVP_TypeId_DifferentialIntegralOperation                    OpenViBE::CIdentifier(0x6E6AD85D, 0x14FD203A)
#define OVP_TypeId_WindowType                                       OpenViBE::CIdentifier(0x332BBB80, 0xC212810A)
#define OVP_TypeId_WaveletType                                      OpenViBE::CIdentifier(0x393EAC3E, 0x793C0F1D)
#define OVP_TypeId_WaveletLevel                                     OpenViBE::CIdentifier(0xF80A2144, 0x6E692C51)

enum class EEpochAverageMethod { Moving, MovingImmediate, Block, Cumulative };

enum class ECropMethod { Min, Max, MinMax };

enum class ESelectionMethod { Select, Reject, Select_EEG };

enum class EMatchMethod { Name, Index, Smart };

enum class EDifferentialIntegralOperation { Differential, Integral };

enum class EWaveletType
{
    Haar,
    Db1, Db2, Db3, Db4, Db5, Db6, Db7, Db8, Db9, Db10, Db11, Db12, Db13, Db14, Db15,
    Bior11, Bior13, Bior15, Bior22, Bior24, Bior26, Bior28, Bior31, Bior33, Bior35, Bior37, Bior39, Bior44, Bior55, Bior68,
    Coif1, Coif2, Coif3, Coif4, Coif5,
    Sym1, Sym2, Sym3, Sym4, Sym5, Sym6, Sym7, Sym8, Sym9, Sym10
};

enum class EWaveletLevel { L1, L2, L3, L4, L5 };

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define OVP_Value_CoupledStringSeparator                            '-'
//#define OVP_Value_AllSelection                                    '*'

#define OVP_Algorithm_MatrixAverage_InputParameterId_Matrix                                 OpenViBE::CIdentifier(0x913E9C3B, 0x8A62F5E3)
#define OVP_Algorithm_MatrixAverage_InputParameterId_MatrixCount                            OpenViBE::CIdentifier(0x08563191, 0xE78BB265)
#define OVP_Algorithm_MatrixAverage_InputParameterId_AveragingMethod                        OpenViBE::CIdentifier(0xE63CD759, 0xB6ECF6B7)
#define OVP_Algorithm_MatrixAverage_OutputParameterId_AveragedMatrix                        OpenViBE::CIdentifier(0x03CE5AE5, 0xBD9031E0)
#define OVP_Algorithm_MatrixAverage_InputTriggerId_Reset                                    OpenViBE::CIdentifier(0x670EC053, 0xADFE3F5C)
#define OVP_Algorithm_MatrixAverage_InputTriggerId_FeedMatrix                               OpenViBE::CIdentifier(0x50B6EE87, 0xDC42E660)
#define OVP_Algorithm_MatrixAverage_InputTriggerId_ForceAverage                             OpenViBE::CIdentifier(0xBF597839, 0xCD6039F0)
#define OVP_Algorithm_MatrixAverage_OutputTriggerId_AveragePerformed                        OpenViBE::CIdentifier(0x2BFF029B, 0xD932A613)

#define OVP_Algorithm_StimulationBasedEpoching_InputParameterId_InputSignal                 OpenViBE::CIdentifier(0x0ED5C92B, 0xE16BEF25)
#define OVP_Algorithm_StimulationBasedEpoching_InputParameterId_OffsetSampleCount           OpenViBE::CIdentifier(0x7646CE65, 0xE128FC4E)
#define OVP_Algorithm_StimulationBasedEpoching_OutputParameterId_OutputSignal               OpenViBE::CIdentifier(0x00D331A2, 0xC13DF043)
#define OVP_Algorithm_StimulationBasedEpoching_InputTriggerId_Reset                         OpenViBE::CIdentifier(0x6BA44128, 0x418CF901)
#define OVP_Algorithm_StimulationBasedEpoching_InputTriggerId_PerformEpoching               OpenViBE::CIdentifier(0xD05579B5, 0x2649A4B2)
#define OVP_Algorithm_StimulationBasedEpoching_OutputTriggerId_EpochingDone                 OpenViBE::CIdentifier(0x755BC3FE, 0x24F7B50F)
#define OVP_Algorithm_StimulationBasedEpoching_InputParameterId_EndTimeChunkToProcess       OpenViBE::CIdentifier(0x8B552604, 0x10CD1F94)

#define OVP_Algorithm_MatrixVariance_InputParameterId_Matrix                                OpenViBE::CIdentifier(0x781F51CA, 0xE6E3B0B8)
#define OVP_Algorithm_MatrixVariance_InputParameterId_MatrixCount                           OpenViBE::CIdentifier(0xE5103C63, 0x08D825E0)
#define OVP_Algorithm_MatrixVariance_InputParameterId_AveragingMethod                       OpenViBE::CIdentifier(0x043A1BC4, 0x925D3CD6)
#define OVP_Algorithm_MatrixVariance_InputParameterId_SignificanceLevel                     OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_Algorithm_MatrixVariance_OutputParameterId_AveragedMatrix                       OpenViBE::CIdentifier(0x5CF66A73, 0xF5BBF0BF)
#define OVP_Algorithm_MatrixVariance_OutputParameterId_Variance                             OpenViBE::CIdentifier(0x1BD67420, 0x587600E6)
#define OVP_Algorithm_MatrixVariance_OutputParameterId_ConfidenceBound                      OpenViBE::CIdentifier(0x1E1065B2, 0x2CA32013)
#define OVP_Algorithm_MatrixVariance_InputTriggerId_Reset                                   OpenViBE::CIdentifier(0xD5C5EF91, 0xE1B1C4F4)
#define OVP_Algorithm_MatrixVariance_InputTriggerId_FeedMatrix                              OpenViBE::CIdentifier(0xEBAEB213, 0xDD4735A0)
#define OVP_Algorithm_MatrixVariance_InputTriggerId_ForceAverage                            OpenViBE::CIdentifier(0x344A52F5, 0x489DB439)
#define OVP_Algorithm_MatrixVariance_OutputTriggerId_AveragePerformed                       OpenViBE::CIdentifier(0x2F9ECA0B, 0x8D3CA7BD)

#define OVP_Algorithm_ARBurgMethod_InputParameterId_Matrix                                  OpenViBE::CIdentifier(0x36A69669, 0x3651271D)
#define OVP_Algorithm_ARBurgMethod_OutputParameterId_Matrix                                 OpenViBE::CIdentifier(0x55EF8C81, 0x178A51B2)
#define OVP_Algorithm_ARBurgMethod_InputParameterId_UInteger                                OpenViBE::CIdentifier(0x33139BC1, 0x03D30D3B)
#define OVP_Algorithm_ARBurgMethod_InputTriggerId_Initialize                                OpenViBE::CIdentifier(0xC27B06C6, 0xB8EB5F8D)
#define OVP_Algorithm_ARBurgMethod_InputTriggerId_Process                                   OpenViBE::CIdentifier(0xBEEBBE84, 0x4F14F8F8)
#define OVP_Algorithm_ARBurgMethod_OutputTriggerId_ProcessDone                              OpenViBE::CIdentifier(0xA5AAD435, 0x9EC3DB80)

#define OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_SegLength                  OpenViBE::CIdentifier(0xA4826743, 0x0FA27C06)
#define OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_Overlap                    OpenViBE::CIdentifier(0x527F8AEC, 0xA25F2EAB)
#define OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_Window                     OpenViBE::CIdentifier(0x0EA349EE, 0xB9DC95D0)
#define OVP_Algorithm_MagnitudeSquaredCoherence_InputParameterId_Nfft                       OpenViBE::CIdentifier(0x7726C677, 0xE266C5A2)
#define OVP_Algorithm_MagnitudeSquaredCoherence_OutputParameterId_OutputMatrixSpectrum      OpenViBE::CIdentifier(0x331326BA, 0xA94CFC8A)
#define OVP_Algorithm_MagnitudeSquaredCoherence_OutputParameterId_FreqVector                OpenViBE::CIdentifier(0xD9FAA21C, 0x67D7C451)

#define OVP_Algorithm_HilbertTransform_InputParameterId_Matrix                              OpenViBE::CIdentifier(0xC117CE9A, 0x3FFCB156)
#define OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix                      OpenViBE::CIdentifier(0xDAE13CB8, 0xEFF82E69)
#define OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix                     OpenViBE::CIdentifier(0x9D0A023A, 0x7690C48E)
#define OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix                        OpenViBE::CIdentifier(0x495B55E2, 0x8CAAC08E)
#define OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize                            OpenViBE::CIdentifier(0xE4B3CB4A, 0xF0121A20)
#define OVP_Algorithm_HilbertTransform_InputTriggerId_Process                               OpenViBE::CIdentifier(0xC3DC087D, 0x4AAFC1F0)
#define OVP_Algorithm_HilbertTransform_OutputTriggerId_ProcessDone                          OpenViBE::CIdentifier(0xB0B2A2DD, 0x73529B46)

#define OVP_TypeId_Connectivity_Metric                                                      OpenViBE::CIdentifier(0x9188339d, 0x5da83a84)
#define OV_TypeId_ConnectivityMeasure_PsdMethod                                             OpenViBE::CIdentifier(0x93459018, 0xf3ae39d7)
#define OV_TypeId_ConnectivityMeasure_WindowMethod                                          OpenViBE::CIdentifier(0x8815bfa7, 0x557b102f)

#define OV_TypeId_Matrix3dTo2d_SelectionMethod                                              OpenViBE::CIdentifier(0x1dac07fb, 0x05f1ea05)

#define OV_AttributeId_Box_FlagIsUnstable                                                   OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)
