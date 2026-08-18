///-------------------------------------------------------------------------------------------------
/// 
/// \file Geodesic.hpp
/// \brief All functions to estimate the Geodesic position of two Covariance Matrix.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 26/10/2018.
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
/// \remarks 
/// - List of Metrics inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a> (<a href="https://github.com/alexandrebarachant/pyRiemann/blob/master/LICENSE">License</a>).
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "geometry/Metrics.hpp"
#include <Eigen/Dense>

namespace Geometry {

/// <summary>   Compute the matrix at the position alpha on the geodesic between A and B with the selected \p metric.\n
/// - Allowed Metrics : <c>Riemann</c>, <c>Euclidian</c>, <c>LogEuclidian</c>, <c>Identity</c>
/// </summary>
/// <param name="a">        The First Covariance matrix. </param>
/// <param name="b">        The Second Covariance matrix. </param>
/// <param name="g">        The Geodesic. </param>
/// <param name="metric">   (Optional) The metric (see <see cref="EMetric"/>). </param>
/// <param name="alpha">    (Optional) Position on the Geodesic : \f$ 0\leq \text{alpha} \leq 1\f$. </param>
/// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
bool Geodesic(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, EMetric metric = EMetric::Riemann, double alpha = 0.5);

/// <summary>   Compute the matrix at the position alpha on the Riemannian geodesic between A and B. \n
/// \f[ \gamma_\text{R} = A^{1/2} ~ \left( A^{-1/2} ~ B ~ A^{-1/2} \right)^\alpha ~ A^{1/2} \f]
/// </summary>
/// <param name="a">        The First Covariance matrix. </param>
/// <param name="b">        The Second Covariance matrix. </param>
/// <param name="g">        The Geodesic. </param>
/// <param name="alpha">    (Optional) Position on the Geodesic : \f$ 0\leq \text{alpha} \leq 1\f$. </param>
/// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
bool GeodesicRiemann(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, double alpha = 0.5);

/// <summary>   Compute the matrix at the position alpha on the Euclidean geodesic between A and B.\n
/// \f[ \gamma_\text{E} = \left(1 - \alpha \right) \times A + \alpha \times B \f]
/// </summary>
/// <param name="a">        The First Covariance matrix. </param>
/// <param name="b">        The Second Covariance matrix. </param>
/// <param name="g">        The Geodesic. </param>
/// <param name="alpha">    (Optional) Position on the Geodesic : \f$ 0\leq \text{alpha} \leq 1\f$. </param>
/// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
bool GeodesicEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, double alpha = 0.5);

/// <summary>   Compute the matrix at the position alpha on the Log Euclidean geodesic between A and B. \n
/// \f[ \gamma_\text{LogE} = \exp\left(\left(1 - \alpha \right) \times \log\left(A\right) + \alpha \times \log\left(B\right) \right)\f]
/// </summary>
/// <param name="a">        The First Covariance matrix. </param>
/// <param name="b">        The Second Covariance matrix. </param>
/// <param name="g">        The Geodesic. </param>
/// <param name="alpha">    (Optional) Position on the Geodesic : \f$ 0\leq \text{alpha} \leq 1\f$. </param>
/// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
bool GeodesicLogEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, double alpha = 0.5);

/// <summary>   Compute the matrix at the position alpha on the Identity geodesic. \n
/// \f[ \gamma_\text{I} = I_N \f]
/// </summary>
/// <param name="a">        The First Covariance matrix. </param>
/// <param name="b">        The Second Covariance matrix. </param>
/// <param name="g">        The Geodesic. </param>
/// <param name="alpha">    (Optional) Position on the Geodesic : \f$ 0\leq \text{alpha} \leq 1\f$. </param>
/// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
bool GeodesicIdentity(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, double alpha = 0.5);

}  // namespace Geometry
