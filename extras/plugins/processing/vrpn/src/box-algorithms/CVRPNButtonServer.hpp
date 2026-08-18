///-------------------------------------------------------------------------------------------------
/// 
/// \file CVRPNButtonServer.hpp
/// \brief Classes for the Box VRPN Button Server.
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
#include <cstdio>

namespace OpenViBE {
namespace Plugins {
namespace VRPN {
class CVRPNButtonServer final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CVRPNButtonServer() {}
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 64LL << 32; } // 64 times per second
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, Box_VRPNButtonServer)

    void SetStimulation(const size_t index, const uint64_t id, const uint64_t date);

protected:
    std::vector<Toolkit::TStimulationDecoder<CVRPNButtonServer>*> m_decoders;

    //Start and end time of the last buffer
    uint64_t m_startTime = 0;
    uint64_t m_endTime   = 0;

    size_t m_currInput = 0;

    CIdentifier m_serverID = CIdentifier::undefined();

    //Pairs of start/stop stimulations id
    std::map<size_t, std::pair<uint64_t, uint64_t>> m_stimulationPairs;
};

class CVRPNButtonServerListener final : public Toolkit::TBoxListener<IBoxListener>
{
private:
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputName(i, ("Input " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_Stimulations);
        }

        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setSettingName(i * 2 + 1, ("Button " + std::to_string(i + 1) + " ON").c_str());
            box.setSettingType(i * 2 + 1, OV_TypeId_Stimulation);

            box.setSettingName(i * 2 + 2, ("Button " + std::to_string(i + 1) + " OFF").c_str());
            box.setSettingType(i * 2 + 2, OV_TypeId_Stimulation);
        }

        return true;
    }

public:
    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(index * 2 + 1);
        box.removeSetting(index * 2 + 1);
        return this->check(box);
    }

    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override
    {
        box.addSetting("", OV_TypeId_Stimulation, "OVTK_GDF_Feedback_Continuous");
        box.addSetting("", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Trial");
        return this->check(box);
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CVRPNButtonServerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Button VRPN Server"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Creates VRPN button servers (one per input)."; }

    CString getDetailedDescription() const override
    {
        return "Creates VRPN button servers to make data from the plugin's inputs available to VRPN client applications.";
    }

    CString getCategory() const override { return "Acquisition and network IO/VRPN"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_VRPNButtonServer; }
    IPluginObject* create() override { return new CVRPNButtonServer(); }
    IBoxListener* createBoxListener() const override { return new CVRPNButtonServerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input 1", OVTK_TypeId_Stimulations);
        prototype.addSetting("Peripheral name", OV_TypeId_String, "openvibe-vrpn");
        prototype.addSetting("Button 1 ON", OV_TypeId_Stimulation, "OVTK_GDF_Feedback_Continuous");
        prototype.addSetting("Button 1 OFF", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Trial");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_VRPNButtonServerDesc)
};
}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
