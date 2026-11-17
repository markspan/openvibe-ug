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
    m_encoder.initialize(*this, 0);
    m_sentVersion = ~0ULL;
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

bool CLSLDisplay::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CLSLDisplay::process()
{
    if (m_controller == nullptr) { return true; }
    uint64_t version                    = 0;
    const std::vector<std::string> uids = m_controller->streamsToRecord(version);
    if (version == m_sentVersion) { return true; }
    m_sentVersion = version;

    // A header that lists them, one row each, labelled with the uid; at least one row, empty when nothing is to be recorded
    CMatrix* matrix = m_encoder.getInputMatrix();
    matrix->resize(std::max<size_t>(1, uids.size()), 1);
    for (size_t i = 0; i < std::max<size_t>(1, uids.size()); ++i) { matrix->setDimensionLabel(0, i, i < uids.size() ? uids[i].c_str() : ""); }
    m_encoder.encodeHeader();
    const uint64_t now = getPlayerContext().getCurrentTime();
    getDynamicBoxContext().markOutputAsReadyToSend(0, now, now);
    return true;
}

bool CLSLDisplay::uninitialize()
{
    m_encoder.uninitialize();
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
