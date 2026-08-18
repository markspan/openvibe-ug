///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSpectralAnalysis.hpp
/// \brief Classes for the Box Spectral Analysis.
/// \author Laurent Bonnet / Quentin Barthelemy (Mensia Technologies).
/// \version 1.2.
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

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmSpectralAnalysis final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SpectralAnalysis)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmSpectralAnalysis> m_decoder;

    std::vector<Toolkit::TSpectrumEncoder<CBoxAlgorithmSpectralAnalysis>*> m_spectrumEncoders;
    std::vector<bool> m_isSpectrumEncoderActive;

    size_t m_nChannel = 0;
    size_t m_nSample  = 0;
    size_t m_sampling = 0;

    size_t m_sizeFFT = 0;

    CMatrix* m_frequencyAbscissa = nullptr;
};

class CBoxAlgorithmSpectralAnalysisDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spectral Analysis"; }
    CString getAuthorName() const override { return "Laurent Bonnet / Quentin Barthelemy"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Performs a Spectral Analysis using FFT."; }
    CString getDetailedDescription() const override { return "Performs a Spectral Analysis using FFT."; }
    CString getCategory() const override { return "Signal processing/Spectral Analysis"; }
    CString getVersion() const override { return "1.2"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_SpectralAnalysis; }
    IPluginObject* create() override { return new CBoxAlgorithmSpectralAnalysis(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);

        prototype.addOutput("Amplitude", OV_TypeId_Spectrum);
        prototype.addOutput("Phase", OV_TypeId_Spectrum);
        prototype.addOutput("Real Part", OV_TypeId_Spectrum);
        prototype.addOutput("Imaginary Part", OV_TypeId_Spectrum);

        prototype.addSetting("Amplitude", OV_TypeId_Boolean, "true");
        prototype.addSetting("Phase", OV_TypeId_Boolean, "false");
        prototype.addSetting("Real Part", OV_TypeId_Boolean, "false");
        prototype.addSetting("Imaginary Part", OV_TypeId_Boolean, "false");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SpectralAnalysisDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
