///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxLSLCommunication.hpp
/// \brief Class of the generic box that communicates with LSL.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 24/02/2021
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

#ifdef TARGET_HAS_ThirdPartyLSL

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <lsl_cpp.h>

#include <ctime>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

//--------------------------------------------------------------------------------
/// <summary> The class CBoxLSLCommunication describes the box that sends value in LSL. </summary>
class CBoxLSLCommunication final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 64LL << 32; }
    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LSLCommunication)

protected:
    // Encoder / Decoder
    std::vector<Toolkit::TGenericDecoder<CBoxLSLCommunication>> m_decoders;
    std::vector<Toolkit::TStreamedMatrixEncoder<CBoxLSLCommunication>> m_encoders;
    std::vector<Toolkit::TStimulationDecoder<CBoxLSLCommunication>> m_stimDecoders;
    std::vector<Toolkit::TStimulationEncoder<CBoxLSLCommunication>> m_stimEncoders;

    std::vector<CMatrix*> m_iMatrix, m_oMatrix;
    std::vector<CStimulationSet*> m_iStimSet, m_oStimSet;


    std::vector<lsl::stream_outlet*> m_outlets;
    std::vector<lsl::stream_inlet*> m_inlets;

    std::vector<std::string> m_names, m_ids;
    std::vector<uint64_t> m_lastOutputTimes;
    std::vector<std::vector<float>> m_buffers;

    size_t m_nInput = -1, m_nOutput = -1, m_firstStimInput = -1, m_firstStimOutput = -1;
};

//--------------------------------------------------------------------------------
/// <summary> Listener of the box LSL Communication. </summary>
class CBoxLSLCommunicationListener final : public Toolkit::TBoxListener<IBoxListener>
{
    //--------------------------------------------------------------------------------
    void check(Kernel::IBox& box, const bool input) const
    {
        checkName(box, input);
        checkType(box, input);
    }

    //--------------------------------------------------------------------------------
    void checkName(Kernel::IBox& box, const bool input) const
    {
        // Input/Output
        if (input) { for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputName(i, ("Input " + std::to_string(i + 1)).c_str()); } }
        else { for (size_t i = 0; i < box.getOutputCount(); ++i) { box.setOutputName(i, ("Output " + std::to_string(i + 1)).c_str()); } }
        // Settings
        for (size_t i = 0; i < box.getInputCount(); ++i) { box.setSettingName(i, getLabel(true, i).c_str()); }
        for (size_t i = 0; i < box.getOutputCount(); ++i) { box.setSettingName(i + box.getInputCount(), getLabel(false, i).c_str()); }
    }

    //--------------------------------------------------------------------------------
    void checkType(Kernel::IBox& box, const bool input) const
    {
        CIdentifier type;
        if (input) {
            bool firstStimFound = false;
            for (size_t i = 0; i < box.getInputCount(); ++i) {
                box.getInputType(i, type);
                if (!firstStimFound && type == OV_TypeId_Stimulations) { firstStimFound = true; }
                else if (firstStimFound && type != OV_TypeId_Stimulations) {
                    getLogManager() << Kernel::LogLevel_Error << "You must put All stimulations streams at the end of the Input list.\n";
                }
            }
        }
        else {
            bool firstStimFound = false;
            for (size_t i = 0; i < box.getOutputCount(); ++i) {
                box.getOutputType(i, type);
                if (!firstStimFound && type == OV_TypeId_Stimulations) { firstStimFound = true; }
                else if (firstStimFound && type != OV_TypeId_Stimulations) {
                    getLogManager() << Kernel::LogLevel_Error << "You must put All stimulations streams at the end of the Output list.\n";
                }
            }
        }
    }

    //--------------------------------------------------------------------------------
    std::string getLabel(const bool input, const size_t i) const { return (input ? "Input " : "Output ") + std::to_string(i + 1) + " Stream Name"; }
    std::string getStreamName(const bool input, const size_t i) const { return (input ? "i" : "o") + std::to_string(i + 1) + "ov"; }

public:
    //--------------------------------------------------------------------------------
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputName(index, ("Input " + std::to_string(index + 1)).c_str());
        box.setInputType(index, OV_TypeId_Signal);
        box.addSetting(("Input " + std::to_string(index + 1) + " Stream Name").c_str(), OV_TypeId_String, getStreamName(true, index).c_str(), index);
        return true;
    }

    //--------------------------------------------------------------------------------
    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(index);
        check(box, true);
        return true;
    }


    //--------------------------------------------------------------------------------
    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputName(index, ("Output " + std::to_string(index + 1)).c_str());
        box.setOutputType(index, OV_TypeId_StreamedMatrix);
        box.addSetting(("Output " + std::to_string(index + 1) + " Stream Name").c_str(), OV_TypeId_String, getStreamName(false, index).c_str());
        return true;
    }

    //--------------------------------------------------------------------------------
    bool onOutputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(index + box.getInputCount());
        check(box, false);
        return true;
    }

    //--------------------------------------------------------------------------------
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        check(box, true);
        return true;
    }

    //--------------------------------------------------------------------------------
    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        check(box, false);
        return true;
    }


    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

//--------------------------------------------------------------------------------
/// <summary> Descriptor of the box Hello Bidirectionnal Game. </summary>
class CBoxLSLCommunicationDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "LSL Communication"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "General box to send/receive LSL Stream."; }
    CString getDetailedDescription() const override { return "General box to send/receive LSL Stream."; }
    CString getCategory() const override { return "Acquisition and network IO"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_LSLCommunication; }
    IPluginObject* create() override { return new CBoxLSLCommunication; }

    IBoxListener* createBoxListener() const override { return new CBoxLSLCommunicationListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Stimulations);

        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LSLCommunication_Desc)
};

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
