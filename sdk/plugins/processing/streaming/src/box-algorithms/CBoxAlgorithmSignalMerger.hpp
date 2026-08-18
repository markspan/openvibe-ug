///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalMerger.hpp
/// \brief Classes for the Box Signal Merger.
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

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Streaming {
class CBoxAlgorithmSignalMerger final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SignalMerger)

protected:
    std::vector<Toolkit::TSignalDecoder<CBoxAlgorithmSignalMerger>*> m_decoders;
    Toolkit::TSignalEncoder<CBoxAlgorithmSignalMerger>* m_encoder = nullptr;
};

class CBoxAlgorithmSignalMergerListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputName(index, ("Input " + std::to_string(index + 1)).c_str());
        box.setInputType(index, OV_TypeId_Signal);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmSignalMergerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal Merger"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Merges several input streams into a single output stream"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Streaming"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_SignalMerger; }
    IPluginObject* create() override { return new CBoxAlgorithmSignalMerger; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmSignalMergerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input 1", OV_TypeId_Signal);
        prototype.addInput("Input 2", OV_TypeId_Signal);
        prototype.addOutput("Merged", OV_TypeId_Signal);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SignalMergerDesc)
};
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
