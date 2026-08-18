#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCApplyTemporalFilter.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void ComputeFilterInitialCondition(itpp::vec b, itpp::vec a, itpp::vec& zi)
{
    int na, j, i;
    na = length(a);
    // FIXME is it necessary to keep next line uncomment ?
    //int nb = length(b);

    //--------------------------------------
    // use sparse matrix to solve system of linear equations for initial conditions
    // zi are the steady-state states of the filter b(z)/a(z) in the state-space 
    //implementation of the 'filter' command.

    itpp::mat eye1;
    eye1 = itpp::eye(na - 1);

    itpp::mat eye2;
    eye2 = itpp::eye(na - 2);

    itpp::vec a1(na - 1);
    itpp::vec zeros1(na - 2);
    zeros1 = itpp::zeros(na - 2);
    itpp::vec b1(na - 1);
    itpp::vec a2(na - 1);

    for (j = 1; j < na; ++j)
    {
        a1[j - 1] = - a[j];
        b1[j - 1] = b[j];
        a2[j - 1] = a[j];
    }

    itpp::mat matConc01;
    itpp::mat matZeros1;

    matZeros1 = itpp::zeros(na - 2, 1);
    matConc01 = concat_vertical(eye2, transpose(matZeros1));

    itpp::mat matA1;
    matA1 = itpp::zeros(na - 1, 1);
    matA1.set_col(0, a1);

    itpp::mat matConc02;
    matConc02 = concat_horizontal(matA1, matConc01);

    itpp::mat matNum;
    matNum = eye1 - matConc02;


    itpp::vec vecDenom(na - 1);
    for (i = 0; i < na - 1; ++i) { vecDenom[i] = b1[i] - (a2[i] * b[0]); }

    zi = inv(matNum) * vecDenom;
}


void FilterIRR(itpp::vec b, itpp::vec a, itpp::vec data, itpp::vec v0, itpp::vec& dataFiltered, itpp::vec& vf)
{
    int i, j, iV0 = 0;
    double sumA, sumB;
    double sumVf;
    // FIXME is it necessary to keep next line uncomment ?
    //int na = length(a);
    const int nb   = length(b);
    const int size = length(data);

    if (size < nb)
    {
        for (i = 0; i < size; ++i)
        {
            sumB = 0.0;
            for (j = 0; j <= i; ++j) { sumB = sumB + (b[j] * data[i - j]); }


            sumA = 0.0;

            for (j = 0; j <= i; ++j) { sumA = sumA + (a[j] * dataFiltered[i - j]); }
            dataFiltered[i] = sumB - sumA + v0[i];
        }

        for (i = 0; i < (nb - 1); ++i)
        {
            sumVf      = 0.0;
            double tmp = 0.0;
            for (j = 0; j < (nb - 1); ++j)
            {
                if ((i + j) < (nb - 1))
                {
                    if ((size - 1 - j) >= 0)
                    {
                        sumVf = sumVf + (b[i + j + 1] * data[size - 1 - j]) - (a[i + j + 1] * dataFiltered[size - 1 - j]);
                        iV0   = i + j + 1;
                    }
                    if ((size - 1 - j) < 0) { tmp = v0[iV0]; }
                }
            }
            vf[i] = sumVf + tmp;
        }
    }
    else
    {
        for (i = 0; i < nb - 1; ++i)
        {
            sumB = 0.0;
            for (j = 0; j <= i; ++j) { sumB = sumB + (b[j] * data[i - j]); }
            sumA = 0.0;

            for (j = 0; j <= i; ++j) { sumA = sumA + (a[j] * dataFiltered[i - j]); }
            dataFiltered[i] = sumB - sumA + v0[i];
        }


        for (i = nb - 1; i < size; ++i)
        {
            sumB = 0.0;
            for (j = 0; j < nb; ++j) { sumB = sumB + (b[j] * data[i - j]); }
            sumA = 0.0;
            for (j = 0; j < nb; ++j) { sumA = sumA + (a[j] * dataFiltered[i - j]); }
            dataFiltered[i] = sumB - sumA;
        }

        for (i = 0; i < nb - 1; ++i)
        {
            sumVf = 0.0;
            for (j = i; j < nb - 1; ++j) { sumVf = sumVf + (b[j + 1] * data[size - 1 - j + i]) - (a[j + 1] * dataFiltered[size - 1 - j + i]); }
            vf[i] = sumVf;
        }
    }
}


