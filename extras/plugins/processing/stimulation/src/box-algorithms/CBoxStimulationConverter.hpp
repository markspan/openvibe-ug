///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxStimulationConverter.hpp
/// \brief Classes for the box Stimulation Converter.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 14/10/2022.
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
/// <summary> The class CBoxStimulationConverter describes the box Stimulation Converter. </summary>
class CBoxStimulationConverter final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StimulationConverter)

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxStimulationConverter> m_decoder;       // Stimulation Decoder
    Toolkit::TStimulationEncoder<CBoxStimulationConverter> m_encoder;       // Stimulation Encoder
    CStimulationSet *m_iStimulation = nullptr, *m_oStimulation = nullptr;   // Stimulation Receiver

    //***** Settings *****
    bool m_keepOthers      = true;
    uint64_t m_stimStart   = OVTK_StimulationId_Label_00;
    uint64_t m_stimEnd     = OVTK_StimulationId_Label_0F;
    uint64_t m_stimSent    = OVTK_StimulationId_Label_00;
    uint64_t m_lastEndTime = 0;
};


/// <summary> Descriptor of the box Stimulation Converter. </summary>
class CBoxStimulationConverterDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation Converter"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Convert a range of stimulation to one."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-convert"; }

    CIdentifier getCreatedClass() const override { return Box_StimulationConverter; }
    IPluginObject* create() override { return new CBoxStimulationConverter; }

    /*
    IBoxListener* createBoxListener() const override { return new CBoxStimulationConverterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }
    */
    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input",OV_TypeId_Stimulations);
        prototype.addOutput("Output",OV_TypeId_Stimulations);
        prototype.addSetting("Keep other stimulations", OV_TypeId_Boolean, "true");
        prototype.addSetting("Stimulation range begin", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Stimulation range end", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_0F");
        prototype.addSetting("Stimulation to send",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StimulationConverterDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
