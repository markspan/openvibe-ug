///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationListener.hpp
/// \brief Classes for the Box Stimulation listener.
/// \author Yann Renard (Inria).
/// \version 1.0.
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

namespace OpenViBE {
namespace Plugins {
namespace Tools {

class CBoxAlgorithmStimulationListener final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StimulationListener)

protected:
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_None;
    std::vector<Toolkit::TStimulationDecoder<CBoxAlgorithmStimulationListener>*> m_stimulationDecoders;
};

class CBoxAlgorithmStimulationListenerListener final : public Toolkit::TBoxListener<IBoxListener>
{
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputName(i, ("Stimulation stream " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_Stimulations);
        }

        return true;
    }

public:
    bool onInputRemoved(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }
    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmStimulationListenerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation listener"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Prints stimulation codes in the log manager"; }
    CString getDetailedDescription() const override { return "Prints each received stimulationto the log using the log level specified in the box config."; }
    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_StimulationListener; }
    IPluginObject* create() override { return new CBoxAlgorithmStimulationListener; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStimulationListenerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulation stream 1", OV_TypeId_Stimulations);
        prototype.addSetting("Log level to use", OV_TypeId_LogLevel, "Information");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StimulationListenerDesc)
};

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
