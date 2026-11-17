/**-------------------------------------------------------------------------------------------------
 * \file CSignalDisplay.h
 * \brief Classes for the Box Signal display.
 * \author Bruno Renier, Yann Renard, Alison Cellard, Jussi T. Lindgren (INRIA/IRISA).
 * \version 0.3.
 * \copyright Copyright (C) 2022 Inria
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *-------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "../SDisplaySettings.h"

namespace OpenViBE {

namespace VisualizationToolkit {
class IVisualizationContext;
}  // namespace VisualizationToolkit

namespace Plugins {
namespace SimpleVisualization {

class CSignalDisplayController;

/**
 * UG fork: the display settings, as the Signal display and the LSL display both have them: added to a box prototype, and
 * read from a box into SDisplaySettings, with an error logged for a setting missing or out of range. One list for both.
 * The settings are read by name, so their order in the prototype does not matter.
 */
constexpr const char* DISPLAY_SETTING_NAMES[] = {
    "Display Mode", "Auto vertical scale", "Vertical Scale", "Vertical Offset", "Time Scale", "Bottom ruler", "Left ruler", "Multiview"
};
void addDisplaySettings(Kernel::IBoxProto& prototype);

/**
 * \param setting The box's setting of a name, as FSettingValueAutoCast reads it; a box passes a lambda, as only a box can
 *                make one.
 */
template <class Setting>
bool readDisplaySettings(const Kernel::IBox& box, Kernel::ILogManager& log, Setting setting, SDisplaySettings& settings)
{
    for (const char* name : DISPLAY_SETTING_NAMES)
    {
        if (!box.hasSettingWithName(CString(name)))
        {
            log << Kernel::LogLevel_Error << "Missing setting '" << name << "'\n";
            return false;
        }
    }

    // The two enumeration settings go through operator uint64_t(), which resolves the entry name
    // through the type manager. The value it yields is the enumerator main.cpp registered, so it
    // converts straight across -- no string matching, and no second enum to translate into.
    settings.timeMode = ESignalDisplayMode(static_cast<uint64_t>(setting("Display Mode")));
    settings.scaling  = ESignalDisplayScaling(static_cast<uint64_t>(setting("Auto vertical scale")));

    settings.verticalScale      = static_cast<double>(setting("Vertical Scale"));
    settings.baselineOffset     = static_cast<double>(setting("Vertical Offset"));
    settings.timeScaleSeconds   = static_cast<double>(setting("Time Scale"));
    settings.showTimeRuler      = static_cast<bool>(setting("Bottom ruler"));
    settings.showAmplitudeRuler = static_cast<bool>(setting("Left ruler"));
    settings.layout             = static_cast<bool>(setting("Multiview")) ? ELayoutMode::Butterfly : ELayoutMode::Stacked;

    std::string error;
    if (!settings.validate(error))
    {
        log << Kernel::LogLevel_Error << error.c_str() << "\n";
        return false;
    }
    return true;
}

/**
* Decodes the incoming streams and forwards them to a QML view that stacks every channel as one
* trace of a single graph. The view is built on the designer's QML engine and handed to the window
* manager as a QQuickItem.
*/
class CSignalDisplay final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CSignalDisplay() = default;

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SignalDisplay)

private:
    /**
     * UG fork: the matrix the stream decoder put out. TSignalDecoder does not derive from TStreamedMatrixDecoder, so the
     * decoder is cast to the class it is; upstream cast both to TStreamedMatrixDecoder, which is undefined behaviour.
     */
    const CMatrix* decodedMatrix() const;

public:
    Toolkit::TDecoder<CSignalDisplay>* m_StreamDecoder = nullptr;
    Toolkit::TStimulationDecoder<CSignalDisplay> m_StimulationDecoder;
    Toolkit::TChannelUnitsDecoder<CSignalDisplay> m_UnitDecoder;

    CIdentifier m_InputTypeID = CIdentifier::undefined();

protected:
    /** Parsed once in initialize(), then owned and mutated by the controller. */
    SDisplaySettings m_settings;

    CSignalDisplayController* m_controller = nullptr;
    /** Held for the box's lifetime so that uninitialize() does not have to create a second one. */
    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
};


class CSignalDisplayListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 1)
        {
            CIdentifier inputType = CIdentifier::undefined();
            box.getInputType(index, inputType);
            if (inputType != OV_TypeId_Stimulations)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Error: Only stimulation type supported for input 2\n";
                box.setInputType(index, OV_TypeId_Stimulations);
            }
        }
        else if (index == 2)
        {
            CIdentifier inputType = CIdentifier::undefined();
            box.getInputType(index, inputType);
            if (inputType != OV_TypeId_ChannelUnits)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Error: Only ChannelUnits type supported for input 3\n";
                box.setInputType(index, OV_TypeId_ChannelUnits);
            }
        }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/**
 * \brief Signal Display plugin descriptor.
 */
class CSignalDisplayDesc final : public IBoxAlgorithmDesc
{
public:
    CString getName() const override { return "Signal display"; }
    CString getAuthorName() const override { return "Bruno Renier, Yann Renard, Alison Cellard, Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Displays the incoming stream"; }

    CString getDetailedDescription() const override { return "This box can be used to visualize signal and matrix streams"; }

    CString getCategory() const override { return "Visualization/Basic"; }
    CString getVersion() const override { return "0.3"; }
    void release() override { }

    CIdentifier getCreatedClass() const override { return Box_SignalDisplay; }
    CString getStockItemName() const override { return "gtk-zoom-fit"; }
    IPluginObject* create() override { return new CSignalDisplay(); }
    IBoxListener* createBoxListener() const override { return new CSignalDisplayListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        addDisplaySettings(prototype);  // UG fork: shared with the LSL display

        prototype.addInput("Data", OV_TypeId_Signal);
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Channel Units", OV_TypeId_ChannelUnits);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Stimulations);
        prototype.addInputSupport(OV_TypeId_ChannelUnits);

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SignalDisplayDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
