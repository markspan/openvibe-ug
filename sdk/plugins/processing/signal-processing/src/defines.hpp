///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \brief Defines of Signal Processing related identifiers.
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

#include <random>
#include <string>

// Boxes
//---------------------------------------------------------------------------------------------------
#define Algorithm_MatrixAverage                         OpenViBE::CIdentifier(0x5E5A6C1C, 0x6F6BEB03)
#define Algorithm_MatrixAverageDesc                     OpenViBE::CIdentifier(0x1992881F, 0xC938C0F2)
#define Algorithm_OnlineCovariance                      OpenViBE::CIdentifier(0x5ADD4F8E, 0x005D29C1)
#define Algorithm_OnlineCovarianceDesc                  OpenViBE::CIdentifier(0x00CD2DEA, 0x4C000CEB)
#define Box_ChannelRename                               OpenViBE::CIdentifier(0x1FE50479, 0x39040F40)
#define Box_ChannelRenameDesc                           OpenViBE::CIdentifier(0x20EA1F00, 0x7AED5645)
#define Box_ChannelSelector                             OpenViBE::CIdentifier(0x361722E8, 0x311574E8)
#define Box_ChannelSelectorDesc                         OpenViBE::CIdentifier(0x67633C1C, 0x0D610CD8)
#define Box_CommonAverageReference                      OpenViBE::CIdentifier(0x009C0CE3, 0x6BDF71C3)
#define Box_CommonAverageReferenceDesc                  OpenViBE::CIdentifier(0x0033EAF8, 0x09C65E4E)
#define Box_ContinuousWaveletAnalysis                   OpenViBE::CIdentifier(0x0A43133D, 0x6EAF25A7)
#define Box_ContinuousWaveletAnalysisDesc               OpenViBE::CIdentifier(0x5B397A82, 0x76AE6F81)
#define Box_Crop                                        OpenViBE::CIdentifier(0x7F1A3002, 0x358117BA)
#define Box_CropDesc                                    OpenViBE::CIdentifier(0x64D619D7, 0x26CC42C9)
#define Box_EpochAverage                                OpenViBE::CIdentifier(0x21283D9F, 0xE76FF640)
#define Box_EpochAverageDesc                            OpenViBE::CIdentifier(0x95F5F43E, 0xBE629D82)
#define Box_FrequencyBandSelector                       OpenViBE::CIdentifier(0x140C19C6, 0x4E6E187B)
#define Box_FrequencyBandSelectorDesc                   OpenViBE::CIdentifier(0x13462C56, 0x794E3C07)
#define Box_Identity                                    OpenViBE::CIdentifier(0x5DFFE431, 0x35215C50)
#define Box_IdentityDesc                                OpenViBE::CIdentifier(0x54743810, 0x6A1A88CC)
#define Box_ReferenceChannel                            OpenViBE::CIdentifier(0x444721AD, 0x78342CF5)
#define Box_ReferenceChannelDesc                        OpenViBE::CIdentifier(0x42856103, 0x45B125AD)
#define Box_RegularizedCSPTrainer                       OpenViBE::CIdentifier(0x2EC14CC0, 0x428C48BD)
#define Box_RegularizedCSPTrainerDesc                   OpenViBE::CIdentifier(0x02205F54, 0x733C51EE)
#define Box_SignalAverage                               OpenViBE::CIdentifier(0x00642C4D, 0x5DF7E50A)
#define Box_SignalAverageDesc                           OpenViBE::CIdentifier(0x007CDCE9, 0x16034F77)
#define Box_SignalDecimation                            OpenViBE::CIdentifier(0x012F4BEA, 0x3BE37C66)
#define Box_SignalDecimationDesc                        OpenViBE::CIdentifier(0x1C5F1356, 0x1E685777)
#define Box_SignalResampling                            OpenViBE::CIdentifier(0x0E923A5E, 0xDA474058)
#define Box_SignalResamplingDesc                        OpenViBE::CIdentifier(0xA675A433, 0xC6690920)
#define Box_SimpleDSP                                   OpenViBE::CIdentifier(0x00E26FA1, 0x1DBAB1B2)
#define Box_SimpleDSPDesc                               OpenViBE::CIdentifier(0x00C44BFE, 0x76C9269E)
#define Box_SpatialFilter                               OpenViBE::CIdentifier(0xDD332C6C, 0x195B4FD4)
#define Box_SpatialFilterDesc                           OpenViBE::CIdentifier(0x72A01C92, 0xF8C1FA24)
#define Box_SpectralAnalysis                            OpenViBE::CIdentifier(0x84218FF8, 0xA87E7995)
#define Box_SpectralAnalysisDesc                        OpenViBE::CIdentifier(0x0051E63C, 0x68E83AD1)
#define Box_SpectrumAverage                             OpenViBE::CIdentifier(0x0C092665, 0x61B82641)
#define Box_SpectrumAverageDesc                         OpenViBE::CIdentifier(0x24663D96, 0x71EA7295)
#define Box_StimulationBasedEpoching                    OpenViBE::CIdentifier(0x426163D1, 0x324237B0)
#define Box_StimulationBasedEpochingDesc                OpenViBE::CIdentifier(0x4F60616D, 0x468E0A8C)
#define Box_TemporalFilter                              OpenViBE::CIdentifier(0xB4F9D042, 0x9D79F2E5)
#define Box_TemporalFilterDesc                          OpenViBE::CIdentifier(0x7BF6BA62, 0xAF829A37)
#define Box_TimeBasedEpoching                           OpenViBE::CIdentifier(0x00777FA0, 0x5DC3F560)
#define Box_TimeBasedEpochingDesc                       OpenViBE::CIdentifier(0x00ABDABE, 0x41381683)
#define Box_Windowing                                   OpenViBE::CIdentifier(0x002034AE, 0x6509FD8F)
#define Box_WindowingDesc                               OpenViBE::CIdentifier(0x602CF89F, 0x65BA6DA0)
#define Box_InriaXDAWNTrainer                           OpenViBE::CIdentifier(0x27542F6E, 0x14AA3548)
#define Box_InriaXDAWNTrainerDesc                       OpenViBE::CIdentifier(0x128A6013, 0x370B5C2C)
#define Box_ZeroCrossingDetector                        OpenViBE::CIdentifier(0x0016663F, 0x096A46A6)
#define Box_ZeroCrossingDetectorDesc                    OpenViBE::CIdentifier(0x63AA73A7, 0x1F0419A2)