void Filtfilt(const itpp::vec& b, const itpp::vec& a, itpp::vec data, itpp::vec& dataFiltered)
{
    int j;
    const int na                   = length(a);
    const int nb                   = length(b);
    const int dataSize             = length(data);
    const int lengthEdgeTransients = 3 * (nb - 1);

    itpp::vec xB  = itpp::zeros(dataSize + (2 * lengthEdgeTransients));
    itpp::vec yB  = itpp::zeros(dataSize + (2 * lengthEdgeTransients));
    itpp::vec yB2 = itpp::zeros(dataSize + (2 * lengthEdgeTransients));
    itpp::vec yC  = itpp::zeros(dataSize + (2 * lengthEdgeTransients));
    itpp::vec yC2 = itpp::zeros(dataSize + (2 * lengthEdgeTransients));


    itpp::vec zi(na - 1);
    ComputeFilterInitialCondition(b, a, zi);

    for (j = 0; j < lengthEdgeTransients; ++j) { xB[j] = (2 * data[0]) - data[lengthEdgeTransients - j]; }
    for (j = 0; j < dataSize; ++j) { xB[j + lengthEdgeTransients] = data[j]; }
    for (j = 0; j < lengthEdgeTransients; ++j) { xB[j + lengthEdgeTransients + dataSize] = (2 * data[dataSize - 1]) - data[dataSize - j - 2]; }

    itpp::vec ziChan(na - 1);
    for (j = 0; j < na - 1; ++j) { ziChan[j] = zi[j] * xB[0]; }

    itpp::vec finalStates(na - 1);

    FilterIRR(b, a, xB, ziChan, yB, finalStates);

    for (j = 0; j < dataSize + (2 * lengthEdgeTransients); ++j) { yC[j] = yB[(dataSize + (2 * lengthEdgeTransients)) - 1 - j]; }

    itpp::vec ziChan2(na - 1);

    for (j = 0; j < na - 1; ++j) { ziChan2[j] = zi[j] * yC[0]; }

    FilterIRR(b, a, yC, ziChan2, yB2, finalStates);


    for (j = 0; j < dataSize + (2 * lengthEdgeTransients); ++j) { yC2[j] = yB2[(dataSize + (2 * lengthEdgeTransients)) - 1 - j]; }
    for (j = 0; j < dataSize; ++j) { dataFiltered[j] = yC2[j + lengthEdgeTransients]; }
}


bool CApplyTemporalFilter::initialize()
{
    bool res = true;
    res &= ip_signalMatrix.initialize(getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix));
    res &= ip_filterCoefsMatrix.initialize(getInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefsMatrix));
    res &= op_signalMatrix.initialize(getOutputParameter(OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix));
    return res;
}

bool CApplyTemporalFilter::uninitialize()
{
    op_signalMatrix.uninitialize();
    ip_filterCoefsMatrix.uninitialize();
    ip_signalMatrix.uninitialize();
    return true;
}

//  
//

