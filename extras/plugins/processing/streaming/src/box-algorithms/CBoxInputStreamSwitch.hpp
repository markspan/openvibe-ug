///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxInputStreamSwitch.hpp
/// \brief Classes for the box Input Stream Switch.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 14/10/2022.
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
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Streaming {
/// <summary> The class CBoxInputStreamSwitch describes the box Streamed Matrix Switch. </summary>
class CBoxInputStreamSwitch final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_InputMatrixSwitch)

protected:
    Toolkit::TStimulationDecoder<CBoxInputStreamSwitch> m_stimDecoder;
    std::vector<Toolkit::TDecoder<CBoxInputStreamSwitch>*> m_streamDecoder;

    std::map<uint64_t, size_t> m_stimInputIndexes;
    bool m_headerSent       = false;
    bool m_endSent        = false;
    size_t m_activeInputIdx = 0;
};


class CBoxInputStreamSwitchListener final : public Toolkit::TBoxListener<IBoxListener>
{
private:
    static void changeType(Kernel::IBox& box, const CIdentifier& type)
    {
        for (size_t i = 1; i < box.getInputCount(); ++i) { box.setInputType(i, type); }
        box.setOutputType(0, type);
    }

public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) {
            box.setInputType(0, OV_TypeId_Stimulations);
            return true;
        }

        CIdentifier type = CIdentifier::undefined();
        box.getInputType(index, type);
        changeType(box, type);
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier type = CIdentifier::undefined();
        box.getOutputType(0, type);
        changeType(box, type);
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        // the output must have the same type as the input
        CIdentifier id = CIdentifier::undefined();
        box.getInputType(1, id);
        box.setOutputType(index, id);

        const std::string name = ("Switch stim for input " + std::to_string(index + 1));
        box.addSetting(name.c_str(), OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");

        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(1 + index);       // +1 for the first setting which doesn't correspond to a stream

        // Rename the rest to match the changed indexing
        for (size_t i = (1 + index); i < box.getSettingCount(); ++i) {
            const std::string name = ("Switch stim for input " + std::to_string(i));
            box.setSettingName(i, name.c_str());
        }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Streamed Matrix Switch. </summary>
class CBoxInputStreamSwitchDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Input Stream Switch"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Redirect one of the inputs to the output"; }

    CString getDetailedDescription() const override
    {
        return "This box acts as a switch to output a stream from N possible inputs. Each input has a corresponding stimulation to perform the selection.";
    }

    CString getCategory() const override { return "Streaming"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return Box_InputMatrixSwitch; }
    IPluginObject* create() override { return new CBoxInputStreamSwitch; }


    IBoxListener* createBoxListener() const override { return new CBoxInputStreamSwitchListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Triggers",OV_TypeId_Stimulations);
        prototype.addInput("Input",OV_TypeId_StreamedMatrix);
        prototype.addInput("Input", OV_TypeId_StreamedMatrix);

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addOutput("Output", OV_TypeId_StreamedMatrix);

        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addSetting("Default to Input 1", OV_TypeId_Boolean, "true");
        prototype.addSetting("Switch stim for input 1",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Switch stim for input 2",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_FeatureVector);
        prototype.addInputSupport(OV_TypeId_Stimulations);

        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_FeatureVector);
        prototype.addOutputSupport(OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_InputMatrixSwitchDesc)
};
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
