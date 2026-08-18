///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmContinuousWaveletAnalysis.hpp
/// \brief Classes for the Box Continuous Wavelet Analysis.
/// \author Quentin Barthelemy (Mensia Technologies).
/// \version 1.0.
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
#include <toolkit/ovtk_all.h>
#include <wavelib/header/wavelib.h>
#include <array>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmContinuousWaveletAnalysis final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ContinuousWaveletAnalysis)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmContinuousWaveletAnalysis> m_decoder;
    std::array<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmContinuousWaveletAnalysis>, 4> m_encoders;

    const char* m_waveletType = nullptr;
    double m_waveletParam     = 0;
    size_t m_nScaleJ          = 0;
    double m_highestFreq      = 0;
    double m_smallestScaleS0  = 0;
    double m_scaleSpacingDj   = 0;

    const char* m_scaleType       = nullptr;
    int m_scalePowerBaseA0        = 0;
    double m_samplingPeriodDt     = 0;
    cwt_object m_waveletTransform = nullptr;
};

class CBoxAlgorithmContinuousWaveletAnalysisDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Continuous Wavelet Analysis"; }
    CString getAuthorName() const override { return "Quentin Barthelemy"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Performs a Time-Frequency Analysis using CWT."; }
    CString getDetailedDescription() const override { return "Performs a Time-Frequency Analysis using Continuous Wavelet Transform."; }
    CString getCategory() const override { return "Signal processing/Spectral Analysis"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_ContinuousWaveletAnalysis; }
    IPluginObject* create() override { return new CBoxAlgorithmContinuousWaveletAnalysis(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);

        prototype.addOutput("Amplitude", OV_TypeId_TimeFrequency);
        prototype.addOutput("Phase", OV_TypeId_TimeFrequency);
        prototype.addOutput("Real Part", OV_TypeId_TimeFrequency);
        prototype.addOutput("Imaginary Part", OV_TypeId_TimeFrequency);

        prototype.addSetting("Wavelet type", TypeId_ContinuousWaveletType, "Morlet wavelet");
        prototype.addSetting("Wavelet parameter", OV_TypeId_Float, "4");
        prototype.addSetting("Number of frequencies", OV_TypeId_Integer, "60");
        prototype.addSetting("Highest frequency", OV_TypeId_Float, "35");
        prototype.addSetting("Frequency spacing", OV_TypeId_Float, "12.5");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ContinuousWaveletAnalysisDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
