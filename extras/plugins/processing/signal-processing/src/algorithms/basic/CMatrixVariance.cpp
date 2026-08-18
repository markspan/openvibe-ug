///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixVariance.cpp
/// \brief Definitions of Class used to compute matrix variance.
/// \author Dieter Devlaminck & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 11/11/2021.
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

#include "CMatrixVariance.hpp"

#include <cmath>

// the boost version used at the moment of writing this caused 4800 by internal call to "int _isnan" in a bool-returning function.
#if defined(WIN32)
#pragma warning (disable : 4800)
#endif

#include <boost/math/distributions/students_t.hpp>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void CMatrixVariance::initialize(const EEpochAverageMethod method, const size_t matrixCount, const double significanceLevel)
{
    m_averagingMethod   = method;
    m_matrixCount       = matrixCount;
    m_significanceLevel = significanceLevel;
}

void CMatrixVariance::uninitialize() { m_history.clear(); }

// ________________________________________________________________________________________________________________
//

void CMatrixVariance::processHeader(const size_t size)
{
    m_mean.resize(Eigen::Index(size));
    m_mean.setZero();
    m_m.resize(Eigen::Index(size));
    m_m.setZero();
    m_variance.resize(Eigen::Index(size));
    m_variance.setZero();
    m_counter = 0;
    m_history.clear();
}


bool CMatrixVariance::process(CMatrix* input, CMatrix* averaged, CMatrix* variance, CMatrix* confidence)
{
    bool shouldPerformAverage;

    const Eigen::Index size = Eigen::Index(input->getBufferElementCount());
    const size_t buffersize = input->getBufferElementCount() * sizeof(double);
    if (m_averagingMethod == EEpochAverageMethod::Moving) {
        while (m_history.size() >= m_matrixCount) { m_history.pop_front(); }
        m_history.push_back(Eigen::Map<Eigen::RowVectorXd>(input->getBuffer(), size));
        shouldPerformAverage = (m_history.size() == m_matrixCount);
    }
    else if (m_averagingMethod == EEpochAverageMethod::MovingImmediate) {
        while (m_history.size() >= m_matrixCount) { m_history.pop_front(); }
        m_history.push_back(Eigen::Map<Eigen::RowVectorXd>(input->getBuffer(), size));
        shouldPerformAverage = (!m_history.empty());
    }
    else if (m_averagingMethod == EEpochAverageMethod::Block) {
        if (m_history.size() >= m_matrixCount) { m_history.clear(); }
        m_history.push_back(Eigen::Map<Eigen::RowVectorXd>(input->getBuffer(), size));
        shouldPerformAverage = (m_history.size() == m_matrixCount);
    }
    else if (m_averagingMethod == EEpochAverageMethod::Cumulative) {
        if (!m_history.empty()) { m_history.pop_front(); }
        m_history.push_back(Eigen::Map<Eigen::RowVectorXd>(input->getBuffer(), size));
        shouldPerformAverage = (!m_history.empty());
    }
    else { return false; }

    if (shouldPerformAverage) {
        if (!m_history.empty()) {
            boost::math::students_t_distribution<double> distrib(2);
            if (m_averagingMethod == EEpochAverageMethod::Cumulative) {
                //incremental estimation of mean and variance
                for (auto& history : m_history) {
                    m_counter++;
                    Eigen::RowVectorXd delta = history - m_mean;
                    m_mean += delta / double(m_counter);
                    m_m += delta.cwiseProduct((history - m_mean)); // not same as delta.cwiseProduct(delta) mean is updated
                    if (m_counter > 1) { m_variance = m_m / double(m_counter - 1); }
                }
                distrib = boost::math::students_t_distribution<double>(double(m_counter <= 1 ? 1 : (m_counter - 1)));
                std::memcpy(averaged->getBuffer(), m_mean.data(), buffersize);
                std::memcpy(variance->getBuffer(), m_variance.data(), buffersize);
            }
            else {
                distrib = boost::math::students_t_distribution<double>(double(m_matrixCount) - 1);

                averaged->resetBuffer();
                variance->resetBuffer();

                const Eigen::Index count = Eigen::Index(averaged->getBufferElementCount());
                const double scale = 1.0 / double(m_history.size());

                for (auto& history : m_history) {
                    //batch computation of mean
                    double* averageBuffer = averaged->getBuffer();
                    for (Eigen::Index i = 0; i < count; ++i) { averageBuffer[i] += history[i] * scale; }
                    //batch computation of variance
                    averageBuffer          = averaged->getBuffer();
                    double* varianceBuffer = variance->getBuffer();
                    //const double quotient  = m_history.size() <= 1 ? 1 : 1.0 / double(m_history.size() - 1);
                    const double quotient = 1.0 / double(m_history.size() - 1); // Allow 0 division
                    for (Eigen::Index i = 0; i < count; ++i) { varianceBuffer[i] += (history[i] - averageBuffer[i]) * (history[i] - averageBuffer[i]) * quotient; }
                }
                m_variance = Eigen::Map<Eigen::RowVectorXd>(averaged->getBuffer(), count);
            }

            //computing confidence bounds
            const double q           = double(boost::math::quantile(complement(distrib, m_significanceLevel / 2.0)));
            const double coef        = q / sqrt(double((m_averagingMethod == EEpochAverageMethod::Cumulative) ? m_counter : m_matrixCount));
            Eigen::RowVectorXd bound = coef * m_variance.cwiseSqrt();
            std::memcpy(confidence->getBuffer(), bound.data(), buffersize);
        }
        return true;
    }
    return false;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
