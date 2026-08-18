///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxHelloSenderGame.hpp
/// \brief Class of the box that communicate with Hello Sender Game.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/03/2020.
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
namespace Games {

/// <summary> The class CBoxHelloSenderGame describes the box that send value in LSL. </summary>
class CBoxHelloSenderGame final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 64LL << 32; }
    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Hello_Sender)

protected:
    // Decoders
    Toolkit::TStimulationEncoder<CBoxHelloSenderGame> m_stimEncoder;
    Toolkit::TSignalEncoder<CBoxHelloSenderGame> m_signalEncoder;
    CMatrix* m_oMatrix          = nullptr;
    CStimulationSet* m_oStimSet = nullptr;
    float* m_buffer             = nullptr;
    bool m_headerSent           = false;
    uint64_t m_lastMatrixTime   = 0;
    uint64_t m_lastStimTime     = 0;

    lsl::stream_inlet *m_signalInlet = nullptr, *m_stimInlet    = nullptr;
    const std::string m_signalName   = "ovSignal", m_markerName = "ovMarker";
};


/// <summary> Descriptor of the box Hello Sender Game. </summary>
class CBoxHelloSenderGameDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Hello Sender Game"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Receive stream via LabStreamingLayer (LSL) from the unity game Hello Sender."; }
    CString getDetailedDescription() const override
    {
        return "The Unity Game is in the OpenViBE Unity Game Set of Repository (https://gitlab.inria.fr/openvibe/unity-games/hello-sender).\n";
    }
    CString getCategory() const override { return "Games"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_Hello_Sender; }
    IPluginObject* create() override { return new CBoxHelloSenderGame; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Output matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output stimulations", OV_TypeId_Stimulations);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_Hello_Sender_Desc)
};

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyLSL
