#include "ovpCAlgorithmUnivariateStatistics.h"

#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

// ________________________________________________________________________________________________________________
//

bool CAlgoUnivariateStatistic::initialize()
{
    ip_matrix.initialize(getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_Matrix));
    op_MeanMatrix.initialize(getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Mean));
    op_VarianceMatrix.initialize(getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Var));
    op_RangeMatrix.initialize(getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Range));
    op_MedianMatrix.initialize(getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Med));
    op_IQRMatrix.initialize(getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_IQR));
    op_PercentileMatrix.initialize(getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Percent));

    ip_isMeanActive.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_MeanActive));
    ip_isVarianceActive.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_VarActive));
    ip_isRangeActive.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_RangeActive));
    ip_isMedianActive.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_MedActive));
    ip_isIQRActive.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_IQRActive));
    ip_isPercentileActive.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentActive));

    ip_percentileValue.initialize(this->getInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentValue));
    op_compression.initialize(this->getOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Compression));

    return true;
}

bool CAlgoUnivariateStatistic::uninitialize()
{
    op_compression.uninitialize();
    ip_percentileValue.uninitialize();

    ip_isMeanActive.uninitialize();
    ip_isVarianceActive.uninitialize();
    ip_isRangeActive.uninitialize();
    ip_isMedianActive.uninitialize();
    ip_isIQRActive.uninitialize();
    ip_isPercentileActive.uninitialize();

    op_MeanMatrix.uninitialize();
    op_VarianceMatrix.uninitialize();
    op_RangeMatrix.uninitialize();
    op_MedianMatrix.uninitialize();
    op_IQRMatrix.uninitialize();
    op_PercentileMatrix.uninitialize();
    ip_matrix.uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CAlgoUnivariateStatistic::process()
{
    CMatrix* matrix     = ip_matrix;
    CMatrix* mean       = op_MeanMatrix;
    CMatrix* variance   = op_VarianceMatrix;
    CMatrix* range      = op_RangeMatrix;
    CMatrix* median     = op_MedianMatrix;
    CMatrix* iqr        = op_IQRMatrix;
    CMatrix* percentile = op_PercentileMatrix;

    if (this->isInputTriggerActive(OVP_Algorithm_UnivariateStatistic_InputTriggerId_Initialize))
    {
        this->getLogManager() << Kernel::LogLevel_Debug << "input : " << matrix->getDimensionCount() << " : " << matrix->getDimensionSize(0) << "*" <<
                matrix->getDimensionSize(1) << "\n";

        //initialize matrix output
        if (!setMatrixDimension(mean, matrix)) { return false; }
        if (!setMatrixDimension(variance, matrix)) { return false; }
        if (!setMatrixDimension(range, matrix)) { return false; }
        if (!setMatrixDimension(median, matrix)) { return false; }
        if (!setMatrixDimension(iqr, matrix)) { return false; }
        if (!setMatrixDimension(percentile, matrix)) { return false; }

        //inform about the compression on sampling rate due to this operation N->1 :=: fq->fq/N
        op_compression = 1 / double(matrix->getDimensionSize(1));

        //percentile value
        m_percentileValue = ip_percentileValue;
        //select operation to do (avoid unuseful calculus)
        m_isSumActive      = ip_isMeanActive || ip_isVarianceActive;
        m_isSqaresumActive = ip_isVarianceActive;
        m_isSortActive     = ip_isRangeActive || ip_isMedianActive || ip_isIQRActive || ip_isPercentileActive;

        if (m_isSumActive)
        {
            m_sumMatrix.copyDescription(*matrix);
            m_sumMatrix.setDimensionSize(1, 1);
        }

        if (m_isSqaresumActive)
        {
            m_sumMatrix2.copyDescription(*matrix);
            m_sumMatrix2.setDimensionSize(1, 1);
        }

        if (m_isSortActive) { m_sortMatrix.copyDescription(*matrix); }
    }

    if (this->isInputTriggerActive(OVP_Algorithm_UnivariateStatistic_InputTriggerId_Process))
    {
        ///make necessary operations
        //dimension
        const double s = double(matrix->getDimensionSize(1));
        //sum, sum square, sort
        std::vector<double> vect(matrix->getDimensionSize(1));
        for (size_t i = 0; i < matrix->getDimensionSize(0); ++i)
        {
            if (m_isSortActive)
            {
                //copy fonctionne pas car le buffer n'est pas unidirectionnel...
                for (size_t j = 0; j < matrix->getDimensionSize(1); ++j) { vect[j] = matrix->getBuffer()[i * matrix->getDimensionSize(1) + j]; }
                std::sort(vect.begin(), vect.end());
            }

            double y = 0, y2 = 0;
            for (size_t j = 0; j < matrix->getDimensionSize(1); ++j)
            {
                const double x = matrix->getBuffer()[i * matrix->getDimensionSize(1) + j];
                if (m_isSumActive) { y += x; }
                if (m_isSqaresumActive) { y2 += x * x; }
                if (m_isSortActive) { m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + j] = vect.at(j); }
            }

            if (m_isSumActive) { m_sumMatrix.getBuffer()[i * m_sumMatrix.getDimensionSize(1)] = y; }
            if (m_isSqaresumActive) { m_sumMatrix2.getBuffer()[i * m_sumMatrix2.getDimensionSize(1)] = y2; }
        }

        ///make statistics operations...
        if (ip_isMeanActive)
        {
            for (size_t i = 0; i < mean->getDimensionSize(0); ++i)
            {
                mean->getBuffer()[i * mean->getDimensionSize(1)] = m_sumMatrix.getBuffer()[i * m_sumMatrix.getDimensionSize(1)] / matrix->getDimensionSize(1);
            }
        }
        if (ip_isVarianceActive)
        {
            for (size_t i = 0; i < variance->getDimensionSize(0); ++i)
            {
                const double y  = m_sumMatrix.getBuffer()[i * m_sumMatrix.getDimensionSize(1)];
                const double y2 = m_sumMatrix2.getBuffer()[i * m_sumMatrix2.getDimensionSize(1)];

                variance->getBuffer()[i * variance->getDimensionSize(1)] = y2 / s - y * y / (s * s);
            }
        }
        if (ip_isRangeActive)
        {
            for (size_t i = 0; i < range->getDimensionSize(0); ++i)
            {
                const double min = m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + 0];
                const double max = m_sortMatrix.getBuffer()[(i + 1) * m_sortMatrix.getDimensionSize(1) - 1];

                range->getBuffer()[i * range->getDimensionSize(1)] = max - min;
            }
        }
        if (ip_isMedianActive)
        {
            for (size_t i = 0; i < median->getDimensionSize(0); ++i)
            {
                median->getBuffer()[i * median->getDimensionSize(1)] =
                        (m_sortMatrix.getDimensionSize(1) % 2)
                            ? m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + m_sortMatrix.getDimensionSize(1) / 2 + 1 - 1]
                            : (m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + m_sortMatrix.getDimensionSize(1) / 2 - 1]
                               + m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + m_sortMatrix.getDimensionSize(1) / 2 + 1 - 1]) / 2;
            }
        }
        if (ip_isIQRActive)
        {
            for (size_t i = 0; i < iqr->getDimensionSize(0); ++i)
            {
                double flow        = 0, up = 0;
                const size_t reste = m_sortMatrix.getDimensionSize(1) % 4;
                const size_t nb    = 4 - reste;
                for (size_t k = 0; k < nb; ++k)
                {
                    flow += m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + m_sortMatrix.getDimensionSize(1) / 4 - (nb - 1) + k - 1];
                }

                flow /= nb;

                for (size_t k = 0; k < nb; ++k)
                {
                    up += m_sortMatrix.getBuffer()[i * m_sortMatrix.getDimensionSize(1) + m_sortMatrix.getDimensionSize(1)
                                                   - m_sortMatrix.getDimensionSize(1) / 4 - 1 + k - 1];
                }

                up /= nb;
                iqr->getBuffer()[i * iqr->getDimensionSize(1)] = up - flow;
            }
        }
        if (ip_isPercentileActive)
        {
            const uint64_t value = m_percentileValue;
            for (size_t i = 0; i < percentile->getDimensionSize(0); ++i)
            {
                percentile->getBuffer()[i * percentile->getDimensionSize(1)] = m_sortMatrix.getBuffer()[
                    i * m_sortMatrix.getDimensionSize(1) + std::max(int(0), int(m_sortMatrix.getDimensionSize(1) * value / 100 - 1))];
            }
        }

        this->activateOutputTrigger(OVP_Algorithm_UnivariateStatistic_OutputTriggerId_ProcessDone, true);
    }

    return true;
}

bool CAlgoUnivariateStatistic::setMatrixDimension(CMatrix* matrix, CMatrix* ref)
{
    //@todo We Allow matrix with 3 or more dimension ?
    if (ref->getDimensionCount() < 2)
    {
        this->getLogManager() << Kernel::LogLevel_Warning << "Input matrix doesn't respect basic criteria (2 Dimensions)\n";
        return false;
    }
    matrix->setDimensionCount(ref->getDimensionCount());  // Also clears the buffer
    matrix->setDimensionSize(0, ref->getDimensionSize(0));
    matrix->setDimensionSize(1, 1);

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
