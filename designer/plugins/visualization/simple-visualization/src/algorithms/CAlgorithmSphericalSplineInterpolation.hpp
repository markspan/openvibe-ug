#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <array>

namespace OpenViBE {
namespace Plugins {
namespace Test {
class CAlgorithmSphericalSplineInterpolation final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Algorithm_SphericalSplineInterpolation)

protected:
    //input parameters
    //----------------
    Kernel::TParameterHandler<int64_t> ip_splineOrder;
    Kernel::TParameterHandler<int64_t> ip_nControlPoints;
    Kernel::TParameterHandler<CMatrix*> ip_controlPointsCoords;
    Kernel::TParameterHandler<CMatrix*> ip_controlPointsValues;
    Kernel::TParameterHandler<CMatrix*> ip_samplePointsCoords;

    //output parameters
    //-----------------
    Kernel::TParameterHandler<CMatrix*> op_samplePointsValues;
    Kernel::TParameterHandler<double> op_minSamplePointValue;
    Kernel::TParameterHandler<double> op_maxSamplePointValue;

    //internal data
    //-------------
    bool m_firstProcess = true;
    std::vector<double> m_coords;
    std::vector<double*> m_coordsPtr;
    std::vector<double> m_splineCoefs;
    std::vector<double> m_laplacianCoefs;
    std::array<double, 2004> m_scd;
    std::array<double, 2004> m_pot;
};

class CAlgorithmSphericalSplineInterpolationDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spherical spline interpolation"; }
    CString getAuthorName() const override { return "Vincent Delannoy"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Interpolates potentials/laplacians using a spherical spline"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Algorithm/Signal processing"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Algorithm_SphericalSplineInterpolation; }
    IPluginObject* create() override { return new CAlgorithmSphericalSplineInterpolation(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        //input parameters
        prototype.addInputParameter(SplineInterpolation_InputParameterId_SplineOrder, "Spline order", Kernel::ParameterType_Integer);
        prototype.addInputParameter(
            SplineInterpolation_InputParameterId_ControlPointsCount, "Number of values", Kernel::ParameterType_Integer);
        prototype.addInputParameter(
            SplineInterpolation_InputParameterId_ControlPointsCoord, "Values coordinates", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(SplineInterpolation_InputParameterId_ControlPointsValues, "Values", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(
            SplineInterpolation_InputParameterId_SamplePointsCoord, "Coordinates where to interpolate values",
            Kernel::ParameterType_Matrix);
        //input triggers
        prototype.addInputTrigger(SplineInterpolation_InputTriggerId_PrecomputeTables, CString("Precomputation"));
        prototype.addInputTrigger(SplineInterpolation_InputTriggerId_ComputeSplineCoefs, CString("Spline coefficients computation"));
        prototype.addInputTrigger(
            SplineInterpolation_InputTriggerId_ComputeLaplacianCoefs, CString("Laplacian coefficients computation"));
        prototype.addInputTrigger(
            SplineInterpolation_InputTriggerId_InterpolateSpline, CString("Interpolation using spline coefficients"));
        prototype.addInputTrigger(
            SplineInterpolation_InputTriggerId_InterpolateLaplacian, CString("Interpolation using laplacian coefficients"));
        //output parameters
        prototype.addOutputParameter(
            SplineInterpolation_OutputParameterId_SamplePointsValues, "Interpolated values", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(
            SplineInterpolation_OutputParameterId_MinSamplePointValue, "Min interpolated value", Kernel::ParameterType_Float);
        prototype.addOutputParameter(
            SplineInterpolation_OutputParameterId_MaxSamplePointValue, "Max interpolated value", Kernel::ParameterType_Float);

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Algorithm_SphericalSplineInterpolationDesc)
};
}  // namespace Test
}  // namespace Plugins
}  // namespace OpenViBE
