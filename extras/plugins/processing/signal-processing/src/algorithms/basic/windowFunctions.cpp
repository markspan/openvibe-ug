///-------------------------------------------------------------------------------------------------
///
/// \file windowFunctions.cpp
/// \brief Implementation of Windowing functions
/// \author Alison Cellard
/// \version 1.0
/// \date 13/11/2013
///
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
///-------------------------------------------------------------------------------------------------

#include "windowFunctions.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
namespace WindowFunctions {

bool bartlett(Eigen::VectorXd& window, const Eigen::Index size)
{
    for (Eigen::Index i = 0; i < size; ++i) {
        if (i <= (size - 1) / 2) { window(i) = 2.0 * double(i) / double(size - 1); }
        else if (i < size) { window(i) = 2.0 * double((size - 1) - i) / double(size - 1); }
    }
    return true;
}

bool hamming(Eigen::VectorXd& window, const Eigen::Index size)
{
    for (Eigen::Index i = 0; i < size; ++i) {
        window(i) = 0.54 - 0.46 * cos(2.0 * M_PI * double(i) / double(size - 1));
    }
    return true;
}

bool hann(Eigen::VectorXd& window, const Eigen::Index size)
{
    for (Eigen::Index i = 0; i < size; ++i) {
        window(i) = 0.5 - 0.5 * cos(2.0 * M_PI * double(i) / double(size - 1));
    }
    return true;
}


bool parzen(Eigen::VectorXd& window, const Eigen::Index size)
{
    for (Eigen::Index i = 0; i < size; ++i) {
        window(i) = 1.0 - pow((double(i) - (double(size) - 1.0) / 2.0) / ((double(size) + 1.0) / 2.0), 2);
    }
    return true;
}


bool welch(Eigen::VectorXd& window, const Eigen::Index size)
{
    for (Eigen::Index i = 0; i < size; ++i) {
        window(i) = 1.0 - fabs((double(i) - (double(size) - 1.0) / 2.0) / ((double(size) + 1.0) / 2.0));
    }
    return true;
}

}  //namespace WindowFunctions
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
