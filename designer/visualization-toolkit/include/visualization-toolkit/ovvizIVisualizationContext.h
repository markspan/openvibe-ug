#pragma once

#include <openvibe/plugins/ovIPluginObject.h>
#include <toolkit/ovtk_all.h>

#include "ovvizIWindowManager.h"

#define OV_ClassId_VisualizationCtx     OpenViBE::CIdentifier(0xE06B92EF, 0xB6B68081)

namespace OpenViBE {
namespace VisualizationToolkit {
class IVisualizationContext : public Plugins::IPluginObject
{
public:
    /**
     * @brief Assing a visualization manager to the visualization context singleton
     * @param windowManager The window manager that handles visualization widgets for the application
     * @retval CIdentifier The mapping ID of the window manager
     */
    virtual CIdentifier addWindowManager(std::unique_ptr<IWindowManager> windowManager) = 0;

    /**
     * @brief Remove a window manager
     * @param windowManagerID The mapping ID of the window manager
     * @retval true In case of success.
     * @retval false In case of error.
     */
    virtual bool removeWindowManager(const CIdentifier& windowManagerID) = 0;

    /**
     * @brief Get the window manager
     * @param windowManagerID The mapping ID of the window manager
     * @return The window manager
     */
    virtual IWindowManager* getWindowManager(const CIdentifier& windowManagerID) = 0;

    _IsDerivedFromClass_(Plugins::IPluginObject, OV_ClassId_VisualizationCtx)
};
}  // namespace VisualizationToolkit
}  // namespace OpenViBE
