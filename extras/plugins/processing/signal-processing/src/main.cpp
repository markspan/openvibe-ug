#include "defines.hpp"

#include "algorithms/basic/ovpCAlgorithmARBurgMethod.h"
#include "algorithms/basic/ovpCHilbertTransform.h"

#include "box-algorithms/basic/CBoxAlgorithmMatrix3dTo2d.hpp"
#include "box-algorithms/basic/CBoxAlgorithmMatrix2dToVector.hpp"
#include "box-algorithms/ovpCBoxAlgorithmDiscreteWaveletTransform.h"
#include "box-algorithms/ovpCBoxAlgorithmEOG_Denoising.h"
#include "box-algorithms/ovpCBoxAlgorithmEOG_Denoising_Calibration.h"
#include "box-algorithms/ovpCBoxAlgorithmQuadraticForm.h"
#include "box-algorithms/CBoxAlgorithmInverse_DWT.hpp"

#include "box-algorithms/basic/CBoxEpochVariance.hpp"
#include "box-algorithms/basic/CBoxIFFT.hpp"
#include "box-algorithms/basic/ovpCBoxAlgorithmARCoefficients.h"
#include "box-algorithms/basic/ovpCBoxAlgorithmDifferentialIntegral.h"
#include "box-algorithms/basic/ovpCBoxAlgorithmERSPAverage.h"
#include "box-algorithms/basic/ovpCBoxAlgorithmHilbert.h"
#include "box-algorithms/basic/ovpCBoxAlgorithmMatrixTranspose.h"
//#include "box-algorithms/basic/ovpCBoxAlgorithmNull.h"

#include "box-algorithms/connectivity/CBoxAlgorithmConnectivityMeasure.hpp"
#include "box-algorithms/connectivity/CBoxAlgorithmConnectivitySpectrumExtract.hpp"

#include "box-algorithms/CBoxAlgorithmPulseRateCalculator.hpp"
#include "box-algorithms/CBoxAlgorithmEntropyMeasure.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

OVP_Declare_Begin()
    //*********** Boxes ***********
    OVP_Declare_New(CBoxAlgorithmDifferentialIntegralDesc)
    OVP_Declare_New(CBoxAlgorithmMatrixTransposeDesc)
    //OVP_Declare_New(CBoxAlgorithmNullDesc)
    OVP_Declare_New(CBoxAlgorithmERSPAverageDesc)
    OVP_Declare_New(CBoxAlgorithmQuadraticFormDesc)
    OVP_Declare_New(CBoxEpochVarianceDesc)
    OVP_Declare_New(CBoxAlgorithmMatrix3dTo2dDesc)
    OVP_Declare_New(CBoxAlgorithmMatrix2dToVectorDesc)
    OVP_Declare_New(CBoxAlgorithmConnectivitySpectrumExtractDesc)


    OVP_Declare_New(CBoxAlgorithmARCoefficientsDesc)
    OVP_Declare_New(CAlgorithmARBurgMethodDesc)
    OVP_Declare_New(CAlgorithmHilbertTransformDesc)
    OVP_Declare_New(CBoxAlgorithmConnectivityMeasureDesc)
    OVP_Declare_New(CBoxAlgorithmEOG_DenoisingDesc)
    OVP_Declare_New(CBoxAlgorithmEOG_Denoising_CalibrationDesc)
    OVP_Declare_New(CBoxAlgorithmHilbertDesc)
    OVP_Declare_New(CBoxIFFTDesc)

    OVP_Declare_New(CBoxAlgorithmPulseRateCalculatorDesc)
    OVP_Declare_New(CBoxAlgorithmEntropyMeasureDesc)


