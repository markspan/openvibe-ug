///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationValidator.hpp
/// \brief Classes of the Box Stimulation Validator.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/02/2020.
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
/// <summary> The class CBoxAlgorithmStimulationValidator describes the box Stimulation Validator. </summary>
class CBoxAlgorithmStimulationValidator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    //Here is the different process callbacks possible
    // - On clock ticks :
    //bool processClock(Kernel::CMessageClock& msg) override;       
    // - On new input received (the most common behaviour for signal processing) :
    bool processInput(const size_t index) override;

    // If you want to use processClock, you must provide the clock frequency.
    //uint64_t getClockFrequency() override;

    bool process() override;

    // As we do with any class in openvibe, we use the macro below to associate this box to an unique identifier. 
    // The inheritance information is also made available, as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StimulationValidator)

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxAlgorithmStimulationValidator> m_decoder;  // Stimulation Decoder
    Toolkit::TStimulationEncoder<CBoxAlgorithmStimulationValidator> m_encoder;  // Stimulation Encoder
    CStimulationSet *m_iStimulation = nullptr, *m_oStimulation = nullptr;       // Stimulation Receiver

    //***** Settings *****
    CIdentifier m_stim     = OVTK_StimulationId_Label_00;
    size_t m_count         = 0, m_limit = 0;
    uint64_t m_lastEndTime = 0;
};


/// <summary> Descriptor of the box Stimulation Validator. </summary>
class CBoxAlgorithmStimulationValidatorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation Validator"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Count a stimulation and send this after N occurence."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-add"; }

    CIdentifier getCreatedClass() const override { return Box_StimulationValidator; }
    IPluginObject* create() override { return new CBoxAlgorithmStimulationValidator; }

    /*
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStimulationValidatorListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }
    */
    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input",OV_TypeId_Stimulations);
        prototype.addOutput("Output",OV_TypeId_Stimulations);
        prototype.addSetting("Stimulation to count",OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Number for validation",OV_TypeId_Integer, "2");
        prototype.addFlag(Box_FlagIsUnstable);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StimulationValidatorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
