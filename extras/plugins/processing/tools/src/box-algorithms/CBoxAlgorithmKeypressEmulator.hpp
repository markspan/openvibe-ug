///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmKeypressEmulator.hpp
/// \author Jussi T. Lindgren (Inria)
/// \version 0.1.
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

namespace OpenViBE {
namespace Plugins {
namespace Tools {
/**
 * \class CBoxAlgorithmKeypressEmulator
 * \author Jussi T. Lindgren / Inria
 * \date 29.Oct.2019
 * \brief Emulates keypresses on a keyboard based on input stimulations. Based on a request from Fabien Lotte / POTIOC.
 *
 */
class CBoxAlgorithmKeypressEmulator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_KeypressEmulator)

    // Register enums to the kernel used by this box
    static void RegisterEnums(const Kernel::IPluginModuleContext& ctx);

protected:
    Toolkit::TStimulationDecoder<CBoxAlgorithmKeypressEmulator> m_decoder;

    // @todo for multiple triggers, use std::map<> 
    uint64_t m_triggerStimulation = 0;
    uint64_t m_keyToPress         = 0;
    uint64_t m_modifier           = 0;
};

class CBoxAlgorithmKeypressEmulatorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Keypress Emulator"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Emulates pressing keyboard keys when receiving stimulations"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-index"; }

    CIdentifier getCreatedClass() const override { return Box_KeypressEmulator; }
    IPluginObject* create() override { return new CBoxAlgorithmKeypressEmulator; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations",OV_TypeId_Stimulations);

        // @todo add support for multiple keys, e.g. look at VRPN boxes for howto
        prototype.addSetting("Trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Key to press", TypeId_Keypress_Key, "A");
        prototype.addSetting("Key modifier", TypeId_Keypress_Modifier, TypeId_Keypress_Modifier_None.toString());

        prototype.addFlag(Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_KeypressEmulatorDesc)
};
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