#if defined(TARGET_HAS_ThirdPartyFFTW3)
    OVP_Declare_New(CBoxAlgorithmDiscreteWaveletTransformDesc)
    OVP_Declare_New(CBoxAlgorithmInverse_DWTDesc)

    //*********** Enumeration ***********
    context.getTypeManager().registerEnumerationType(OVP_TypeId_WaveletType, "Wavelet type");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "haar", size_t(EWaveletType::Haar));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db1", size_t(EWaveletType::Db1));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db2", size_t(EWaveletType::Db2));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db3", size_t(EWaveletType::Db3));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db4", size_t(EWaveletType::Db4));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db5", size_t(EWaveletType::Db5));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db6", size_t(EWaveletType::Db6));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db7", size_t(EWaveletType::Db7));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db8", size_t(EWaveletType::Db8));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db9", size_t(EWaveletType::Db9));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db10", size_t(EWaveletType::Db10));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db11", size_t(EWaveletType::Db11));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db12", size_t(EWaveletType::Db12));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db13", size_t(EWaveletType::Db13));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db14", size_t(EWaveletType::Db14));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "db15", size_t(EWaveletType::Db15));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior1.1", size_t(EWaveletType::Bior11));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior1.3", size_t(EWaveletType::Bior13));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior1.5", size_t(EWaveletType::Bior15));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior2.2", size_t(EWaveletType::Bior22));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior2.4", size_t(EWaveletType::Bior24));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior2.6", size_t(EWaveletType::Bior26));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior2.8", size_t(EWaveletType::Bior28));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior3.1", size_t(EWaveletType::Bior31));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior3.3", size_t(EWaveletType::Bior33));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior3.5", size_t(EWaveletType::Bior35));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior3.7", size_t(EWaveletType::Bior37));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior3.9", size_t(EWaveletType::Bior39));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior4.4", size_t(EWaveletType::Bior44));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior5.5", size_t(EWaveletType::Bior55));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "bior6.8", size_t(EWaveletType::Bior68));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "coif1", size_t(EWaveletType::Coif1));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "coif2", size_t(EWaveletType::Coif2));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "coif3", size_t(EWaveletType::Coif3));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "coif4", size_t(EWaveletType::Coif4));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "coif5", size_t(EWaveletType::Coif5));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym1", size_t(EWaveletType::Sym1));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym2", size_t(EWaveletType::Sym2));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym3", size_t(EWaveletType::Sym3));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym4", size_t(EWaveletType::Sym4));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym5", size_t(EWaveletType::Sym5));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym6", size_t(EWaveletType::Sym6));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym7", size_t(EWaveletType::Sym7));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym8", size_t(EWaveletType::Sym8));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym9", size_t(EWaveletType::Sym9));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletType, "sym10", size_t(EWaveletType::Sym10));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_WaveletLevel, "Wavelet decomposition levels");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletLevel, "1", size_t(EWaveletLevel::L1));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletLevel, "2", size_t(EWaveletLevel::L2));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletLevel, "3", size_t(EWaveletLevel::L3));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletLevel, "4", size_t(EWaveletLevel::L4));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_WaveletLevel, "5", size_t(EWaveletLevel::L5));
#endif
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

    context.getTypeManager().registerEnumerationType(OVP_TypeId_EpochAverageMethod, "Epoch Average method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Moving epoch average", size_t(EEpochAverageMethod::Moving));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Moving epoch average (Immediate)",
                                                      size_t(EEpochAverageMethod::MovingImmediate));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Epoch block average", size_t(EEpochAverageMethod::Block));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_EpochAverageMethod, "Cumulative average", size_t(EEpochAverageMethod::Cumulative));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_CropMethod, "Crop method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_CropMethod, "Min", size_t(ECropMethod::Min));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_CropMethod, "Max", size_t(ECropMethod::Max));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_CropMethod, "Min/Max", size_t(ECropMethod::MinMax));


    context.getTypeManager().registerEnumerationType(OVP_TypeId_SelectionMethod, "Selection method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SelectionMethod, "Select", size_t(ESelectionMethod::Select));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SelectionMethod, "Reject", size_t(ESelectionMethod::Reject));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_MatchMethod, "Match method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_MatchMethod, "Name", size_t(EMatchMethod::Name));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_MatchMethod, "Index", size_t(EMatchMethod::Index));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_MatchMethod, "Smart", size_t(EMatchMethod::Smart));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_DifferentialIntegralOperation, "Differential/Integral select");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_DifferentialIntegralOperation, "Differential",
                                                      size_t(EDifferentialIntegralOperation::Differential));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_DifferentialIntegralOperation, "Integral", size_t(EDifferentialIntegralOperation::Integral));


    context.getTypeManager().registerEnumerationType(OV_TypeId_ConnectivityMeasure_PsdMethod, "PSD Method");
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_ConnectivityMeasure_PsdMethod, "Welch", size_t(EPsdMode::Welch));
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_ConnectivityMeasure_PsdMethod, "Burg", size_t(EPsdMode::Burg));

    context.getTypeManager().registerEnumerationType(OV_TypeId_ConnectivityMeasure_WindowMethod, "Welch Window method");
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_ConnectivityMeasure_WindowMethod, "Hamming", size_t(EConnectWindowMethod::Hamming));
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_ConnectivityMeasure_WindowMethod, "Hann", size_t(EConnectWindowMethod::Hann));
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_ConnectivityMeasure_WindowMethod, "Welch", size_t(EConnectWindowMethod::Welch));

    context.getTypeManager().registerEnumerationType(OVP_TypeId_Connectivity_Metric, "Metric method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_Connectivity_Metric, toString(EConnectMetric::Coherence).c_str(), size_t(EConnectMetric::Coherence));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_Connectivity_Metric, toString(EConnectMetric::MagnitudeSquaredCoherence).c_str(), size_t(EConnectMetric::MagnitudeSquaredCoherence));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_Connectivity_Metric, toString(EConnectMetric::ImaginaryCoherence).c_str(), size_t(EConnectMetric::ImaginaryCoherence));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_Connectivity_Metric, toString(EConnectMetric::AbsImaginaryCoherence).c_str(), size_t(EConnectMetric::AbsImaginaryCoherence));

    context.getTypeManager().registerEnumerationType(OV_TypeId_Matrix3dTo2d_SelectionMethod, "Selection Method");
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_Matrix3dTo2d_SelectionMethod, "Select", size_t(ESelectionMode::Select));
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_Matrix3dTo2d_SelectionMethod, "Average", size_t(ESelectionMode::Average));

OVP_Declare_End()

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
