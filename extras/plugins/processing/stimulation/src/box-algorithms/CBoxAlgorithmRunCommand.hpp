///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRunCommand.hpp
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
#include <cstdio>
#include <vector>
#include <map>
#include <iomanip>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
class CBoxAlgorithmRunCommand final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_RunCommand)

protected:
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CStimulationSet*> op_stimSet;
    std::map<uint64_t, std::vector<CString>> m_commands;
};

class CBoxAlgorithmRunCommandListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool Check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getSettingCount(); i += 2) {
            box.setSettingName(i, ("Stimulation " + std::to_string(i / 2 + 1)).c_str());
            box.setSettingType(i, OV_TypeId_Stimulation);
            box.setSettingName(i + 1, ("Command " + std::to_string(i / 2 + 1)).c_str());
            box.setSettingType(i + 1, OV_TypeId_String);
        }
        return true;
    }

    bool onSettingAdded(Kernel::IBox& box, const size_t index) override
    {
        std::stringstream ss;
        ss.fill('0');
        ss << std::setw(2) << size_t(index / 2 + 1);
        const CString name(ss.str().c_str());
        box.setSettingDefaultValue(index, name);
        box.setSettingValue(index, name);
        box.addSetting("", OV_TypeId_String, "");
        this->Check(box);
        return true;
    }

    bool onSettingRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting((index / 2) * 2);
        this->Check(box);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmRunCommandDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Run Command"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Runs some command using system call depending on provided stimulations"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_RunCommand; }
    IPluginObject* create() override { return new CBoxAlgorithmRunCommand; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmRunCommandListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addSetting("Stimulation 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("Command 1", OV_TypeId_String, "");
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_RunCommandDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
