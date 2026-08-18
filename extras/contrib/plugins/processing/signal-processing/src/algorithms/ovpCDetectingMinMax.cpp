#include "ovpCDetectingMinMax.h"

#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
// ________________________________________________________________________________________________________________
//

bool CDetectingMinMax::initialize()
{
    ip_signalMatrix.initialize(getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_SignalMatrix));
    ip_sampling.initialize(getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_Sampling));
    ip_timeWindowStart.initialize(getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowStart));
    ip_timeWindowEnd.initialize(getInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowEnd));

    op_signalMatrix.initialize(getOutputParameter(OVP_Algorithm_DetectingMinMax_OutputParameterId_SignalMatrix));

    return true;
}

bool CDetectingMinMax::uninitialize()
{
    op_signalMatrix.uninitialize();

    ip_timeWindowEnd.uninitialize();
    ip_timeWindowStart.uninitialize();
    ip_sampling.uninitialize();
    ip_signalMatrix.uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CDetectingMinMax::process()
{
    // signal input vars
    CMatrix* iMatrix = ip_signalMatrix;
    double* ibuffer  = iMatrix->getBuffer();

    // signal output vars
    CMatrix* oMatrix = op_signalMatrix;
    oMatrix->resize(1, 1);

    double* oBuffer = oMatrix->getBuffer();

    if (isInputTriggerActive(OVP_Algorithm_DetectingMinMax_InputTriggerId_Initialize)) { }

    if (isInputTriggerActive(OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMin))
    {
        // dimension of input signal biuffer
        const size_t nChannel  = ip_signalMatrix->getDimensionSize(0);
        const size_t EpochSize = ip_signalMatrix->getDimensionSize(1);

        // Must be changed
        double minValue = 1E10;

        for (size_t i = 0; i < nChannel; ++i)
        {
            for (size_t j = 0; j < EpochSize; ++j) { if (ibuffer[i * EpochSize + j] < minValue) { minValue = ibuffer[i * EpochSize + j]; } }
        }
        oBuffer[0] = minValue;
    }

    if (isInputTriggerActive(OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMax))
    {
        // dimension of input signal biuffer
        const size_t nChannels = ip_signalMatrix->getDimensionSize(0);
        const size_t epochSize = ip_signalMatrix->getDimensionSize(1);

        // Must be changed
        double maxValue = -1E10;

        for (size_t i = 0; i < nChannels; ++i)
        {
            const uint64_t start = uint64_t(floor(ip_timeWindowStart / 1000. * ip_sampling));
            const uint64_t stop  = uint64_t(floor(ip_timeWindowEnd / 1000. * ip_sampling));
            for (uint64_t j = start; j < stop; ++j) { if (ibuffer[i * epochSize + j] > maxValue) { maxValue = ibuffer[i * epochSize + j]; } }
        }
        oBuffer[0] = maxValue;
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
