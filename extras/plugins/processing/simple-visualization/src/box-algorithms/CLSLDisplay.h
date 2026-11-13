// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.28. The LSL display box.

#pragma once

#include "../defines.hpp"
#include "../SDisplaySettings.h"
#include "CSignalDisplay.h"  // addDisplaySettings(), readDisplaySettings()

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {

namespace VisualizationToolkit {
class IVisualizationContext;
}  // namespace VisualizationToolkit

namespace Plugins {
namespace SimpleVisualization {

class CLSLDisplayController;

/**
 * \brief Shows LSL streams from the network, without the acquisition server: data streams of any rate, stacked on one time
 *        axis, and marker streams, numbers or text, as flags on all of them.
 *
 * The box has no inputs: it reads the streams itself (CLSLReader), and the streams are chosen while it runs, in its panel. Its
 * settings are the Signal display's, and are where the panel starts. It is for seeing the streams; record them with LabRecorder.
 */
class CLSLDisplay final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LSLDisplay)

private:
    SDisplaySettings m_settings;
    CLSLDisplayController* m_controller = nullptr;
    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
};

class CLSLDisplayDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "LSL display (UG)"; }
    CString getAuthorName() const override { return "MMS"; }
    CString getAuthorCompanyName() const override { return "University of Groningen"; }
    CString getShortDescription() const override { return "Shows LSL streams from the network: data and markers"; }
    CString getDetailedDescription() const override
    {
        return "Shows the LabStreamingLayer streams on the network that are ticked in its panel while it runs. Data streams, of any rate "
               "and any number of channels, are stacked on one time axis, lined up by their LSL times; markers, numbers or text, are "
               "drawn as flags on all of them. The options are the Signal display's. Record the streams with LabRecorder.";
    }
    CString getCategory() const override { return "Visualization/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-zoom-fit"; }

    CIdentifier getCreatedClass() const override { return Box_LSLDisplay; }
    IPluginObject* create() override { return new CLSLDisplay(); }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        addDisplaySettings(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LSLDisplayDesc)
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
