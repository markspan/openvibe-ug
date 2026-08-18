///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxLSLExport.hpp
/// \brief Class of the box that export stream with LSL.
/// \author Jussi T. Lindgren (Inria).
/// \version 1.0.
/// \date 30/01/2015
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
namespace NetworkIO {

//--------------------------------------------------------------------------------
/// <summary>  The class CBoxLSLExport describes the box LSL Export. </summary>
class CBoxLSLExport final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LSLExport)

protected:
    // Decoders
    Toolkit::TStimulationDecoder<CBoxLSLExport> m_stimDecoder;
    Toolkit::TSignalDecoder<CBoxLSLExport> m_signalDecoder;

    lsl::stream_outlet* m_signalOutlet   = nullptr;
    lsl::stream_outlet* m_stimulusOutlet = nullptr;

    std::vector<float> m_buffer;

    std::string m_signalName, m_signalID;
    std::string m_markerName, m_markerID;

    bool m_useOVTimestamps = false;
    CTime m_startTime      = CTime(0);
};

//--------------------------------------------------------------------------------
/// <summary>  Descriptor of the box LSL Export. </summary>
class CBoxLSLExportDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "LSL Export"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Send input stream out via LabStreamingLayer (LSL)"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Acquisition and network IO"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return Box_LSLExport; }
    IPluginObject* create() override { return new CBoxLSLExport; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addInput("Input stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Signal stream", OV_TypeId_String, "openvibeSignal");
        prototype.addSetting("Marker stream", OV_TypeId_String, "openvibeMarkers");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LSLExportDesc)
};
}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE


#endif // TARGET_HAS_Boost
