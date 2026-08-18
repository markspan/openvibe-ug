///-------------------------------------------------------------------------------------------------
/// 
/// \file CVRPNAnalogServer.hpp
/// \brief Classes for the Box VRPN Analog Server.
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
namespace VRPN {
class CVRPNAnalogServer final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CVRPNAnalogServer() {}
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 64LL << 32; } // 64hz
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, Box_VRPNAnalogServer)

protected:
    CIdentifier m_serverID = CIdentifier::undefined();
    bool m_analogSet       = false;

    std::map<size_t, Kernel::IAlgorithmProxy*> m_decoders;
    std::map<size_t, size_t> m_nAnalogs;
};

class CVRPNAnalogServerListener final : public Toolkit::TBoxListener<IBoxListener>
{
private:
    static bool check(Kernel::IBox& box)
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputName(i, ("Input " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_StreamedMatrix);
        }

        return true;
    }

public:
    bool onInputRemoved(Kernel::IBox& box, const size_t /*index*/) override { return check(box); }
    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override { return check(box); }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CVRPNAnalogServerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Analog VRPN Server"; }
    CString getAuthorName() const override { return "Bruno Renier/Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Creates VRPN analog servers (one per input)."; }

    CString getDetailedDescription() const override
    {
        return "Creates VRPN analog servers to make data from the plugin's inputs available to VRPN client applications.";
    }

    CString getCategory() const override { return "Acquisition and network IO/VRPN"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_VRPNAnalogServer; }
    IPluginObject* create() override { return new CVRPNAnalogServer(); }
    IBoxListener* createBoxListener() const override { return new CVRPNAnalogServerListener; }

    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input 1", OV_TypeId_StreamedMatrix);
        prototype.addSetting("Peripheral name", OV_TypeId_String, "openvibe-vrpn");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_VRPNAnalogServerDesc)
};
}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