bool CApplyTemporalFilter::process()
{
    CMatrix* iMatrix = ip_signalMatrix;
    CMatrix* oMatrix = op_signalMatrix;

    if (isInputTriggerActive(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize))
    {
        m_flagInitialize = true;

        oMatrix->copyDescription(*iMatrix);

        // dimension of input coef (numerator, denominator) filter
        const size_t filterCoefNumeratorDimSize   = ip_filterCoefsMatrix->getDimensionSize(0);
        const size_t filterCoefDenominatorDimSize = ip_filterCoefsMatrix->getDimensionSize(0);

        //coef filters vars
        CMatrix* filterCoefInputMatrix = ip_filterCoefsMatrix;
        double* filterCoefInput        = filterCoefInputMatrix->getBuffer();

        m_coefFilterDen = itpp::zeros(filterCoefDenominatorDimSize);
        m_coefFilterNum = itpp::zeros(filterCoefNumeratorDimSize);


        for (size_t i = 0; i < filterCoefNumeratorDimSize; ++i) { m_coefFilterNum[i] = filterCoefInput[i]; }


        for (size_t i = 0; i < filterCoefDenominatorDimSize; ++i) { m_coefFilterDen[i] = filterCoefInput[filterCoefNumeratorDimSize + i]; }
    }

    // This mode is used when the consecutive input chunks are discontinuous in time
    if (isInputTriggerActive(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter))
    {
        // signal input vars
        double* input = iMatrix->getBuffer();

        // dimension of input signal buffer
        const size_t nDim      = ip_signalMatrix->getDimensionCount();
        const size_t nChannels = ip_signalMatrix->getDimensionSize(0);
        const size_t nEpoch    = ip_signalMatrix->getDimensionSize(1);

        // signal output vars
        oMatrix->setDimensionCount(nDim);
        for (size_t i = 0; i < nDim; ++i) { oMatrix->setDimensionSize(i, ip_signalMatrix->getDimensionSize(i)); }
        double* filteredSignalMatrix = oMatrix->getBuffer();

        itpp::vec y(nEpoch);
        itpp::vec x = itpp::zeros(nEpoch);

        // test that Filtfilt() won't exceed the data array boundaries
        const size_t minSize = 3 * (m_coefFilterDen.size() - 1) + 1;
        if (nEpoch < minSize)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Data chunk size (" << nEpoch << ") "
                    << "is too short for the requirements of the filter (" << minSize << "). Please use a larger chunk size.\n";
            return false;
        }

        for (size_t i = 0; i < nChannels; ++i)
        {
            for (size_t j = 0; j < nEpoch; ++j) { x[int(j)] = double(input[i * nEpoch + j]); }

            // --- Modif Manu
            Filtfilt(m_coefFilterNum, m_coefFilterDen, x, y);
            // --- Fin Modif Manu

            for (size_t k = 0; k < nEpoch; ++k) { filteredSignalMatrix[i * nEpoch + k] = y[k]; }
        }
    }

    // This mode is used when the consecutive input chunks are continuous in time
    if (isInputTriggerActive(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric))
    {
        // signal input vars
        double* input = iMatrix->getBuffer();

        // dimension of input signal biuffer
        const size_t nDim      = ip_signalMatrix->getDimensionCount();
        const size_t nChannels = ip_signalMatrix->getDimensionSize(0);
        const size_t nEpoch    = ip_signalMatrix->getDimensionSize(1);

        // historic buffers
        if (m_flagInitialize)
        {
            // --- Modif Manu
            itpp::vec zi = itpp::zeros(int(ip_filterCoefsMatrix->getDimensionSize(0) - 1));
            ComputeFilterInitialCondition(m_coefFilterNum, m_coefFilterDen, zi);

            m_currentStates.resize(nChannels);
            for (size_t i = 0; i < nChannels; ++i) { m_currentStates[i] = zi * double(input[i * nEpoch]); }
            // --- Fin Modif Manu

            m_flagInitialize = false;
        }

        // signal output vars
        oMatrix->setDimensionCount(nDim);
        for (size_t i = 0; i < nDim; ++i) { oMatrix->setDimensionSize(i, ip_signalMatrix->getDimensionSize(i)); }
        double* filteredSignalMatrix = oMatrix->getBuffer();

        itpp::vec x = itpp::zeros(nEpoch);

        itpp::vec y(nEpoch);
        //y = zeros(nEpoch);

        for (size_t i = 0; i < nChannels; ++i)
        {
            for (size_t j = 0; j < nEpoch; ++j) { x[j] = double(input[i * nEpoch + j]); }
            // --- Modif Manu
            y = itpp::zeros(nEpoch);
            FilterIRR(m_coefFilterNum, m_coefFilterDen, x, m_currentStates[i], y, m_currentStates[i]);
            // --- Fin Modif Manu

            for (size_t k = 0; k < nEpoch; ++k) { filteredSignalMatrix[i * nEpoch + k] = y[k]; }
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
