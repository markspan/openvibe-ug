#include "defines.hpp"

#include "algorithms/CAlgorithmSphericalSplineInterpolation.hpp"

#include "box-algorithms/CBoxAlgorithmTopographicMap2DDisplay.hpp"
#include "box-algorithms/CBoxAlgorithmMatrixDisplay.hpp"

namespace OpenViBE {
namespace Plugins {
OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationType(TypeId_SphericalLinearInterpolation, "Spherical linear interpolation type");
    context.getTypeManager().registerEnumerationEntry(TypeId_SphericalLinearInterpolation, "Spline (potentials)", size_t(EInterpolationType::Spline));
    context.getTypeManager().registerEnumerationEntry(TypeId_SphericalLinearInterpolation, "Spline laplacian (currents)",
                                                      size_t(EInterpolationType::Laplacian));

    OVP_Declare_New(Test::CAlgorithmSphericalSplineInterpolationDesc)

    OVP_Declare_New(SimpleVisualization::CBoxAlgorithmTopographicMap2DDisplayDesc)
    OVP_Declare_New(SimpleVisualization::CBoxAlgorithmMatrixDisplayDesc)
OVP_Declare_End()
}  // namespace Plugins
}  // namespace OpenViBE
