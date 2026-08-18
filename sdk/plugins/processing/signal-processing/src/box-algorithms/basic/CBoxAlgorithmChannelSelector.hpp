///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmChannelSelector.hpp
/// \brief Classes for the Box Channel Selector.
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

#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmChannelSelector final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ChannelSelector)

protected:
    Toolkit::TDecoder<CBoxAlgorithmChannelSelector>* m_decoder = nullptr;
    Toolkit::TEncoder<CBoxAlgorithmChannelSelector>* m_encoder = nullptr;

    CMatrix* m_iMatrix = nullptr;
    CMatrix* m_oMatrix = nullptr;

    std::vector<size_t> m_vLookup;
};

class CBoxAlgorithmChannelSelectorListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onOutputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(0, typeID);
        if (typeID == OV_TypeId_Signal || typeID == OV_TypeId_Spectrum || typeID == OV_TypeId_StreamedMatrix) {
            box.setInputType(0, typeID);
            return true;
        }
        box.getInputType(0, typeID);
        box.setOutputType(0, typeID);
        OV_ERROR_KRF("Invalid output type [" << typeID.str() << "] (expected Signal, Spectrum or Streamed Matrix)", Kernel::ErrorType::BadOutput);
    }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(0, typeID);
        if (typeID == OV_TypeId_Signal || typeID == OV_TypeId_Spectrum || typeID == OV_TypeId_StreamedMatrix) {
            box.setOutputType(0, typeID);
            return true;
        }
        box.getOutputType(0, typeID);
        box.setInputType(0, typeID);

        OV_ERROR_KRF("Invalid input type [" << typeID.str() << "] (expected Signal, Spectrum or Streamed Matrix)", Kernel::ErrorType::BadInput);
    }

    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        //we are only interested in the setting 0 and the type changes (select or reject)
        if ((index == 0 || index == 1) && (!m_hasUserSetName)) {
            CString channels;
            box.getSettingValue(0, channels);

            CString method;
            CIdentifier enumID = CIdentifier::undefined();
            box.getSettingValue(1, method);
            box.getSettingType(1, enumID);

            const ESelectionMethod methodID = ESelectionMethod(this->getTypeManager().getEnumerationEntryValueFromName(enumID, method));

            if (methodID == ESelectionMethod::Reject) { channels = CString("!") + channels; }
            box.setName(channels);
        }
        return true;
    }

    bool onNameChanged(Kernel::IBox& box) override
    //when user set box name manually
    {
        if (m_hasUserSetName) {
            const CString rename = box.getName();
            if (rename == CString("Channel Selector")) {//default name, we switch back to default behaviour
                m_hasUserSetName = false;
            }
        }
        else { m_hasUserSetName = true; }
        return true;
    }

    bool initialize() override
    {
        m_hasUserSetName = false;//need to initialize this value
        return true;
    }

private:
    bool m_hasUserSetName = false;

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmChannelSelectorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Channel Selector"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Select a subset of signal channels"; }
    CString getDetailedDescription() const override { return "Selection can be based on channel name (case-sensitive) or index starting from 0"; }
    CString getCategory() const override { return "Signal processing/Channels"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_ChannelSelector; }
    IPluginObject* create() override { return new CBoxAlgorithmChannelSelector; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmChannelSelectorListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("Channel List", OV_TypeId_String, ":");
        prototype.addSetting("Action", TypeId_SelectionMethod, "Select");
        prototype.addSetting("Channel Matching Method", TypeId_MatchMethod, "Smart");

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);

        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ChannelSelectorDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
