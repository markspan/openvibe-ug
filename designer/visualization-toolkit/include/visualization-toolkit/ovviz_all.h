#pragma once

#include "ovp_global_defines.h"

//___________________________________________________________________//
//                                                                   //
// Basic includes                                                    //
//___________________________________________________________________//

#include "ovviz_defines.h"
#include "ovviz_base.h"

//___________________________________________________________________//
//                                                                   //
// Tools                                                             //
//___________________________________________________________________//

#include "ovvizColorGradient.h"
#include "ovvizIVisualizationContext.h"
#include "ovvizIVisualizationTree.h"
#include "ovvizIVisualizationWidget.h"

namespace OpenViBE {
namespace VisualizationToolkit {
OVVIZ_API bool initialize(const Kernel::IKernelContext& ctx);
OVVIZ_API bool uninitialize(const Kernel::IKernelContext& ctx);
}  // namespace VisualizationToolkit
}  // namespace OpenViBE
