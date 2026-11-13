// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see CLSLDisplay.h.

#include "CLSLDisplay.h"

#include "../CLSLDisplayController.h"

#include <visualization-toolkit/ovviz_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CLSLDisplay::initialize()
{
    const auto setting = [this](const char* name) { return FSettingValueAutoCast(*this->getBoxAlgorithmContext(), CString(name)); };
    if (!readDisplaySettings(this->getStaticBoxContext(), this->getLogManager(), setting, m_settings)) { return false; }

    // As the Signal display: the view is built on the window manager's QML engine and handed to it
    const CIdentifier windowManagerID = CIdentifier(
        getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (m_visualizationCtx == nullptr)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
        return false;
    }
    VisualizationToolkit::IWindowManager* windowManager = m_visualizationCtx->getWindowManager(windowManagerID);
    if (windowManager == nullptr || windowManager->getQmlEngine() == nullptr)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not get the window manager or its QML engine\n";
        return false;
    }

    m_controller = new CLSLDisplayController(this->getLogManager(), m_settings, windowManager->getQmlEngine());
    if (m_controller->rootItem() == nullptr)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to build the LSL display's view\n";
        return false;
    }
    CIdentifier boxID(getStaticBoxContext().getIdentifier());  // the window manager takes it by reference
    windowManager->setVisualizationItem(boxID, m_controller->rootItem());
    return true;
}

bool CLSLDisplay::uninitialize()
{
    if (m_visualizationCtx != nullptr)
    {
        const CIdentifier windowManagerID = CIdentifier(
            getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
        if (VisualizationToolkit::IWindowManager* windowManager = m_visualizationCtx->getWindowManager(windowManagerID))
        {
            CIdentifier boxID(getStaticBoxContext().getIdentifier());
            windowManager->clearVisualizationItem(boxID);  // before the item is destroyed
        }
        this->releasePluginObject(m_visualizationCtx);
        m_visualizationCtx = nullptr;
    }
    // As the Signal display: uninitialize() may run on the player thread, so the controller, with its timer and its reader
    // thread, goes on its own (GUI) thread
    if (m_controller != nullptr)
    {
        m_controller->deleteLater();
        m_controller = nullptr;
    }
    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
