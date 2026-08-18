///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxStimulationMultiplexer.hpp
/// \brief Classes for the Box Stimulation Multiplexer.
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

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <vector>
#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

/// <summary> The class CBoxStimulationMultiplexer describes the box that merges several stimulation streams into one. </summary>
class CBoxStimulationMultiplexer final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StimulationMultiplexer)

private:
    std::vector<Toolkit::TStimulationDecoder<CBoxStimulationMultiplexer>> m_decoders;
    Toolkit::TStimulationEncoder<CBoxStimulationMultiplexer> m_encoder;

    std::vector<uint64_t> m_decoderEndTimes;

    uint64_t m_lastStartTime = 0;
    uint64_t m_lastEndTime   = 0;
    bool m_wasHeaderSent     = false;

    std::multimap<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> m_stimulations;
};

/// <summary> Listener of the box Stimulation multiplexer. </summary>
class CBoxStimulationMultiplexerListener final : public Toolkit::TBoxListener<IBoxListener>
{
    bool check(Kernel::IBox& box) const
    {
        for (size_t input = 0; input < box.getInputCount(); ++input) {
            box.setInputName(input, ("Input stimulations " + std::to_string(input + 1)).c_str());
            box.setInputType(input, OV_TypeId_Stimulations);
        }
        return true;
    }

public:
    bool onInputRemoved(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }
    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Stimulation multiplexer. </summary>
class CBoxStimulationMultiplexerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation multiplexer"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Merges several stimulation streams into one."; }

    CString getDetailedDescription() const override
    {
        return
                "The stimulations are ordered according to their start date. Thus each time all the input have chunks covering a period of time, a new output chunk is sent. This box may eventually produce output chunk reflecting a different duration depending on the inputs.";
    }

    CString getCategory() const override { return "Streaming"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return Box_StimulationMultiplexer; }
    IPluginObject* create() override { return new CBoxStimulationMultiplexer; }
    IBoxListener* createBoxListener() const override { return new CBoxStimulationMultiplexerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stimulations 1", OV_TypeId_Stimulations);
        prototype.addInput("Input stimulations 2", OV_TypeId_Stimulations);
        prototype.addOutput("Multiplexed stimulations", OV_TypeId_Stimulations);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addInputSupport(OV_TypeId_Stimulations);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StimulationMultiplexerDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
