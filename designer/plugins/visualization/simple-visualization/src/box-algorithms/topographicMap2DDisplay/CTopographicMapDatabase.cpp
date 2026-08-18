#include "CTopographicMapDatabase.hpp"

#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

CTopographicMapDatabase::CTopographicMapDatabase(Toolkit::TBoxAlgorithm<IBoxAlgorithm>& plugin, Kernel::IAlgorithmProxy& interpolation)
    : CBufferDatabase(plugin), m_interpolation(interpolation)
{
    //map input parameters
    //--------------------

    //spline order
    m_interpolation.getInputParameter(SplineInterpolation_InputParameterId_SplineOrder)->setReferenceTarget(&m_splineOrder);
    //number of channels (or electrodes)
    m_interpolation.getInputParameter(SplineInterpolation_InputParameterId_ControlPointsCount)->setReferenceTarget(&m_NElectrodes);
    //matrix of pointers to electrode coordinates
    m_pElectrodeCoords = &m_electrodeCoords;
    m_interpolation.getInputParameter(SplineInterpolation_InputParameterId_ControlPointsCoord)->setReferenceTarget(
        &m_pElectrodeCoords);
    //matrix of potentials measured at each electrode
    m_pElectrodePotentials = &m_electrodePotentials;
    m_interpolation.getInputParameter(SplineInterpolation_InputParameterId_ControlPointsValues)->setReferenceTarget(
        &m_pElectrodePotentials);
    //matrix holding sample coordinates mapped at runtime (its size is not known a priori and may vary)
    //

    //map output parameters
    //---------------------
    m_minSamplePointValue.initialize(m_interpolation.getOutputParameter(SplineInterpolation_OutputParameterId_MinSamplePointValue));
    m_maxSamplePointValue.initialize(m_interpolation.getOutputParameter(SplineInterpolation_OutputParameterId_MaxSamplePointValue));
}

void CTopographicMapDatabase::SetMatrixDimensionSize(const size_t index, const size_t size)
{
    CBufferDatabase::SetMatrixDimensionSize(index, size);

    if (index == 0) { m_electrodePotentials.resize(m_NElectrodes); }
}

bool CTopographicMapDatabase::OnChannelLocalisationBufferReceived(const size_t bufferIndex)
{
    CBufferDatabase::OnChannelLocalisationBufferReceived(bufferIndex);

    if (!m_ChannelLookupTableInitialized || m_channelLocalisationCoords.empty() || m_NElectrodes == 0) {
        m_ParentPlugin.getLogManager() << Kernel::LogLevel_Warning
                << "Channel localisation buffer received before channel lookup table was initialized! Can't process buffer!\n";
    }

    //static electrode coordinates
    if (!m_dynamicChannelLocalisation) {
        //if streamed coordinates are cartesian
        if (m_cartesianCoords) {
            //fill electrode coordinates matrix
            m_electrodeCoords.resize(3 * m_NElectrodes);
            const double* coords = m_channelLocalisationCoords[0].first->getBuffer();
            for (size_t i = 0; i < m_NElectrodes; ++i) {
                const size_t lookupIdx       = m_ChannelLookupIndices[i];
                m_electrodeCoords[3 * i]     = *(coords + 3 * lookupIdx);
                m_electrodeCoords[3 * i + 1] = *(coords + 3 * lookupIdx + 1);
                m_electrodeCoords[3 * i + 2] = *(coords + 3 * lookupIdx + 2);
            }

            //electrode coordinates initialized : it is now possible to interpolate potentials
            m_electrodeCoordsInitialized = true;
        }
    }

    return true;
}

void CTopographicMapDatabase::GetLastBufferInterpolatedMinMaxValue(double& min, double& max) const
{
    min = m_minSamplePointValue;
    max = m_maxSamplePointValue;
}

bool CTopographicMapDatabase::ProcessValues()
{
    //wait for electrode coordinates
    if (!m_electrodeCoordsInitialized) { return true; }

    if (m_firstProcess) {
        //done in CBufferDatabase::setMatrixBuffer
        //initialize the drawable object
        //m_Drawable->init();

        if (!checkElectrodeCoordinates()) { return false; }

        //precompute sin/cos tables
        m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_PrecomputeTables, true);

        m_firstProcess = false;
    }

    //retrieve electrode values
    //determine what buffer to use from delay
    size_t bufferIdx           = 0;
    const uint64_t currentTime = m_ParentPlugin.getPlayerContext().getCurrentTime();
    const uint64_t displayTime = currentTime - m_delay;
    getBufferIndexFromTime(displayTime, bufferIdx);

    //determine what sample to use
    size_t sampleIdx;
    if (displayTime <= m_StartTime[bufferIdx]) { sampleIdx = 0; }
    else if (displayTime >= m_EndTime[bufferIdx]) { sampleIdx = m_DimSizes[1] - 1; }
    else { sampleIdx = size_t(double(displayTime - m_StartTime[bufferIdx]) / double(m_BufferDuration * m_DimSizes[1])); }

    for (size_t i = 0; i < m_NElectrodes; ++i) { *(m_electrodePotentials.getBuffer() + i) = m_SampleBuffers[bufferIdx][i * m_DimSizes[1] + sampleIdx]; }

    //interpolate spline values (potentials)
    if (m_interpolationType == EInterpolationType::Spline) {
        m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_ComputeSplineCoefs, true);
    }
    else //interpolate spline laplacian (currents)
    {
        m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_ComputeLaplacianCoefs, true);
    }

    //retrieve up-to-date pointer to sample matrix
    m_samplePointCoords = dynamic_cast<CTopographicMapDrawable*>(m_Drawable)->GetSampleCoordinatesMatrix();

    if (m_samplePointCoords != nullptr) {
        //map pointer to input parameter
        m_interpolation.getInputParameter(SplineInterpolation_InputParameterId_SamplePointsCoord)->
                        setReferenceTarget(&m_samplePointCoords);

        if (m_interpolationType == EInterpolationType::Spline) {
            m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_InterpolateSpline, true);
        }
        else { m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_InterpolateLaplacian, true); }
    }

    m_interpolation.process();
    bool process = true;
    if (m_interpolation.isOutputTriggerActive(SplineInterpolation_OutputTriggerId_Error)) {
        m_ParentPlugin.getLogManager() << Kernel::LogLevel_Warning << "An error occurred while interpolating potentials!\n";
        process = false;
    }
    else {
        if (m_samplePointCoords != nullptr) {
            //retrieve interpolation results
            Kernel::TParameterHandler<CMatrix*> sampleValuesMatrix;
            sampleValuesMatrix.initialize(m_interpolation.getOutputParameter(SplineInterpolation_OutputParameterId_SamplePointsValues));
            dynamic_cast<CTopographicMapDrawable*>(m_Drawable)->SetSampleValuesMatrix(sampleValuesMatrix);

            //tells the drawable to redraw itself since the signal information has been updated
            m_Drawable->Redraw();
        }
    }

    return process;
}

