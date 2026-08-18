///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxHelloWorldGame.hpp
/// \brief Class of the box that communicate with Hello World Game.
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

/// <summary> The class CBoxHelloWorldGame describes the box that send value in LSL. </summary>
class CBoxHelloWorldGame final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Hello_World)

protected:
    // Decoders
    Toolkit::TStimulationDecoder<CBoxHelloWorldGame> m_stimDecoder;
    Toolkit::TSignalDecoder<CBoxHelloWorldGame> m_signalDecoder;
    CMatrix* m_iMatrix          = nullptr;
    CStimulationSet* m_iStimSet = nullptr;

    lsl::stream_outlet *m_signalOutlet = nullptr, *m_stimOutlet   = nullptr;
    const std::string m_signalName     = "ovSignal", m_markerName = "ovMarkers";
    std::string m_signalID, m_markerID;
};


/// <summary> Descriptor of the box Hello World Game. </summary>
class CBoxHelloWorldGameDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Hello World Game"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Send input stream via LabStreamingLayer (LSL) to the unity game Hello World."; }
    CString getDetailedDescription() const override
    {
        return "The Unity Game is in the OpenViBE Unity Game Set of Repository (https://gitlab.inria.fr/openvibe/unity-games/hello-world).\n";
    }
    CString getCategory() const override { return "Games"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_Hello_World; }
    IPluginObject* create() override { return new CBoxHelloWorldGame; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addInput("Input stimulations", OV_TypeId_Stimulations);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_Hello_World_Desc)
};

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