// Type definitions
//---------------------------------------------------------------------------------------------------
// Filter method identifiers from OpenViBE 0.14.0
#define TypeId_FilterMethod                             OpenViBE::CIdentifier(0x2F2C606C, 0x8512ED68)
#define TypeId_FilterType                               OpenViBE::CIdentifier(0xFA20178E, 0x4CBA62E9)
#define TypeId_OnlineCovariance_UpdateMethod            OpenViBE::CIdentifier(0x59E83F33, 0x592F1DD0)
#define TypeId_EpochAverageMethod                       OpenViBE::CIdentifier(0x6530BDB1, 0xD057BBFE)
#define TypeId_ContinuousWaveletType                    OpenViBE::CIdentifier(0x09177469, 0x52404583)
#define TypeId_CropMethod                               OpenViBE::CIdentifier(0xD0643F9E, 0x8E35FE0A)
#define TypeId_SelectionMethod                          OpenViBE::CIdentifier(0x3BCF9E67, 0x0C23994D)
#define TypeId_MatchMethod                              OpenViBE::CIdentifier(0x666F25E9, 0x3E5738D6)
#define TypeId_WindowMethod                             OpenViBE::CIdentifier(0x0A430FE4, 0x4F318280)


//enum class EFilterMethod { Butterworth, Chebyshev, YuleWalker };

//--------------------------------------------------------------------------------
enum class EFilterType { LowPass, BandPass, HighPass, BandStop };

/// <summary>   Convert filter type to string. </summary>
/// <param name="type"> The Filter type. </param>
/// <returns>   <c>std::string</c> </returns>
inline std::string toString(const EFilterType type)
{
    switch (type) {
        case EFilterType::BandPass: return "Band Pass";
        case EFilterType::BandStop: return "Band Stop";
        case EFilterType::HighPass: return "High Pass";
        case EFilterType::LowPass: return "Low Pass";
        default: return "Invalid Filter Type";
    }
}

/// <summary>   Convert string to filter type. </summary>
/// <param name="type"> The Filter type. </param>
/// <returns>   <see cref="EFilterType"/> </returns>
inline EFilterType StringToFilterType(const std::string& type)
{
    if (type == "Band Pass") { return EFilterType::BandPass; }
    if (type == "Band Stop") { return EFilterType::BandStop; }
    if (type == "High Pass") { return EFilterType::HighPass; }
    if (type == "Low Pass") { return EFilterType::LowPass; }
    return EFilterType::BandPass;
}

