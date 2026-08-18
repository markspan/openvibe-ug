#pragma once

#include "../../defines.hpp"

#include "CBufferDatabase.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CTopographicMapDrawable : public CSignalDisplayDrawable
{
public:
    ~CTopographicMapDrawable() override = default;
    virtual CMatrix* GetSampleCoordinatesMatrix() = 0;
    virtual bool SetSampleValuesMatrix(CMatrix* pSampleValuesMatrix) = 0;
};

/**
* This class is used to store information about the incoming signal stream. It can request a CSignalDisplayDrawable
* object to redraw himself in case of some changes in its data.
*/
class CTopographicMapDatabase final : public CBufferDatabase
{
public:
    CTopographicMapDatabase(Toolkit::TBoxAlgorithm<IBoxAlgorithm>& plugin, Kernel::IAlgorithmProxy& interpolation);
    ~CTopographicMapDatabase() override = default;

    void SetMatrixDimensionSize(const size_t index, const size_t size) override;

    /**
     * \brief Callback called upon channel localisation buffer reception
     * \param bufferIndex Index of newly received channel localisation buffer
     * \return True if buffer data was correctly processed, false otherwise
     */
    bool OnChannelLocalisationBufferReceived(const size_t bufferIndex) override;

    bool SetDelay(double delay);

    /**
     * \brief Set interpolation type
     * Spline values (potentials) can be interpolated directly, but the spline laplacian (currents) may
     * be used as well
     * \sa TypeId_SphericalLinearInterpolation enumeration
     */
    void SetInterpolationType(const EInterpolationType type) { m_interpolationType = type; }

    bool ProcessValues();

    bool InterpolateValues();

    //! Returns min/max interpolated values using the last buffer arrived (all channels taken into account)
    void GetLastBufferInterpolatedMinMaxValue(double& min, double& max) const;

private:
    /**
     * \brief Looks for buffer whose timeframe contains time passed as parameter
     * \param time [in] Time of buffer to be retrieved
     * \param bufferIndex [out] Index of buffer closest to time passed as parameter
     * \return True if time passed as parameter lies within a buffer's timeframe, false otherwise
     */
    bool getBufferIndexFromTime(uint64_t time, size_t& bufferIndex) const;

    /**
     * \brief Ensure electrode coordinates are normalized
     * \return True if all electrode coordinates are normalized, false otherwise
     */
    bool checkElectrodeCoordinates();

    //true until process() is called for the first time
    bool m_firstProcess = true;
    //spherical spline interpolation
    Kernel::IAlgorithmProxy& m_interpolation;
    //order of spherical spline used for interpolation - mapped to SplineInterpolation_InputParameterId_SplineOrder
    int64_t m_splineOrder = 4;
    /**
     * \brief Type of interpolation
     * \sa TypeId_SphericalLinearInterpolation enumeration
     */
    EInterpolationType m_interpolationType = EInterpolationType::Spline;
    //number of electrodes (see CBufferDatabase) - mapped to SplineInterpolation_InputParameterId_ControlPointsCount
    //size_t m_NElectrodes = 0;
    //flag set to true once electrode coordinates have been initialized
    bool m_electrodeCoordsInitialized = false;
    //electrode cartesian coordinates, in normalized space (X right Y front Z up)
    CMatrix m_electrodeCoords;
    //pointer to electrode coordinates matrix - mapped to SplineInterpolation_InputParameterId_ControlPointsCoord
    CMatrix* m_pElectrodeCoords = nullptr;
    //electrode potentials
    CMatrix m_electrodePotentials;
    //pointer to electrode potentials matrix - mapped to SplineInterpolation_InputParameterId_ControlPointsValues
    CMatrix* m_pElectrodePotentials = nullptr;
    //pointer to sample points coordinates matrix - mapped to SplineInterpolation_InputParameterId_SamplePointsCoord
    CMatrix* m_samplePointCoords = nullptr;
    //minimum interpolated value
    Kernel::TParameterHandler<double> m_minSamplePointValue;
    //maximum interpolated value
    Kernel::TParameterHandler<double> m_maxSamplePointValue;
    //delay to apply to interpolated values
    uint64_t m_delay = 0;
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
