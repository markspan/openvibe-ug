///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmConnectivitySpectrumExtract.hpp
/// \brief Classes of the box ConnectivitySpectrumExtract
/// \author Arthur DESBOIS (INRIA).
/// \version 0.0.1.
/// \date June 28 15:13:00 2022.
///
/// \copyright Copyright (C) 2022 INRIA
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
///-------------------------------------------------------------------------------------------------

#pragma once

#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

/// \brief The class CBoxAlgorithmConnectivitySpectrumExtract describes the box ConnectivitySpectrumExtract.
class CBoxAlgorithmConnectivitySpectrumExtract final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_ConnectivitySpectrumExtract)

protected:
    // Codecs
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmConnectivitySpectrumExtract> m_matrixDecoder;
    Toolkit::TSpectrumEncoder<CBoxAlgorithmConnectivitySpectrumExtract> m_spectrumEncoder;

    // Matrices
    CMatrix* m_iMatrix = nullptr;
    CMatrix* m_oSpectrum = nullptr;

    // Parameters
    size_t m_chan1 = 0;
    size_t m_chan2 = 0;
    uint64_t m_dim1 = 0;
    uint64_t m_dim2 = 0;
    uint64_t m_sampFreq = 500;
    uint64_t m_fftSize = 512;

    CMatrix* m_frequencyAbscissa;


private:
    bool extractVector(const CMatrix& in, CMatrix& out) const;
};


/// \brief Descriptor of the box Matrix 2D to Vector.
class CBoxAlgorithmConnectivitySpectrumExtractDesc final : virtual public IBoxAlgorithmDesc
{
public:

    void release() override {}

    CString getName() const override { return "Connectivity Spectrum Extractor"; }
    CString getAuthorName() const override { return "Arthur Desbois"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Extract spectrum vector from 3D Connectivity Matrix"; }
    CString getDetailedDescription() const override { return "Extract a spectrum vector (coherence, MSC...) from a 3D connectivity matrix, assuming its first dimension is the frequency dim."; }
    CString getCategory() const override { return "Signal processing/Connectivity"; }
    CString getVersion() const override { return "0.0.1"; }
    CString getStockItemName() const override { return ""; }
    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_ConnectivitySpectrumExtract; }
    IPluginObject* create() override { return new CBoxAlgorithmConnectivitySpectrumExtract; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("3d connectivity matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("connectivity", OV_TypeId_Spectrum);

        prototype.addSetting("Channel 1", OV_TypeId_Integer, "0");
        prototype.addSetting("Channel 2", OV_TypeId_Integer, "0");
        prototype.addSetting("Sampling Freq", OV_TypeId_Integer, "500");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ConnectivitySpectrumExtractDesc)
};

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
