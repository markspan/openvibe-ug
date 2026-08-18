///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for Signal Processing plugin, registering the boxes to OpenViBE
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

#include "algorithms/basic/CAlgorithmMatrixAverage.hpp"
#include "algorithms/basic/CAlgorithmOnlineCovariance.hpp"

#include "box-algorithms/CBoxAlgorithmSignalAverage.hpp"
#include "box-algorithms/CBoxAlgorithmSimpleDSP.hpp"
#include "box-algorithms/CBoxAlgorithmWindowing.hpp"
#include "box-algorithms/CBoxXDAWNTrainer.hpp"

#include "box-algorithms/basic/CBoxAlgorithmChannelRename.hpp"
#include "box-algorithms/basic/CBoxAlgorithmChannelSelector.hpp"
#include "box-algorithms/basic/CBoxAlgorithmCrop.hpp"
#include "box-algorithms/basic/CBoxAlgorithmEpochAverage.hpp"
#include "box-algorithms/basic/CBoxAlgorithmIdentity.hpp"
#include "box-algorithms/basic/CBoxAlgorithmReferenceChannel.hpp"
#include "box-algorithms/basic/CBoxAlgorithmSignalDecimation.hpp"
#include "box-algorithms/basic/CBoxAlgorithmZeroCrossingDetector.hpp"

#include "box-algorithms/epoching/CBoxAlgorithmStimulationBasedEpoching.hpp"
#include "box-algorithms/epoching/CBoxAlgorithmTimeBasedEpoching.hpp"

#include "box-algorithms/filters/CBoxAlgorithmCommonAverageReference.hpp"
#include "box-algorithms/filters/CBoxAlgorithmRegularizedCSPTrainer.hpp"
#include "box-algorithms/filters/CBoxAlgorithmSpatialFilter.hpp"
#include "box-algorithms/filters/CBoxAlgorithmTemporalFilter.hpp"

#include "box-algorithms/resampling/CBoxAlgorithmSignalResampling.hpp"

#include "box-algorithms/spectral-analysis/CBoxAlgorithmContinuousWaveletAnalysis.hpp"
#include "box-algorithms/spectral-analysis/CBoxAlgorithmFrequencyBandSelector.hpp"
#include "box-algorithms/spectral-analysis/CBoxAlgorithmSpectralAnalysis.hpp"
#include "box-algorithms/spectral-analysis/CBoxAlgorithmSpectrumAverage.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationType(TypeId_EpochAverageMethod, "Epoch Average method");
    context.getTypeManager().registerEnumerationEntry(TypeId_EpochAverageMethod, "Moving epoch average", size_t(EEpochAverageMethod::Moving));
    context.getTypeManager().registerEnumerationEntry(TypeId_EpochAverageMethod, "Moving epoch average (Immediate)",
                                                      size_t(EEpochAverageMethod::MovingImmediate));
    context.getTypeManager().registerEnumerationEntry(TypeId_EpochAverageMethod, "Epoch block average", size_t(EEpochAverageMethod::Block));
    context.getTypeManager().registerEnumerationEntry(TypeId_EpochAverageMethod, "Cumulative average", size_t(EEpochAverageMethod::Cumulative));

    context.getTypeManager().registerEnumerationType(TypeId_CropMethod, "Crop method");
    context.getTypeManager().registerEnumerationEntry(TypeId_CropMethod, "Min", size_t(ECropMethod::Min));
    context.getTypeManager().registerEnumerationEntry(TypeId_CropMethod, "Max", size_t(ECropMethod::Max));
    context.getTypeManager().registerEnumerationEntry(TypeId_CropMethod, "Min/Max", size_t(ECropMethod::MinMax));

    context.getTypeManager().registerEnumerationType(TypeId_SelectionMethod, "Selection method");
    context.getTypeManager().registerEnumerationEntry(TypeId_SelectionMethod, "Select", size_t(ESelectionMethod::Select));
    context.getTypeManager().registerEnumerationEntry(TypeId_SelectionMethod, "Reject", size_t(ESelectionMethod::Reject));
    context.getTypeManager().registerEnumerationEntry(TypeId_SelectionMethod, "Select EEG", size_t(ESelectionMethod::Select_EEG));

    context.getTypeManager().registerEnumerationType(TypeId_MatchMethod, "Match method");
    context.getTypeManager().registerEnumerationEntry(TypeId_MatchMethod, "Name", size_t(EMatchMethod::Name));
    context.getTypeManager().registerEnumerationEntry(TypeId_MatchMethod, "Index", size_t(EMatchMethod::Index));
    context.getTypeManager().registerEnumerationEntry(TypeId_MatchMethod, "Smart", size_t(EMatchMethod::Smart));

    // Temporal filter
    context.getTypeManager().registerEnumerationType(TypeId_FilterType, "Filter type");
    context.getTypeManager().registerEnumerationEntry(TypeId_FilterType, toString(EFilterType::BandPass).c_str(), size_t(EFilterType::BandPass));
    context.getTypeManager().registerEnumerationEntry(TypeId_FilterType, toString(EFilterType::BandStop).c_str(), size_t(EFilterType::BandStop));
    context.getTypeManager().registerEnumerationEntry(TypeId_FilterType, toString(EFilterType::HighPass).c_str(), size_t(EFilterType::HighPass));
    context.getTypeManager().registerEnumerationEntry(TypeId_FilterType, toString(EFilterType::LowPass).c_str(), size_t(EFilterType::LowPass));

    OVP_Declare_New(CAlgorithmMatrixAverageDesc)

    OVP_Declare_New(CBoxAlgorithmIdentityDesc)
    OVP_Declare_New(CBoxAlgorithmTimeBasedEpochingDesc)
    OVP_Declare_New(CBoxAlgorithmChannelRenameDesc)
    OVP_Declare_New(CBoxAlgorithmChannelSelectorDesc)
    OVP_Declare_New(CBoxAlgorithmReferenceChannelDesc)
    OVP_Declare_New(CBoxAlgorithmEpochAverageDesc)
    OVP_Declare_New(CBoxAlgorithmCropDesc)
    OVP_Declare_New(CBoxAlgorithmSignalDecimationDesc)
    OVP_Declare_New(CBoxAlgorithmZeroCrossingDetectorDesc)
    OVP_Declare_New(CBoxAlgorithmStimulationBasedEpochingDesc)
    OVP_Declare_New(CBoxAlgorithmCommonAverageReferenceDesc)

    OVP_Declare_New(CBoxAlgorithmSpatialFilterDesc)
    OVP_Declare_New(CBoxAlgorithmTemporalFilterDesc)

    context.getTypeManager().registerEnumerationType(TypeId_OnlineCovariance_UpdateMethod, "Update method");
    context.getTypeManager().registerEnumerationEntry(TypeId_OnlineCovariance_UpdateMethod, "Chunk average", size_t(EUpdateMethod::ChunkAverage));
    context.getTypeManager().registerEnumerationEntry(TypeId_OnlineCovariance_UpdateMethod, "Per sample", size_t(EUpdateMethod::Incremental));

    OVP_Declare_New(CBoxAlgorithmRegularizedCSPTrainerDesc)
    OVP_Declare_New(CAlgorithmOnlineCovarianceDesc)