bool CTopographicMapDatabase::SetDelay(const double delay)
{
    if (delay > m_TotalDuration) { return false; }

    //convert delay to 32:32 format
    m_delay = int64_t(delay * (1LL << 32)); // $$$ Casted in (int64_t) because of Ubuntu 7.10 crash !
    return true;
}

bool CTopographicMapDatabase::InterpolateValues()
{
    //can't interpolate before first buffer has been received
    if (m_firstProcess) { return false; }

    //retrieve up-to-date pointer to sample matrix
    m_samplePointCoords = dynamic_cast<CTopographicMapDrawable*>(m_Drawable)->GetSampleCoordinatesMatrix();

    if (m_samplePointCoords != nullptr) {
        //map pointer to input parameter
        m_interpolation.getInputParameter(SplineInterpolation_InputParameterId_SamplePointsCoord)->
                        setReferenceTarget(&m_samplePointCoords);

        //interpolate using spline or laplacian coefficients depending on interpolation mode
        if (m_interpolationType == EInterpolationType::Spline) {
            m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_InterpolateSpline, true);
        }
        else { m_interpolation.activateInputTrigger(SplineInterpolation_InputTriggerId_InterpolateLaplacian, true); }
    }

    m_interpolation.process();

    if (m_interpolation.isOutputTriggerActive(SplineInterpolation_OutputTriggerId_Error)) {
        m_ParentPlugin.getLogManager() << Kernel::LogLevel_Warning << "An error occurred while interpolating potentials!\n";
    }
    else {
        if (m_samplePointCoords != nullptr) {
            //retrieve interpolation results
            Kernel::TParameterHandler<CMatrix*> sampleValuesMatrix;
            sampleValuesMatrix.initialize(m_interpolation.getOutputParameter(SplineInterpolation_OutputParameterId_SamplePointsValues));
            dynamic_cast<CTopographicMapDrawable*>(m_Drawable)->SetSampleValuesMatrix(sampleValuesMatrix);
        }
    }

    return true;
}

bool CTopographicMapDatabase::getBufferIndexFromTime(const uint64_t time, size_t& bufferIndex) const
{
    if (m_SampleBuffers.empty()) { return false; }

    if (time < m_StartTime[0]) {
        bufferIndex = 0;
        return false;
    }
    if (time > m_EndTime.back()) {
        bufferIndex = size_t(m_SampleBuffers.size() - 1);
        return false;
    }
    for (size_t i = 0; i < m_SampleBuffers.size(); ++i) {
        if (time <= m_EndTime[i]) {
            bufferIndex = i;
            break;
        }
    }

    return true;
}

bool CTopographicMapDatabase::checkElectrodeCoordinates()
{
    const size_t nChannel = GetChannelCount();

    for (size_t i = 0; i < nChannel; ++i) {
        double* normalizedChannelCoords = nullptr;
        if (!GetChannelPosition(i, normalizedChannelCoords)) {
            CString channelLabel;
            GetChannelLabel(i, channelLabel);
            m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error
                    << "Couldn't retrieve coordinates of electrode #" << i
                    << "(" << channelLabel << "), aborting model frame electrode coordinates computation\n";
            return false;
        }

#define MY_THRESHOLD 0.01
        if (fabs(normalizedChannelCoords[0] * normalizedChannelCoords[0] +
                 normalizedChannelCoords[1] * normalizedChannelCoords[1] +
                 normalizedChannelCoords[2] * normalizedChannelCoords[2] - 1.) > MY_THRESHOLD)
#undef MY_THRESHOLD
        {
            CString channelLabel;
            GetChannelLabel(i, channelLabel);
            m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error
                    << "Coordinates of electrode #" << i << "(" << channelLabel
                    << "), are not normalized, aborting model frame electrode coordinates computation\n";
            return false;
        }
    }

    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
