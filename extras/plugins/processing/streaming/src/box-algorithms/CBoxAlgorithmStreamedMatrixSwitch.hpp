///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStreamedMatrixSwitch.hpp
/// \author Laurent Bonnet (Inria)
/// \version 1.1.
/// \date 12/05/2011.
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

#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Streaming {
/// <summary> The class CBoxAlgorithmStreamedMatrixSwitch describes the box Streamed Matrix Switch. </summary>
class CBoxAlgorithmStreamedMatrixSwitch final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StreamedMatrixSwitch)

protected:
    Toolkit::TStimulationDecoder<CBoxAlgorithmStreamedMatrixSwitch> m_stimDecoder;
    Toolkit::TDecoder<CBoxAlgorithmStreamedMatrixSwitch>* m_streamDecoder = nullptr;

    std::map<uint64_t, size_t> m_stimOutputIndexes;
    size_t m_activeOutputIdx             = 0;
    uint64_t m_lastStimInputChunkEndTime = 0;
};


class CBoxAlgorithmStreamedMatrixSwitchListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) {
            box.setInputType(0,OV_TypeId_Stimulations);
            return true;
        }

        CIdentifier id = CIdentifier::undefined();
        box.getInputType(1, id);

        // all output must have the input type
        for (size_t i = 0; i < box.getOutputCount(); ++i) { box.setOutputType(i, id); }
        return true;
    }

    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        // the output must have the same type as the input
        CIdentifier id = CIdentifier::undefined();
        box.getInputType(1, id);
        box.setOutputType(index, id);

        const std::string name = ("Switch stim for output " + std::to_string(index + 1));
        box.addSetting(name.c_str(), OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");

        return true;
    }

    bool onOutputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(1 + index);       // +1 for the first setting which doesn't correspond to a stream

        // Rename the rest to match the changed indexing
        for (size_t i = (1 + index); i < box.getSettingCount(); ++i) {
            const std::string name = ("Switch stim for output " + std::to_string(i));
            box.setSettingName(i, name.c_str());
        }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Streamed Matrix Switch. </summary>
class CBoxAlgorithmStreamedMatrixSwitchDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stream Switch"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Redirect its input on a particular output"; }

    CString getDetailedDescription() const override
    {
        return "This box act as a switch between N possible outputs for its Streamed Matrix input. N Stimulation settings trigger the switch.";
    }

    CString getCategory() const override { return "Streaming"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return Box_StreamedMatrixSwitch; }
    IPluginObject* create() override { return new CBoxAlgorithmStreamedMatrixSwitch; }


    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStreamedMatrixSwitchListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Triggers",OV_TypeId_Stimulations);
        prototype.addInput("Matrix",OV_TypeId_StreamedMatrix);

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        //prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        prototype.addOutput("Output",OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output",OV_TypeId_StreamedMatrix);

        //prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);

        prototype.addSetting("Default to output 1", OV_TypeId_Boolean, "false");
        prototype.addSetting("Switch stim for output 1",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Switch stim for output 2",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");

        //prototype.addFlag(Kernel::BoxFlag_CanModifySetting);
        //prototype.addFlag(Kernel::BoxFlag_CanAddSetting);

        //prototype.addFlag(Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StreamedMatrixSwitchDesc)
};
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
