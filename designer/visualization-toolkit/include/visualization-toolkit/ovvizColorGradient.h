#pragma once

#include "ovviz_base.h"

namespace OpenViBE {
namespace VisualizationToolkit {
namespace ColorGradient {
OVVIZ_API bool parse(CMatrix& colorGradient, const CString& string);
OVVIZ_API bool format(CString& string, const CMatrix& colorGradient);
OVVIZ_API bool interpolate(CMatrix& interpolatedColorGradient, const CMatrix& colorGradient, size_t steps);
}  // namespace ColorGradient
}  // namespace VisualizationToolkit
}  // namespace OpenViBE
