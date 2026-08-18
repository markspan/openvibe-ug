///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxHelloBidirectionnalGame.hpp
/// \brief Class of the box that communicates with Hello Bidirectionnal Game.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 24/02/2021
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

/// <summary> The class CBoxHelloBidirectionnalGame describes the box that sends value in LSL. </summary>
class CBoxHelloBidirectionnalGame final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Hello_Bidirectionnal)

protected:
    // Encoder / Decoder
    Toolkit::TStimulationDecoder<CBoxHelloBidirectionnalGame> m_stimDecoder;
    Toolkit::TSignalDecoder<CBoxHelloBidirectionnalGame> m_signalDecoder;
    Toolkit::TStimulationEncoder<CBoxHelloBidirectionnalGame> m_stimEncoder;
    Toolkit::TSignalEncoder<CBoxHelloBidirectionnalGame> m_signalEncoder;

    // Input/ Output
    CMatrix *m_iMatrix          = nullptr, *m_oMatrix  = nullptr;
    CStimulationSet *m_iStimSet = nullptr, *m_oStimSet = nullptr;
    uint64_t m_lastMatrixTime   = 0, m_lastStimTime    = 0;

    // LSL Stream
    lsl::stream_outlet *m_signalOutlet = nullptr, *m_stimOutlet         = nullptr;
    lsl::stream_inlet *m_signalInlet   = nullptr, *m_stimInlet          = nullptr;
    const std::string m_outSignalName  = "ovOutSignal", m_outMarkerName = "ovOutMarkers";
    const std::string m_inSignalName   = "ovInSignal", m_inMarkerName   = "ovInMarkers";
    std::string m_outSignalID, m_outMarkerID;
    float m_bufferLSL = 0.0;
};


/// <summary> Descriptor of the box Hello Bidirectionnal Game. </summary>
class CBoxHelloBidirectionnalGameDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Hello Bidirectionnal Game"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override
    {
        return "Send input stream via LabStreamingLayer (LSL) to the unity game Hello Bidirectionnal and receive back matrix and stimulation.";
    }
    CString getDetailedDescription() const override
    {
        return "The Unity Game is in the OpenViBE Unity Game Set of Repository (https://gitlab.inria.fr/openvibe/unity-games/hello-bidirectionnal).\n";
    }
    CString getCategory() const override { return "Games"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_Hello_Bidirectionnal; }
    IPluginObject* create() override { return new CBoxHelloBidirectionnalGame; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addInput("Input stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Output matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output stimulations", OV_TypeId_Stimulations);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_Hello_Bidirectionnal_Desc)
};

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
