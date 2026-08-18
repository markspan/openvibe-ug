///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmVRPNButtonClient.hpp
/// \brief Classes for the Box VRPN Button Client.
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

#include <list>
#include <vrpn_Button.h>

namespace OpenViBE {
namespace Plugins {
namespace VRPN {
class CBoxAlgorithmVRPNButtonClient final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 128LL << 32; } // the box clock frequency
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_VRPNButtonClient)

    std::list<std::pair<size_t, bool>> m_Buttons;
    void SetButton(const size_t index, const bool pressed);

protected:
    uint64_t m_lastChunkEndTime = 0;
    bool m_gotStimulation       = false;
    std::vector<Kernel::IAlgorithmProxy*> m_encoders;
    std::vector<CStimulationSet*> m_stimSets;
    std::vector<uint64_t> m_stimIDsOn;
    std::vector<uint64_t> m_stimIDsOff;
    vrpn_Button_Remote* m_vrpnButtonRemote = nullptr;
};

class CBoxAlgorithmVRPNButtonClientListener final : public Toolkit::TBoxListener<IBoxListener>
{
private:
    static bool check(Kernel::IBox& box)
    {
        for (size_t i = 0; i < box.getOutputCount(); ++i) {
            const std::string idx = std::to_string(i + 1);
            box.setOutputName(i, ("Output " + idx).c_str());
            box.setSettingName(i * 2 + 1, ("Button " + idx + " ON").c_str());
            box.setSettingName(i * 2 + 2, ("Button " + idx + " OFF").c_str());
        }
        return true;
    }

public:
    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputType(index, OV_TypeId_Stimulations);
        box.addSetting("", OV_TypeId_Stimulation, "OVTK_GDF_Feedback_Continuous");
        box.addSetting("", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Trial");
        return check(box);
    }

    bool onOutputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(index * 2 + 2);
        box.removeSetting(index * 2 + 1);
        return check(box);
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmVRPNButtonClientDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Button VRPN Client"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Connects to an external VRPN device and translate a button information into OpenViBE stimulations"; }
    CString getDetailedDescription() const override { return "-"; }
    CString getCategory() const override { return "Acquisition and network IO/VRPN"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_VRPNButtonClient; }
    IPluginObject* create() override { return new CBoxAlgorithmVRPNButtonClient; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmVRPNButtonClientListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // prototype.addInput  ("input name", /* input type (OV_TypeId_Signal) */);
        prototype.addOutput("Output", OV_TypeId_Stimulations);
        prototype.addSetting("Peripheral name", OV_TypeId_String, "openvibe-vrpn@localhost");
        prototype.addSetting("Button 1 ON", OV_TypeId_Stimulation, "OVTK_GDF_Feedback_Continuous");
        prototype.addSetting("Button 1 OFF", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Trial");
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_VRPNButtonClientDesc)
};
}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
