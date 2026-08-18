///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCommonAverageReference.hpp
/// \brief Classes for the Box Common Average Reference.
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

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmCommonAverageReference final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_CommonAverageReference)

protected:
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CMatrix*> op_matrix;
    Kernel::TParameterHandler<uint64_t> op_sampling;

    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    CMatrix m_oMatrix;
};

class CBoxAlgorithmCommonAverageReferenceDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Common Average Reference"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Re-reference the signal to common average reference"; }

    CString getDetailedDescription() const override
    {
        return
                "Re-referencing the signal to common average reference consists in subtracting from each sample the average value of the samples of all electrodes at this time";
    }

    CString getCategory() const override { return "Signal processing/Spatial Filtering"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_CommonAverageReference; }
    IPluginObject* create() override { return new CBoxAlgorithmCommonAverageReference; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_CommonAverageReferenceDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
