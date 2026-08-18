#include "CAlgorithmSphericalSplineInterpolation.hpp"

//INSERM lib
#include "spline.hpp"

#include <cfloat> //DBL_MAX
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace Test {

bool CAlgorithmSphericalSplineInterpolation::initialize()

{
    m_firstProcess = true;
    m_coords.clear();
    m_coordsPtr.clear();
    m_splineCoefs.clear();
    m_laplacianCoefs.clear();

    ip_splineOrder.initialize(getInputParameter(SplineInterpolation_InputParameterId_SplineOrder));
    ip_nControlPoints.initialize(getInputParameter(SplineInterpolation_InputParameterId_ControlPointsCount));
    ip_controlPointsCoords.initialize(getInputParameter(SplineInterpolation_InputParameterId_ControlPointsCoord));
    ip_controlPointsValues.initialize(getInputParameter(SplineInterpolation_InputParameterId_ControlPointsValues));
    ip_samplePointsCoords.initialize(getInputParameter(SplineInterpolation_InputParameterId_SamplePointsCoord));

    op_samplePointsValues.initialize(getOutputParameter(SplineInterpolation_OutputParameterId_SamplePointsValues));
    op_samplePointsValues->setDimensionCount(1);
    op_minSamplePointValue.initialize(getOutputParameter(SplineInterpolation_OutputParameterId_MinSamplePointValue));
    op_maxSamplePointValue.initialize(getOutputParameter(SplineInterpolation_OutputParameterId_MaxSamplePointValue));

    return true;
}

bool CAlgorithmSphericalSplineInterpolation::uninitialize()

{
    ip_splineOrder.uninitialize();
    ip_nControlPoints.uninitialize();
    ip_controlPointsCoords.uninitialize();
    ip_controlPointsValues.uninitialize();
    ip_samplePointsCoords.uninitialize();

    op_samplePointsValues.uninitialize();
    op_minSamplePointValue.uninitialize();
    op_maxSamplePointValue.uninitialize();

    m_coords.clear();
    m_coordsPtr.clear();
    m_splineCoefs.clear();
    m_laplacianCoefs.clear();

    return true;
}

bool CAlgorithmSphericalSplineInterpolation::process()

{
    if (m_firstProcess) {
        //store coords as doubles
        m_coords.resize(3 * size_t(ip_nControlPoints));
        //set up matrix of pointers to double coords matrix
        m_coordsPtr.resize(size_t(ip_nControlPoints));
        //fill both matrices
        for (size_t i = 0; i < size_t(ip_nControlPoints); ++i) {
            const size_t id  = 3 * i;
            m_coords[id]     = double((*ip_controlPointsCoords)[id]);
            m_coords[id + 1] = double((*ip_controlPointsCoords)[id + 1]);
            m_coords[id + 2] = double((*ip_controlPointsCoords)[id + 2]);
            m_coordsPtr[i]   = id + m_coords.data();
        }
        m_firstProcess = false;
    }

    //do we want to precompute tables?
    if (isInputTriggerActive(SplineInterpolation_InputTriggerId_PrecomputeTables)) {
        //compute cos/sin values used in spline polynomias
        const int result = SplineTables(int(ip_splineOrder), m_pot.data(), m_scd.data());

        if (result != 0) {
            getLogManager() << Kernel::LogLevel_ImportantWarning << "Spline tables precomputation failed!\n";
            activateOutputTrigger(SplineInterpolation_OutputTriggerId_Error, true);
        }
    }

    if (isInputTriggerActive(SplineInterpolation_InputTriggerId_ComputeSplineCoefs)) {
        if (m_splineCoefs.empty() && size_t(ip_nControlPoints) != 0) { m_splineCoefs.resize(size_t(ip_nControlPoints) + 1); }

        //compute spline ponderation coefficients using spline values
        //FIXME : have a working copy of control points values stored as doubles?
        const int result = SplineCoef(int(ip_nControlPoints), m_coordsPtr.data(), ip_controlPointsValues->getBuffer(), m_pot.data(), m_splineCoefs.data());

        if (result != 0) {
            getLogManager() << Kernel::LogLevel_ImportantWarning << "Spline coefficients computation failed!\n";

            const Kernel::ELogLevel level = Kernel::LogLevel_Debug;

            getLogManager() << level << "CtrlPointsCount = " << int(ip_nControlPoints) << "\n";
            const auto size = size_t(ip_nControlPoints);
            std::stringstream ss;
            ss.fill('0');
            ss.precision(1);

            ss << "CtrlPointsCoords = ";
            for (size_t i = 0; i < size; ++i) { ss << std::fixed << "[" << m_coordsPtr[i][0] << " " << m_coordsPtr[i][1] << " " << m_coordsPtr[i][2] << "] "; }
            ss << "\n";
            getLogManager() << level << ss.str();

            ss.str("CtrlPointsValues = ");
            for (size_t i = 0; i < size; ++i) { ss << std::fixed << *(ip_controlPointsValues->getBuffer() + i) << " "; }
            ss << "\n";
            getLogManager() << level << ss.str();

            ss.str("Spline Coeffs    = ");
            for (size_t i = 0; i <= size; ++i) { ss << std::fixed << m_splineCoefs[i] << " "; }
            ss << "\n";
            getLogManager() << level << ss.str();

            ss.str("PotTable coeffs  = ");
            for (size_t i = 0; i < 10; ++i) { ss << std::fixed << m_pot[i] << " "; }
            ss << " ... ";
            for (size_t i = 2001; i < 2004; ++i) { ss << std::fixed << m_pot[i] << " "; }
            ss << "\n";
            getLogManager() << level << ss.str();

            activateOutputTrigger(SplineInterpolation_OutputTriggerId_Error, true);
        }
    }

    if (isInputTriggerActive(SplineInterpolation_InputTriggerId_ComputeLaplacianCoefs)) {
        if (m_laplacianCoefs.empty() && size_t(ip_nControlPoints) != 0) { m_laplacianCoefs.resize(size_t(ip_nControlPoints) + 1); }

        //compute spline ponderation coefficients using spline values
        //FIXME : have a working copy of control points values stored as doubles?
        const int result = SplineCoef(int(ip_nControlPoints), m_coordsPtr.data(), ip_controlPointsValues->getBuffer(), m_pot.data(), m_laplacianCoefs.data());
        m_laplacianCoefs[int(ip_nControlPoints)] = 0;

        if (result != 0) {
            getLogManager() << Kernel::LogLevel_ImportantWarning << "Laplacian coefficients computation failed!\n";
            activateOutputTrigger(SplineInterpolation_OutputTriggerId_Error, true);
        }
    }

    if (isInputTriggerActive(SplineInterpolation_InputTriggerId_InterpolateSpline)) {
        bool ok = true;

        //ensure we got enough storage space for interpolated values
        if (op_samplePointsValues->getDimensionSize(0) != ip_samplePointsCoords->getDimensionSize(0)) {
            op_samplePointsValues->setDimensionSize(0, ip_samplePointsCoords->getDimensionSize(0));
        }

        //compute interpolated values using spline
        double* sampleValue = op_samplePointsValues->getBuffer();

        op_minSamplePointValue = +DBL_MAX;
        op_maxSamplePointValue = -DBL_MAX;

        for (size_t i = 0; i < ip_samplePointsCoords->getDimensionSize(0); i++, sampleValue++) {
#if defined TARGET_OS_Windows
#ifndef NDEBUG
            if (_finite(*(ip_samplePointsCoords->getBuffer() + 3 * i)) == 0 ||
                _finite(*(ip_samplePointsCoords->getBuffer() + 3 * i + 1)) == 0 ||
                _finite(*(ip_samplePointsCoords->getBuffer() + 3 * i + 2)) == 0) //tests whether a double is infinite or a NaN
            {
                getLogManager() << Kernel::LogLevel_ImportantWarning << "Bad interpolation point coordinates !\n";
                getLogManager() << Kernel::LogLevel_ImportantWarning << *(ip_samplePointsCoords->getBuffer() + 3 * i) << "\n";
                getLogManager() << Kernel::LogLevel_ImportantWarning << *(ip_samplePointsCoords->getBuffer() + 3 * i + 1) << "\n";
                getLogManager() << Kernel::LogLevel_ImportantWarning << *(ip_samplePointsCoords->getBuffer() + 3 * i + 2) << "\n";
                ok = false;
            }
#endif
#endif

            *sampleValue = SplineInterp(int(ip_nControlPoints), //number of fixed values
                                        m_coordsPtr.data(), //coordinates of fixed values
                                        m_pot.data(), //sin/cos table for spline
                                        m_splineCoefs.data(), //spline coefficients
                                        *(ip_samplePointsCoords->getBuffer() + 3 * i),
                                        *(ip_samplePointsCoords->getBuffer() + 3 * i + 1),
                                        *(ip_samplePointsCoords->getBuffer() + 3 * i + 2) //coordinate where to interpolate
            );

#if defined TARGET_OS_Windows
#ifndef NDEBUG
            if (_finite(*sampleValue) == 0) //tests whether a double is infinite or a NaN
            {
                getLogManager() << Kernel::LogLevel_ImportantWarning << "Interpolation fails !\n";
                getLogManager() << Kernel::LogLevel_ImportantWarning << *(ip_samplePointsCoords->getBuffer() + 3 * i) << "\n";
                getLogManager() << Kernel::LogLevel_ImportantWarning << *(ip_samplePointsCoords->getBuffer() + 3 * i + 1) << "\n";
                getLogManager() << Kernel::LogLevel_ImportantWarning << *(ip_samplePointsCoords->getBuffer() + 3 * i + 2) << "\n";
                ok = false;
                break;
            }
#endif
#endif

            if (*sampleValue < op_minSamplePointValue) { op_minSamplePointValue = *sampleValue; }
            if (*sampleValue > op_maxSamplePointValue) { op_maxSamplePointValue = *sampleValue; }
        }

        if (!ok) { activateOutputTrigger(SplineInterpolation_OutputTriggerId_Error, true); }
    }
    else if (isInputTriggerActive(SplineInterpolation_InputTriggerId_InterpolateLaplacian)) {
        //ensure we got enough storage space for interpolated values
        if (op_samplePointsValues->getDimensionSize(0) != ip_samplePointsCoords->getDimensionSize(0)) {
            op_samplePointsValues->setDimensionSize(0, ip_samplePointsCoords->getDimensionSize(0));
        }

        //compute interpolated values using spline
        auto* sampleValue = op_samplePointsValues->getBuffer();

        op_minSamplePointValue = +DBL_MAX;
        op_maxSamplePointValue = -DBL_MAX;

        for (size_t i = 0; i < ip_samplePointsCoords->getDimensionSize(0); i++, sampleValue++) {
            *sampleValue = SplineInterp(int(ip_nControlPoints), //number of fixed values
                                        m_coordsPtr.data(), //coordinates of fixed values
                                        m_scd.data(), //sin/cos table for laplacian
                                        m_laplacianCoefs.data(), //laplacian coefficients
                                        *(ip_samplePointsCoords->getBuffer() + 3 * i),
                                        *(ip_samplePointsCoords->getBuffer() + 3 * i + 1),
                                        *(ip_samplePointsCoords->getBuffer() + 3 * i + 2)); //coordinate where to interpolate

            /***************************************************************************/
            /*** Units : potential values being very often expressed as micro-Volts  ***/
            /***         SCD values should be multiplied by a scaling factor         ***/
            /***         to get nano-Amperes/m3                                      ***/
            /***         Since the output of SplineInterp corresponds to the        ***/
            /***         Laplacian operator only, SCD are obtained with a scaling    ***/
            /***         factor equal to 10-3 * sigma / (R*R)                        ***/
            /***         with sigma = conductivity of the scalp    = 0.45 Siemens/m  ***/
            /***         and  R     = radius of the spherical head = 0.09 m          ***/
            /***************************************************************************/
            *sampleValue = *sampleValue * (0.001 * 0.45 / 0.09 / 0.09);

            if (*sampleValue < op_minSamplePointValue) { op_minSamplePointValue = *sampleValue; }
            if (*sampleValue > op_maxSamplePointValue) { op_maxSamplePointValue = *sampleValue; }
        }
    }

    return true;
}

}  // namespace Test
}  // namespace Plugins
}  // namespace OpenViBE
