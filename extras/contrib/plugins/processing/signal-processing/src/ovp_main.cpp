#include <vector>
#include <openvibe/ov_all.h>
#include "ovp_defines.h"

#include "box-algorithms/ovpCBoxAlgorithmCSPSpatialFilterTrainer.h"     // ghent univ
#include "algorithms/ovpCAlgorithmUnivariateStatistics.h"               // gipsa
#include "box-algorithms/ovpCBoxAlgorithmUnivariateStatistics.h"        // gipsa
#include "box-algorithms/ovpCBoxAlgorithmSynchro.h"                     // gipsa

// @BEGIN inserm-gpl
#include "algorithms/ovpCDetectingMinMax.h"
#include "box-algorithms/ovpCDetectingMinMaxBoxAlgorithm.h"

#include "box-algorithms/ovpCWindowingFunctions.h"
#include "box-algorithms/ovpCFastICA.h"
#include "box-algorithms/ovpCSpectralAnalysis.h"

#include "algorithms/ovpCApplyTemporalFilter.h"
#include "algorithms/ovpCComputeTemporalFilterCoefficients.h"
#include "box-algorithms/ovpCTemporalFilterBoxAlgorithm.h"
#include "box-algorithms/ovpCModTemporalFilterBoxAlgorithm.h"

#include "algorithms/ovpCDownsampling.h"
#include "box-algorithms/ovpCDownsamplingBoxAlgorithm.h"

#include "algorithms/ovpCDetectingMinMax.h"
#include "box-algorithms/ovpCDetectingMinMaxBoxAlgorithm.h"
// @END inserm-gpl

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

OVP_Declare_Begin()
#if defined TARGET_HAS_ThirdPartyITPP
    OVP_Declare_New(CBoxAlgorithmCSPSpatialFilterTrainerDesc) // ghent univ
#endif

    OVP_Declare_New(CBoxAlgorithmSynchroDesc)       // gipsa
    OVP_Declare_New(CAlgoUnivariateStatisticDesc)   // gipsa
    OVP_Declare_New(CBoxUnivariateStatisticDesc)    // gipsa


    // @BEGIN inserm-gpl
    context.getTypeManager().registerBitMaskType(OVP_TypeId_SpectralComponent, "Spectral component");
    context.getTypeManager().registerBitMaskEntry(OVP_TypeId_SpectralComponent, "Amplitude", size_t(ESpectralComponent::Amplitude));
    context.getTypeManager().registerBitMaskEntry(OVP_TypeId_SpectralComponent, "Phase", size_t(ESpectralComponent::Phase));
    context.getTypeManager().registerBitMaskEntry(OVP_TypeId_SpectralComponent, "Real part", size_t(ESpectralComponent::RealPart));
    context.getTypeManager().registerBitMaskEntry(OVP_TypeId_SpectralComponent, "Imaginary part", size_t(ESpectralComponent::ImaginaryPart));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_FilterMethod, "Filter method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FilterMethod, "Butterworth", size_t(EFilterMethod::Butterworth));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FilterMethod, "Chebyshev", size_t(EFilterMethod::Chebyshev));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_FilterType, "Filter type");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FilterType, "Band Pass", size_t(EFilterType::BandPass));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FilterType, "Band Stop", size_t(EFilterType::BandStop));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FilterType, "High Pass", size_t(EFilterType::HighPass));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FilterType, "Low Pass", size_t(EFilterType::LowPass));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_WindowMethod, "Window method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WindowMethod, "Hamming", size_t(EWindowMethod::Hamming));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WindowMethod, "Hanning", size_t(EWindowMethod::Hanning));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WindowMethod, "Hann", size_t(EWindowMethod::Hann));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WindowMethod, "Blackman", size_t(EWindowMethod::Blackman));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WindowMethod, "Triangular", size_t(EWindowMethod::Triangular));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WindowMethod, "Square root", size_t(EWindowMethod::SquareRoot));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_FrequencyCutOffRatio, "Frequency cut off ratio");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FrequencyCutOffRatio, "1/4", size_t(EFrequencyCutOffRatio::R14));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FrequencyCutOffRatio, "1/3", size_t(EFrequencyCutOffRatio::R13));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FrequencyCutOffRatio, "1/2", size_t(EFrequencyCutOffRatio::R12));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_MinMax, "Min/Max");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_MinMax, "Min", size_t(EMinMax::Min));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_MinMax, "Max", size_t(EMinMax::Max));

    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

#if defined TARGET_HAS_ThirdPartyITPP

    OVP_Declare_New(CSpectralAnalysisDesc)
    OVP_Declare_New(CFastICADesc)
    context.getTypeManager().registerEnumerationType(OVP_TypeId_FastICA_OperatingMode, "Operating mode");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_OperatingMode, "PCA", size_t(EFastICAMode::PCA));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_OperatingMode, "Whiten", size_t(EFastICAMode::Whiten));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_OperatingMode, "ICA", size_t(EFastICAMode::ICA));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_FastICA_DecompositionType, "Decomposition type");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_DecompositionType, "Symmetric", size_t(EFastICADecomposition::Symmetric));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_DecompositionType, "Deflate", size_t(EFastICADecomposition::Deflate));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_FastICA_Nonlinearity, "Nonlinearity");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_Nonlinearity, "Pow3", size_t(EFastICANonlinearity::Pow3));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_Nonlinearity, "Tanh", size_t(EFastICANonlinearity::Tanh));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_Nonlinearity, "Gauss", size_t(EFastICANonlinearity::Gauss));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_FastICA_Nonlinearity, "Skew", size_t(EFastICANonlinearity::Skew));

    OVP_Declare_New(CWindowingFunctionsDesc)
    OVP_Declare_New(CComputeTemporalFilterCoefficientsDesc)
    OVP_Declare_New(CTemporalFilterBoxAlgorithmDesc)
    OVP_Declare_New(CModTemporalFilterBoxAlgorithmDesc)
    OVP_Declare_New(CApplyTemporalFilterDesc)
#endif // TARGET_HAS_ThirdPartyITPP

    OVP_Declare_New(CDownsamplingDesc)
    OVP_Declare_New(CDownsamplingBoxAlgorithmDesc)
    OVP_Declare_New(CDetectingMinMaxDesc)
    OVP_Declare_New(CDetectingMinMaxBoxAlgorithmDesc)
    // @END inserm-gpl

OVP_Declare_End()

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
