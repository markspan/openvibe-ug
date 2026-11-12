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
        prototype.addSetting("Display Mode", TypeId_SignalDisplayMode, "Scan"); //list scroll or scan
        prototype.addSetting("Auto vertical scale", TypeId_SignalDisplayScaling, "Per channel"); //"Per channel", "By type", "Global", "None"
        // UG fork: no "Scale refresh interval (secs)": the scale is worked out again for every page
        prototype.addSetting("Vertical Scale",OV_TypeId_Float, "100");
        prototype.addSetting("Vertical Offset",OV_TypeId_Float, "0");
        prototype.addSetting("Time Scale", OV_TypeId_Float, "10");
        prototype.addSetting("Bottom ruler", OV_TypeId_Boolean, "true");
        prototype.addSetting("Left ruler", OV_TypeId_Boolean, "false");
        prototype.addSetting("Multiview", OV_TypeId_Boolean, "false");

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
