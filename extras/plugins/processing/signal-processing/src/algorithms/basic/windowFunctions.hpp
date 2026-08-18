///-------------------------------------------------------------------------------------------------
///
/// \file windowFunctions.hpp
/// \brief Windowing functions and helpers for Connectivity Measure
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

#pragma once

#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
namespace WindowFunctions {

///
/// \brief Generate Bartlett window
/// \param window The buffer in which the window is stored
/// \param size The size of the window
/// \return True
bool bartlett(Eigen::VectorXd& window, const Eigen::Index size);

///
/// \brief Generate Hamming window
/// \param window The buffer in which the window is stored
/// \param size The size of the window
/// \return True
bool hamming(Eigen::VectorXd& window, const Eigen::Index size);

///
/// \brief Generate Hann window
/// \param window The buffer in which the window is stored
/// \param size The size of the window
/// \return True
bool hann(Eigen::VectorXd& window, const Eigen::Index size);

///
/// \brief Generate Parzen window
/// \param window The buffer in which the window is stored
/// \param size The size of the window
/// \return True
bool parzen(Eigen::VectorXd& window, const Eigen::Index size);

///
/// \brief Generate Welch window
/// \param window The buffer in which the window is stored
/// \param size The size of the window
/// \return True
bool welch(Eigen::VectorXd& window, const Eigen::Index size);

}  // namespace WindowFunctions
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
