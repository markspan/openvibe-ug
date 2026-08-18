///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxIFFT.hpp
/// \brief Class of the box that inverse the Fast Fourier Transform.
/// \author Guillermo Andrade B. (Inria).
/// \version 1.0.
/// \date 20/01/2012.
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

#include <array>
#include <complex>
#include <unsupported/Eigen/FFT>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/// <summary> The class CBoxIFFT describes the box IFFT. </summary>
class CBoxIFFT final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_IFFT)

protected:
    // Codec algorithms specified in the skeleton-generator:
    std::array<Toolkit::TSpectrumDecoder<CBoxIFFT>, 2> m_decoder;
    Toolkit::TSignalEncoder<CBoxIFFT> m_encoder;

    size_t m_nSample  = 0;
    size_t m_nChannel = 0;
    bool m_headerSent = false;

    Eigen::FFT<double> m_fft;                           ///< Instance of the fft transform
    std::vector<std::complex<double>> m_frequencies;
    std::vector<double> m_signal;
};

/// <summary> Descriptor of the box IFFT. </summary>
class CBoxIFFTDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "IFFT"; }
    CString getAuthorName() const override { return "Guillermo Andrade B."; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Compute Inverse Fast Fourier Transformation"; }
    CString getDetailedDescription() const override { return "Compute Inverse Fast Fourier Transformation"; }
    CString getCategory() const override { return "Signal processing/Spectral Analysis"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_IFFT; }
    IPluginObject* create() override { return new CBoxIFFT; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("real part",OV_TypeId_Spectrum);
        prototype.addInput("imaginary part",OV_TypeId_Spectrum);

        prototype.addOutput("Signal output",OV_TypeId_Signal);

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_IFFTDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
