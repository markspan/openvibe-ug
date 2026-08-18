///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmMatrixAverage.cpp
/// \brief Classes implementation for the Algorithm Matrix average.
/// \author Yann Renard (Inria/IRISA).
/// \version 1.1.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CAlgorithmMatrixAverage.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

// ________________________________________________________________________________________________________________
//

bool CAlgorithmMatrixAverage::initialize()
{
    ip_averagingMethod.initialize(getInputParameter(MatrixAverage_InputParameterId_AveragingMethod));
    ip_matrixCount.initialize(getInputParameter(MatrixAverage_InputParameterId_MatrixCount));
    ip_matrix.initialize(getInputParameter(MatrixAverage_InputParameterId_Matrix));
    op_averagedMatrix.initialize(getOutputParameter(MatrixAverage_OutputParameterId_AveragedMatrix));

    m_nAverageSamples = 0;

    return true;
}

bool CAlgorithmMatrixAverage::uninitialize()
{
    for (auto it = m_history.begin(); it != m_history.end(); ++it) { delete *it; }
    m_history.clear();

    op_averagedMatrix.uninitialize();
    ip_matrix.uninitialize();
    ip_matrixCount.uninitialize();
    ip_averagingMethod.uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CAlgorithmMatrixAverage::process()
{
    CMatrix* iMatrix = ip_matrix;
    CMatrix* oMatrix = op_averagedMatrix;

    bool shouldPerformAverage = false;

    if (this->isInputTriggerActive(MatrixAverage_InputTriggerId_Reset)) {
        for (auto it = m_history.begin(); it != m_history.end(); ++it) { delete*it; }

        m_history.clear();

        oMatrix->copyDescription(*iMatrix);
    }

    if (this->isInputTriggerActive(MatrixAverage_InputTriggerId_FeedMatrix)) {
        if (ip_averagingMethod == uint64_t(EEpochAverageMethod::Moving)) {
            CMatrix* swapMatrix;

            if (m_history.size() >= ip_matrixCount) {
                swapMatrix = m_history.front();
                m_history.pop_front();
            }
            else {
                swapMatrix = new CMatrix();
                swapMatrix->copyDescription(*iMatrix);
            }

            swapMatrix->copyContent(*iMatrix);

            m_history.push_back(swapMatrix);
            shouldPerformAverage = (m_history.size() == ip_matrixCount);
        }
        else if (ip_averagingMethod == uint64_t(EEpochAverageMethod::MovingImmediate)) {
            CMatrix* swapMatrix;

            if (m_history.size() >= ip_matrixCount) {
                swapMatrix = m_history.front();
                m_history.pop_front();
            }
            else {
                swapMatrix = new CMatrix();
                swapMatrix->copyDescription(*iMatrix);
            }

            swapMatrix->copyContent(*iMatrix);

            m_history.push_back(swapMatrix);
            shouldPerformAverage = (!m_history.empty());
        }
        else if (ip_averagingMethod == uint64_t(EEpochAverageMethod::Block)) {
            CMatrix* swapMatrix = new CMatrix();

            if (m_history.size() >= ip_matrixCount) {
                for (auto it = m_history.begin(); it != m_history.end(); ++it) { delete *it; }
                m_history.clear();
            }

            swapMatrix->copy(*iMatrix);

            m_history.push_back(swapMatrix);
            shouldPerformAverage = (m_history.size() == ip_matrixCount);
        }
        else if (ip_averagingMethod == uint64_t(EEpochAverageMethod::Cumulative)) {
            m_history.push_back(iMatrix);
            shouldPerformAverage = true;
        }
        else { shouldPerformAverage = false; }
    }

    if (shouldPerformAverage) {
        oMatrix->resetBuffer();

        if (ip_averagingMethod == uint64_t(EEpochAverageMethod::Cumulative)) {
            CMatrix* matrix = m_history.at(0);

            m_nAverageSamples++;

            if (m_nAverageSamples == 1) // If it's the first matrix, the average is the first matrix
            {
                double* buffer    = matrix->getBuffer();
                const size_t size = matrix->getBufferElementCount();

                m_averageMatrices.clear();
                m_averageMatrices.insert(m_averageMatrices.begin(), buffer, buffer + size);
            }
            else {
                if (matrix->getBufferElementCount() != m_averageMatrices.size()) { return false; }

                const double n = double(m_nAverageSamples);

                double* iBuffer = matrix->getBuffer();

                for (double& value : m_averageMatrices) {
                    // calculate cumulative mean as
                    // mean{k} = mean{k-1} + (new_value - mean{k-1}) / k
                    // which is a recurrence equivalent to mean{k] = mean{k-1} * (k-1)/k + new_value/k
                    // but more numerically stable
                    value += (*iBuffer - value) / n;
                    iBuffer++;
                }
            }

            double* oBuffer = oMatrix->getBuffer();

            for (const double& value : m_averageMatrices) {
                *oBuffer = double(value);
                oBuffer++;
            }

            m_history.clear();
        }
        else {
            const size_t n     = oMatrix->getBufferElementCount();
            const double scale = 1. / m_history.size();

            for (CMatrix* matrix : m_history) {
                double* oBuffer = oMatrix->getBuffer();
                double* iBuffer = matrix->getBuffer();

                for (size_t i = 0; i < n; ++i) {
                    *oBuffer += *iBuffer * scale;
                    oBuffer++;
                    iBuffer++;
                }
            }
        }

        this->activateOutputTrigger(MatrixAverage_OutputTriggerId_AveragePerformed, true);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
