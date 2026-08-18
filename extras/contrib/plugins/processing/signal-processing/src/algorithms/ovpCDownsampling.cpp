#include "ovpCDownsampling.h"

#include <cmath> //floor, ceil
#include <cstdlib>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
// ________________________________________________________________________________________________________________
//

bool CDownsampling::initialize()
{
    ip_sampling.initialize(getInputParameter(OVP_Algorithm_Downsampling_InputParameterId_Sampling));
    ip_newSampling.initialize(getInputParameter(OVP_Algorithm_Downsampling_InputParameterId_NewSampling));
    ip_signalMatrix.initialize(getInputParameter(OVP_Algorithm_Downsampling_InputParameterId_SignalMatrix));
    op_signalMatrix.initialize(getOutputParameter(OVP_Algorithm_Downsampling_OutputParameterId_SignalMatrix));

    m_lastValueOrigSignal = nullptr;
    m_first               = true;

    return true;
}

bool CDownsampling::uninitialize()
{
    free(m_lastValueOrigSignal);
    m_lastValueOrigSignal = nullptr;

    op_signalMatrix.uninitialize();
    ip_signalMatrix.uninitialize();
    ip_newSampling.uninitialize();
    ip_sampling.uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CDownsampling::process()
{
    size_t oEpochSize, indexBegOutput;
    double blocDuration, endTime;

    // signal input vars
    CMatrix* iMatrix        = ip_signalMatrix;
    double* iBuffer         = iMatrix->getBuffer();
    const size_t nChannels  = ip_signalMatrix->getDimensionSize(0);
    const size_t iEpochSize = ip_signalMatrix->getDimensionSize(1);

    const double iSampling = double(ip_sampling);
    const double oSampling = double(ip_newSampling);

    // signal output vars
    CMatrix* oMatrix = op_signalMatrix;
    oMatrix->setDimensionCount(ip_signalMatrix->getDimensionCount());
    if ((m_first) || (isInputTriggerActive(OVP_Algorithm_Downsampling_InputTriggerId_Resample)))
    {
        oEpochSize = size_t(floor(iEpochSize * (oSampling / iSampling)));

        blocDuration = double(iEpochSize - 1) / iSampling;
        endTime      = blocDuration;

        m_lastTimeOrigSignal = 0;
        m_lastTimeNewSignal  = 0;

        if (m_first)
        {
            m_lastValueOrigSignal = static_cast<double*>(calloc(nChannels, sizeof(double)));
            if (m_lastValueOrigSignal == nullptr) { this->getLogManager() << Kernel::LogLevel_Error << "Memory allocation : last values of original signal.\n"; }
        }
    }
    else
    {
        blocDuration            = double(iEpochSize) / iSampling;
        endTime                 = m_lastTimeOrigSignal + blocDuration;
        const double timePassed = endTime - m_lastTimeNewSignal;
        oEpochSize              = size_t(floor(timePassed * oSampling));
    }

    if (oEpochSize == 0)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Output epoch size is 0. Increase input epoch size.\n";
        return false;
    }

    // this->getLogManager() << Kernel::LogLevel_Info << "blockDur " << blocDuration << " et " << endTime << " lt " << m_lastTimeNewSignal << " td " << timeDiff << " dim " << oMatrixDimensionSizeEpoch << "\n";

    oMatrix->setDimensionSize(0, nChannels);
    oMatrix->setDimensionSize(1, oEpochSize);
    double* oBuffer = oMatrix->getBuffer();

    if (isInputTriggerActive(OVP_Algorithm_Downsampling_InputTriggerId_Initialize)) { }

    if ((isInputTriggerActive(OVP_Algorithm_Downsampling_InputTriggerId_ResampleWithHistoric))
        || (isInputTriggerActive(OVP_Algorithm_Downsampling_InputTriggerId_Resample)))
    {
        double countNew = 0, prev;
        int indexInput;

        for (size_t i = 0; i < nChannels; ++i)
        {
            double countOrig = m_lastTimeOrigSignal;
            countNew         = m_lastTimeNewSignal + (1.0 / double(ip_newSampling));
            double timePrev  = m_lastTimeOrigSignal;
            if ((m_first) || (isInputTriggerActive(OVP_Algorithm_Downsampling_InputTriggerId_Resample)))
            {
                prev                    = iBuffer[i * iEpochSize];
                oBuffer[i * oEpochSize] = prev;
                indexBegOutput          = 1;
                indexInput              = 0;
            }
            else
            {
                prev           = m_lastValueOrigSignal[i];
                indexBegOutput = 0;
                indexInput     = -1;
            }
            for (uint64_t j = indexBegOutput; j < oEpochSize; ++j)
            {
                while ((indexInput < int(iEpochSize)) && (countOrig < countNew))
                {
                    countOrig += 1.0 / double(ip_sampling);
                    indexInput++;
                }

                if (indexInput == -1) { this->getLogManager() << Kernel::LogLevel_Warning << "Downsampling problem : index value=-1\n"; }
                else if (indexInput < int(iEpochSize))
                {
                    const double cur              = iBuffer[(i * iEpochSize) + indexInput];
                    oBuffer[(i * oEpochSize) + j] = ((cur - prev) * (countNew - timePrev) / (countOrig - timePrev)) + prev;
                    prev                          = cur;
                    timePrev                      = countOrig;
                }
                else
                {
                    this->getLogManager() << Kernel::LogLevel_Warning << "Downsampling problem : sample #" << j << "/" << oEpochSize <<
                            " time original signal=" << countOrig << " time new signal=" << countNew << " new signal sample #" << indexInput <<
                            " /" << iEpochSize << "\n";
                    j = oEpochSize;
                }

                countNew += 1.0 / oSampling;
            }

            if (oEpochSize > 0) { m_lastValueOrigSignal[i] = iBuffer[(i * iEpochSize) + iEpochSize - 1]; }
        }
        if (oEpochSize > 0)
        {
            m_lastTimeNewSignal  = countNew - (1.0 / oSampling);
            m_lastTimeOrigSignal = endTime;
        }
        if (m_first) { m_first = false; }
    }


    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
