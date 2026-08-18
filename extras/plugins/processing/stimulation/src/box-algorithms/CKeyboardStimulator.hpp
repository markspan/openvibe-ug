///-------------------------------------------------------------------------------------------------
/// 
/// \file CKeyboardStimulator.hpp
/// \author Bruno Renier (INRIA/IRISA).
/// \version 1.1.
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

#if defined(TARGET_HAS_ThirdPartyGTK)

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <vector>
#include <map>

#include <visualization-toolkit/ovviz_all.h>

namespace TCPTagging {
class IStimulusSender; // fwd declare
}

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
class CKeyboardStimulator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CKeyboardStimulator() { }

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    uint64_t getClockFrequency() override { return (32LL << 32); }

    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override { return true; }

    bool ParseConfigurationFile(const char* filename);
    void ProcessKey(const guint key, const bool state);

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_KeyboardStimulator)

protected:
    Toolkit::TStimulationEncoder<CKeyboardStimulator> m_encoder;

    GtkWidget* m_widget = nullptr;

    typedef struct SKey
    {
        std::string meaning;
        uint64_t press;
        uint64_t release;
        bool status;
    } key_t;

    //! Stores keyvalue/stimulation couples
    std::map<guint, key_t> m_keyToStimulation;
    bool m_displayMeaning = false;
    bool m_displayStimulationCodes = true;
    bool m_sendKeyReleaseStimCode = true;
    //! Vector of the stimulations to send when possible
    std::vector<uint64_t> m_stimulationToSend;

    //! Plugin's previous activation date
    uint64_t m_previousActivationTime = 0;

    bool m_error = false;

    // TCP Tagging
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;

    bool m_unknownKeyPressed = false;
    size_t m_unknownKeyCode  = 0;

    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
};

/**
* Plugin's description
*/
class CKeyboardStimulatorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Keyboard stimulator"; }
    CString getAuthorName() const override { return "Bruno Renier"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Stimulation generator"; }
    CString getDetailedDescription() const override { return "Sends stimulations according to key presses"; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "0.2"; }

    CIdentifier getCreatedClass() const override { return Box_KeyboardStimulator; }
    IPluginObject* create() override { return new CKeyboardStimulator(); }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Outgoing Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Filename", OV_TypeId_Filename, "${Path_Data}/plugins/stimulation/simple-keyboard-to-stimulations.txt");
        prototype.addSetting("Display key meaning", OV_TypeId_Boolean, "true");
        prototype.addSetting("Display key stimulation codes", OV_TypeId_Boolean, "true");
        prototype.addSetting("Send key-release stim code", OV_TypeId_Boolean, "true");
        // @note we don't want to expose these to the user as there is no latency correction in tcp tagging; its best to use localhost
        // prototype.addSetting("TCP Tagging Host address", OV_TypeId_String, "");
        // prototype.addSetting("TCP Tagging Host port",    OV_TypeId_Integer, "15361");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_KeyboardStimulatorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE

#endif
