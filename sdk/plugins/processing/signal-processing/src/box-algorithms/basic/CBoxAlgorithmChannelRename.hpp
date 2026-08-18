///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmChannelRename.hpp
/// \brief Classes for the Box Channel Rename.
/// \author Yann Renard (Inria).
/// \version 1.1.
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

#include <string>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmChannelRename final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ChannelRename)

protected:
    Toolkit::TGenericDecoder<CBoxAlgorithmChannelRename> m_decoder;
    Toolkit::TGenericEncoder<CBoxAlgorithmChannelRename> m_encoder;

    CIdentifier m_typeID = CIdentifier::undefined();

    Kernel::TParameterHandler<CMatrix*> ip_Matrix;
    Kernel::TParameterHandler<CMatrix*> op_Matrix;

    std::vector<std::string> m_names;
};

class CBoxAlgorithmChannelRenameListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        box.setOutputType(index, typeID);
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

class CBoxAlgorithmChannelRenameDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Channel Rename"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Renames channels of different types of streamed matrices"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Channels"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Box_ChannelRename; }
    IPluginObject* create() override { return new CBoxAlgorithmChannelRename; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmChannelRenameListener; }
    void releaseBoxListener(IBoxListener* boxListener) const override { delete boxListener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input matrix", OV_TypeId_Signal);
        prototype.addOutput("Output matrix", OV_TypeId_Signal);
        prototype.addSetting("New channel names", OV_TypeId_String, "Channel 1;Channel 2");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_TimeFrequency);
        prototype.addInputSupport(OV_TypeId_CovarianceMatrix);

        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_TimeFrequency);
        prototype.addOutputSupport(OV_TypeId_CovarianceMatrix);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ChannelRenameDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
