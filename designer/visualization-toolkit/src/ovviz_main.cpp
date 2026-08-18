#include "visualization-toolkit/ovviz_all.h"
#include "ovvizCVisualizationContext.hpp"

namespace OpenViBE {

namespace {
VisualizationToolkit::CVisualizationContextDesc visualizationContextDesc;
}  // namespace

bool VisualizationToolkit::initialize(const Kernel::IKernelContext& ctx)
{
    Kernel::ITypeManager& typeManager     = ctx.getTypeManager();
    Kernel::IPluginManager& pluginManager = ctx.getPluginManager();

    pluginManager.registerPluginDesc(visualizationContextDesc);

    return true;
}

bool VisualizationToolkit::uninitialize(const Kernel::IKernelContext& /*ctx*/) { return true; }

}  // namespace OpenViBE
