///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixVariance.hpp
/// \brief Class used to compute matrix variance.
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

#pragma once

#include "defines.hpp"

#include <Eigen/Dense>
#include <openvibe/ov_all.h>
#include <deque>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CMatrixVariance
{
public:
    CMatrixVariance()  = default;
    ~CMatrixVariance() = default;

    void initialize(EEpochAverageMethod method, size_t matrixCount, double significanceLevel);
    void uninitialize();

    void processHeader(const size_t size);
    bool process(CMatrix* input, CMatrix* averaged, CMatrix* variance, CMatrix* confidence);


protected:
    EEpochAverageMethod m_averagingMethod = EEpochAverageMethod::Moving;
    size_t m_matrixCount                  = 4;
    double m_significanceLevel            = 0.01;

    std::deque<Eigen::RowVectorXd> m_history;

    Eigen::RowVectorXd m_mean;
    Eigen::RowVectorXd m_m;
    Eigen::RowVectorXd m_variance;
    size_t m_counter = 0;
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