#if defined TARGET_HAS_R8BRAIN
    OVP_Declare_New(CBoxAlgorithmSignalResamplingDesc)
#endif

    OVP_Declare_New(CBoxAlgorithmSimpleDSPDesc)
    OVP_Declare_New(CSignalAverageDesc)

    // Wavelet Type
    context.getTypeManager().registerEnumerationType(TypeId_ContinuousWaveletType, "Continuous Wavelet Type");
    context.getTypeManager().registerEnumerationEntry(TypeId_ContinuousWaveletType, "Morlet wavelet", size_t(EContinuousWaveletType::Morlet));
    context.getTypeManager().registerEnumerationEntry(TypeId_ContinuousWaveletType, "Paul wavelet", size_t(EContinuousWaveletType::Paul));
    context.getTypeManager().registerEnumerationEntry(TypeId_ContinuousWaveletType, "Derivative of Gaussian wavelet", size_t(EContinuousWaveletType::DOG));

    OVP_Declare_New(CBoxAlgorithmContinuousWaveletAnalysisDesc)

    OVP_Declare_New(CBoxAlgorithmFrequencyBandSelectorDesc)
    OVP_Declare_New(CBoxAlgorithmSpectrumAverageDesc)

    OVP_Declare_New(CBoxAlgorithmSpectralAnalysisDesc)
    OVP_Declare_New(CBoxAlgorithmWindowingDesc)
    context.getTypeManager().registerEnumerationType(TypeId_WindowMethod, "Window method");
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "None", size_t(EWindowMethod::None));
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "Hamming", size_t(EWindowMethod::Hamming));
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "Hanning", size_t(EWindowMethod::Hanning));
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "Hann", size_t(EWindowMethod::Hann));
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "Blackman", size_t(EWindowMethod::Blackman));
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "Triangular", size_t(EWindowMethod::Triangular));
    context.getTypeManager().registerEnumerationEntry(TypeId_WindowMethod, "Square root", size_t(EWindowMethod::SquareRoot));

    OVP_Declare_New(CBoxXDAWNTrainerDesc)

OVP_Declare_End()

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
