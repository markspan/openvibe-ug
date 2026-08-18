///-------------------------------------------------------------------------------------------------
/// 
/// \file Distance.hpp
/// \brief All functions to estimate the Distance between two Covariance Matrix.
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

/// <summary>   Compute the distance between two matrix with the selected \p metric. </summary>
/// <param name="a">        The First Covariance matrix. </param>
/// <param name="b">        The Second Covariance matrix. </param>
/// <param name="metric">   (Optional) The metric (see <see cref="EMetric"/>). </param>
/// <returns>   The Distance between A and B. </returns>
double Distance(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, EMetric metric = EMetric::Riemann);

/// <summary>   Compute the Riemannian Distance between two covariance matrices A and B.\n
/// \f[ d_{\text{R}}(A,B) = \sqrt{\left( \sum_i \log\left(\lambda_i\right)^2 \right)} \f]
/// with : \f$\lambda_i\f$ the joint eigenvalues of \f$A\f$ and \f$B\f$.
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Riemannian Distance between A and B. </returns>
double DistanceRiemann(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/// <summary>   Compute the Euclidian Distance between two covariance matrices A and B.\n
/// \f[ d_{\text{E}}(A,B) = \left\lVert B - A \right\rVert\f]
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Eclidean Distance between A and B. </returns>
double DistanceEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/// <summary>   Compute the Log Euclidian Distance between two covariance matrices A and B.\n
/// \f[ d_{\text{lE}}(A,B) = \left\lVert \log\left(B\right) - \log\left(A\right) \right\rVert\f]
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Log Eclidean Distance between A and B. </returns>
double DistanceLogEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/// <summary>   Compute the Log-det Distance between two covariance matrices A and B.\n
/// \f[ d_{\text{lD}}(A,B) = \sqrt{\log\left(\left\lvert\frac{A + B}{2}\right\rvert\right) - 0.5 \times \log\left( \left\lvert A \right\rvert \times \left\lvert B \right\rvert \right)} \f]
/// with : \f$\left\lvert A \right\rvert\f$ the determinant of \f$A\f$.
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Log-det Distance between A and B. </returns>
double DistanceLogDet(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/// <summary>   Compute the Kullback Leibler Divergence between two covariance matrices A and B.\n
/// \f[ d_{\text{K}}(A,B) = 0.5 \times \left( \operatorname{trace}\left(B^{-1} ~ A\right) - N + \log\left( \frac{\left\lvert B \right\rvert }{\left\lvert A \right\rvert} \right) \right) \f]
/// with : \f$\left\lvert A \right\rvert\f$ the determinant of \f$A\f$.
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Kullback Leibler Divergence Distance between A and B. </returns>
double DistanceKullback(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/// <summary>   Compute the Symetric Kullback Leibler Divergence between two covariance matrices A and B.\n
/// \f[ d_{\text{sK}}(A,B) = d_\text{K}(A, B) + d_\text{K}(B, A) \f]
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Symetric Kullback Leibler Divergence Distance between A and B. </returns>
double DistanceKullbackSym(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

/// <summary>   Compute the Wasserstein Distance between two covariance matrices A and B.\n
/// \f[ d_{\text{W}}(A,B) = \sqrt{ \operatorname{trace}\left(A + B - 2 \times \left(A^{1/2} ~ B ~ A^{1/2}\right)^{1/2}\right) } \f]
/// </summary>
/// <param name="a">    The First Covariance matrix. </param>
/// <param name="b">    The Second Covariance matrix. </param>
/// <returns>   The Wasserstein Distance between A and B. </returns>
double DistanceWasserstein(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b);

}  // namespace Geometry
