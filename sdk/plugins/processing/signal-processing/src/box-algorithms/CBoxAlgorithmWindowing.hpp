///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmWindowing.hpp
/// \brief Classes for the Box Windowing.
/// \author Laurent Bonnet (Mensia Technologies).
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

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmWindowing final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Windowing)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmWindowing> m_decoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmWindowing> m_encoder;

    EWindowMethod m_windowMethod = EWindowMethod::None;
    std::vector<double> m_windowCoefs;
};

class CBoxAlgorithmWindowingDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Windowing"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Applies a windowing function to the signal."; }
    CString getDetailedDescription() const override { return "Applies a windowing function to the signal."; }
    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_Windowing; }
    IPluginObject* create() override { return new CBoxAlgorithmWindowing(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("Window method", TypeId_WindowMethod, "Hamming");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_WindowingDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
