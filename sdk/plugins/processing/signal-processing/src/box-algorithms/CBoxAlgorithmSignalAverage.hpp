///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalAverage.hpp
/// \brief Classes for the Box Signal average.
/// \author Bruno Renier (Inria).
/// \version 0.5.
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
/**
*/
class CBoxAlgorithmSignalAverage final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmSignalAverage() {}
    void release() override {}
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SignalAverage)

protected:
    void computeAverage();

    // Needed to read the input and write the output
    Toolkit::TSignalDecoder<CBoxAlgorithmSignalAverage> m_decoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmSignalAverage> m_encoder;
};

/**
* Description of the channel selection plugin
*/
class CSignalAverageDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal average"; }
    CString getAuthorName() const override { return "Bruno Renier"; }
    CString getAuthorCompanyName() const override { return "Inria/IRISA"; }
    CString getShortDescription() const override { return "Computes the average of each input buffer."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Averaging"; }
    CString getVersion() const override { return "0.5"; }

    CIdentifier getCreatedClass() const override { return Box_SignalAverage; }
    IPluginObject* create() override { return new CBoxAlgorithmSignalAverage(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Filtered signal", OV_TypeId_Signal);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SignalAverageDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
