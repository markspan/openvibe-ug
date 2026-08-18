///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmThresholdCrossingDetector.hpp
/// \brief Class of the box that detects threshold crossing.
/// \author Joan Fruitet, Jozef Legeny, Axel Bouneau (Inria).
/// \version 1.3.
/// \date 14/04/2022
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
namespace Stimulation {
class CBoxAlgorithmThresholdCrossingDetector final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ThresholdCrossingDetector)

protected:
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmThresholdCrossingDetector> m_decoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmThresholdCrossingDetector> m_encoder;

    uint64_t m_onStimId          = 0;
    uint64_t m_offStimId         = 0;
    uint64_t m_channelIdx        = 0;
    uint64_t m_samplesPerChannel = 0;
    double m_lastSample          = 0;
    bool m_firstSample           = false;

    double m_threshold = 0;
};

class CBoxAlgorithmThresholdCrossingDetectorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Threshold Crossing Detector"; }
    CString getAuthorName() const override { return "Joan Fruitet, Jozef Legeny, Axel Bouneau"; }
    CString getAuthorCompanyName() const override { return "Inria Sophia, Inria Bordeaux"; }
    CString getShortDescription() const override { return "Detects if the input signal crosses a chosen threshold"; }
    CString getDetailedDescription() const override { return "Triggers a stimulation when one sample in the input signal crosses a chosen threshold"; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.3"; }
    CString getStockItemName() const override { return "gtk-zoom-fit"; }

    CIdentifier getCreatedClass() const override { return Box_ThresholdCrossingDetector; }
    IPluginObject* create() override { return new CBoxAlgorithmThresholdCrossingDetector; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Signal", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Generated stimulations", OV_TypeId_Stimulations);
        prototype.addSetting("Threshold", OV_TypeId_Float, "0");
        prototype.addSetting("Cross over threshold", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Cross under threshold", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("Channel Index", OV_TypeId_Integer, "1");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ThresholdCrossingDetectorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
