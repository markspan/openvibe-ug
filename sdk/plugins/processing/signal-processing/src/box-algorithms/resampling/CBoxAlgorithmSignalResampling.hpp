///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalResampling.hpp
/// \brief Classes for the Box Signal Resampling.
/// \author Quentin Barthelemy (Mensia Technologies).
/// \version 2.0.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include "ovCResampler.h"

#include <Eigen/Eigen>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
typedef Common::Resampler::CResamplerSd CResampler;

class CBoxAlgorithmSignalResampling final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>, CResampler::ICallback
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    // implementation for TResampler::ICallback
    void processResampler(const double* sample, const size_t nChannel) const override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SignalResampling)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmSignalResampling> m_decoder;
    mutable Toolkit::TSignalEncoder<CBoxAlgorithmSignalResampling> m_encoder;

    size_t m_oSampling = 0;
    size_t m_oNSample  = 0;

    int m_nFractionalDelayFilterSample = 0;
    double m_transitionBandPercent     = 0;
    double m_stopBandAttenuation       = 0;

    size_t m_iSampling              = 0;
    mutable uint64_t m_oTotalSample = 0;
    CResampler m_resampler;
    Kernel::IBoxIO* m_boxContext = nullptr;
};

class CBoxAlgorithmSignalResamplingDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal Resampling"; }
    CString getAuthorName() const override { return "Quentin Barthelemy"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Resamples and re-epochs input signal to chosen sampling frequency"; }

    CString getDetailedDescription() const override
    {
        return "The input signal is resampled, down-sampled or up-sampled, at a chosen sampling frequency and then re-epoched.";
    }

    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "2.0"; }

    CIdentifier getCreatedClass() const override { return Box_SignalResampling; }
    IPluginObject* create() override { return new CBoxAlgorithmSignalResampling; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("New Sampling Frequency", OV_TypeId_Integer, "128", false, SignalResampling_SettingId_NewSampling);
        prototype.addSetting("Sample Count Per Buffer", OV_TypeId_Integer, "8", false, SignalResampling_SettingId_SampleCountPerBuffer);
        // displayed for backward compatibility, but never used
        prototype.addSetting("Low Pass Filter Signal Before Downsampling", OV_TypeId_Boolean, "true", false,
                             SignalResampling_SettingId_LowPassFilterSignalFlag);

        //prototype.addSetting("New Sampling Frequency",OV_TypeId_Integer,"128");
        //prototype.addSetting("Sample Count Per Buffer",OV_TypeId_Integer,"8");
        //prototype.addSetting("Low Pass Filter Signal Before Downsampling", OV_TypeId_Boolean, "true"); // displayed for backward compatibility, but never used

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SignalResamplingDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
