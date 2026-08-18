///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmFrequencyBandSelector.hpp
/// \brief Classes for the Box Frequency Band Selector.
/// \author Yann Renard (Inria).
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
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
typedef std::pair<double, double> BandRange;

class CBoxAlgorithmFrequencyBandSelector final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_FrequencyBandSelector)

protected:
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CMatrix*> op_matrix;
    Kernel::TParameterHandler<CMatrix*> op_bands;

    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<CMatrix*> ip_frequencyAbscissa;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    CMatrix m_oMatrix;
    std::vector<BandRange> m_selecteds;
    std::vector<double> m_selectionFactors;
};

class CBoxAlgorithmFrequencyBandSelectorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Frequency Band Selector"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }

    CString getShortDescription() const override
    {
        return "Preserves some spectrum coefficients and puts the others to zero depending on a list of frequencies / frequency bands to select";
    }

    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Spectral Analysis"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_FrequencyBandSelector; }
    IPluginObject* create() override { return new CBoxAlgorithmFrequencyBandSelector; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input spectrum", OV_TypeId_Spectrum);
        prototype.addOutput("Output spectrum", OV_TypeId_Spectrum);
        prototype.addSetting("Frequencies to select", OV_TypeId_String, "8:12;16:24");
        // @fixme Use OV_Value_RangeStringSeparator / OV_Value_EnumeratedStringSeparator tokens above

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_FrequencyBandSelectorDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
