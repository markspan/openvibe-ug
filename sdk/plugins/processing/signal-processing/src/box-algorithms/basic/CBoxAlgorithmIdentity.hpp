///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmIdentity.hpp
/// \brief Classes for the Box Identity.
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
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmIdentity final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Identity)
};

class CBoxAlgorithmIdentityListener final : public Toolkit::TBoxListener<IBoxListener>
{
    static bool check(Kernel::IBox& box)
    {
        size_t i;
        for (i = 0; i < box.getInputCount(); ++i) { box.setInputName(i, ("Input stream " + std::to_string(i + 1)).c_str()); }
        for (i = 0; i < box.getOutputCount(); ++i) { box.setOutputName(i, ("Output stream " + std::to_string(i + 1)).c_str()); }
        return true;
    }

public:
    bool onDefaultInitialized(Kernel::IBox& box) override
    {
        box.setInputType(0, OV_TypeId_Signal);
        box.setOutputType(0, OV_TypeId_Signal);
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_Signal);
        box.addOutput("", OV_TypeId_Signal, box.getUnusedInputIdentifier());
        check(box);
        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeOutput(index);
        check(box);
        return true;
    }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        box.setOutputType(index, typeID);
        return true;
    }

    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputType(index, OV_TypeId_Signal);
        box.addInput("", OV_TypeId_Signal, box.getUnusedOutputIdentifier());
        check(box);
        return true;
    }

    bool onOutputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeInput(index);
        check(box);
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        box.setInputType(index, typeID);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmIdentityDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Identity"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria/IRISA"; }
    CString getShortDescription() const override { return "Duplicates input to output"; }
    CString getDetailedDescription() const override { return "This simply duplicates intput on its output"; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_Identity; }
    IPluginObject* create() override { return new CBoxAlgorithmIdentity(); }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmIdentityListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream", OV_TypeId_Signal);
        prototype.addOutput("Output stream", OV_TypeId_Signal);
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_IdentityDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
