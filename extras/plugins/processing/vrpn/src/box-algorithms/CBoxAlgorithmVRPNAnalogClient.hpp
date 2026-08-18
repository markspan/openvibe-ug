///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmVRPNAnalogClient.hpp
/// \brief Classes for the Box VRPN Analog Client.
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
#include <deque>

#include <vrpn_Analog.h>

namespace OpenViBE {
namespace Plugins {
namespace VRPN {
class CBoxAlgorithmVRPNAnalogClient final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 128LL << 32; }    // the box clock frequency
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_VRPNAnalogClient)

    void SetAnalog(size_t nAnalog, const double* analog);

protected:
    uint64_t m_lastChunkEndTime  = 0;
    uint64_t m_chunkDuration     = 0;
    size_t m_sampling            = 0;
    size_t m_nChannel            = 0;
    size_t m_nSamplePerSentBlock = 0;

    bool m_firstStart = false;

    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    CString m_peripheralName;

    std::deque<std::vector<double>> m_sampleBuffer; // Used as a limited-size buffer of sample vectors
    std::vector<double> m_lastSamples;              // The last sample received from VRPN

    vrpn_Analog_Remote* m_vrpnAnalogRemote = nullptr;
};

class CBoxAlgorithmVRPNAnalogClientDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Analog VRPN Client"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Connects to an external VRPN device and translate an analog information into OpenViBE signal"; }
    CString getDetailedDescription() const override { return "-"; }
    CString getCategory() const override { return "Acquisition and network IO/VRPN"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_VRPNAnalogClient; }
    IPluginObject* create() override { return new CBoxAlgorithmVRPNAnalogClient; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // prototype.addInput  ("input name", /* input type (OV_TypeId_Signal) */);
        prototype.addOutput("Output", OV_TypeId_Signal);
        prototype.addSetting("Peripheral name", OV_TypeId_String, "openvibe-vrpn@localhost");
        prototype.addSetting("Sampling Rate", OV_TypeId_Integer, "512");
        prototype.addSetting("Number of Channels", OV_TypeId_Integer, "16");
        prototype.addSetting("Sample Count per Sent Block", OV_TypeId_Integer, "32");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_VRPNAnalogClientDesc)
};
}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
