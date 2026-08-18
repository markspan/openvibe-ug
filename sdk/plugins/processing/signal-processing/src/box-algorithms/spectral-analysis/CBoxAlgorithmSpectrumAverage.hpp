///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSpectrumAverage.hpp
/// \brief Classes for the Box Spectrum Average.
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

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmSpectrumAverage final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    // virtual uint64_t getClockFrequency();
    bool initialize() override;
    bool uninitialize() override;
    // virtual bool processClock(Kernel::CMessageClock& msg);
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SpectrumAverage)

protected:
    bool m_bZeroCare = false;

    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::IAlgorithmProxy* m_encoder = nullptr;

    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<CMatrix*> op_matrix;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    std::vector<size_t> m_selectedIndices;
};

class CBoxAlgorithmSpectrumAverageDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spectrum Average"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Computes the average of all the frequency band powers for a spectrum"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Spectral Analysis"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_SpectrumAverage; }
    IPluginObject* create() override { return new CBoxAlgorithmSpectrumAverage; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Spectrum", OV_TypeId_Spectrum);
        prototype.addOutput("Spectrum average", OV_TypeId_StreamedMatrix);
        prototype.addSetting("Considers zeros", OV_TypeId_Boolean, "false");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SpectrumAverageDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