//--------------------------------------------------------------------------------
enum class EUpdateMethod { ChunkAverage, Incremental };

//--------------------------------------------------------------------------------
enum class EEpochAverageMethod { Moving, MovingImmediate, Block, Cumulative };

//--------------------------------------------------------------------------------
enum class EContinuousWaveletType { Morlet, Paul, DOG };

//--------------------------------------------------------------------------------
enum class ECropMethod { Min, Max, MinMax };

//--------------------------------------------------------------------------------
enum class ESelectionMethod { Select, Reject, Select_EEG };

//--------------------------------------------------------------------------------
enum class EMatchMethod { Name, Index, Smart };

//--------------------------------------------------------------------------------
enum class EWindowMethod { None, Hamming, Hanning, Hann, Blackman, Triangular, SquareRoot };

// Global defines
//---------------------------------------------------------------------------------------------------
#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#define OVP_Value_CoupledStringSeparator                                    '-'
//#define OVP_Value_AllSelection                                            '*'

#define MatrixAverage_InputParameterId_Matrix                   OpenViBE::CIdentifier(0x913E9C3B, 0x8A62F5E3)
#define MatrixAverage_InputParameterId_MatrixCount              OpenViBE::CIdentifier(0x08563191, 0xE78BB265)
#define MatrixAverage_InputParameterId_AveragingMethod          OpenViBE::CIdentifier(0xE63CD759, 0xB6ECF6B7)
#define MatrixAverage_OutputParameterId_AveragedMatrix          OpenViBE::CIdentifier(0x03CE5AE5, 0xBD9031E0)
#define MatrixAverage_InputTriggerId_Reset                      OpenViBE::CIdentifier(0x670EC053, 0xADFE3F5C)
#define MatrixAverage_InputTriggerId_FeedMatrix                 OpenViBE::CIdentifier(0x50B6EE87, 0xDC42E660)
#define MatrixAverage_InputTriggerId_ForceAverage               OpenViBE::CIdentifier(0xBF597839, 0xCD6039F0)
#define MatrixAverage_OutputTriggerId_AveragePerformed          OpenViBE::CIdentifier(0x2BFF029B, 0xD932A613)
#define OnlineCovariance_InputParameterId_Shrinkage             OpenViBE::CIdentifier(0x16577C7B, 0x4E056BF7)
#define OnlineCovariance_InputParameterId_InputVectors          OpenViBE::CIdentifier(0x47E55F81, 0x27A519C4)
#define OnlineCovariance_InputParameterId_UpdateMethod          OpenViBE::CIdentifier(0x1C4F444F, 0x3CA213E2)
#define OnlineCovariance_InputParameterId_TraceNormalization    OpenViBE::CIdentifier(0x269D5E63, 0x3B6D486E)
#define OnlineCovariance_OutputParameterId_Mean                 OpenViBE::CIdentifier(0x3F1F50A3, 0x05504D0E)
#define OnlineCovariance_OutputParameterId_CovarianceMatrix     OpenViBE::CIdentifier(0x203A5472, 0x67C5324C)
#define OnlineCovariance_Process_Reset                          OpenViBE::CIdentifier(0x4C1C510C, 0x3CF56E7C) // to reset estimates to 0
#define OnlineCovariance_Process_Update                         OpenViBE::CIdentifier(0x72BF2277, 0x2974747B) // update estimates with a new chunk of data
#define OnlineCovariance_Process_GetCov                         OpenViBE::CIdentifier(0x2BBC4A91, 0x27050CFD) // also returns the mean estimate
#define OnlineCovariance_Process_GetCovRaw                      OpenViBE::CIdentifier(0x0915148C, 0x5F792B2A) // also returns the mean estimate
#define SignalResampling_SettingId_NewSampling                  OpenViBE::CIdentifier(0x158A8EFD, 0xAA894F86)
#define SignalResampling_SettingId_SampleCountPerBuffer         OpenViBE::CIdentifier(0x588783F3, 0x8E8DCF86)
#define SignalResampling_SettingId_LowPassFilterSignalFlag      OpenViBE::CIdentifier(0xAFDD8EFD, 0x23EF94F6)

template <typename T>
T Random(T min, T max)
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_real_distribution<T> uni(min, max);
    return uni(rng);
}
