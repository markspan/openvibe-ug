///-------------------------------------------------------------------------------------------------
/// 
/// \file Geodesic.cpp
/// \brief All functions implementation to estimate the Geodesic position of two Covariance Matrix.
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

#include "geometry/Geodesic.hpp"
#include "geometry/Basics.hpp"
#include <unsupported/Eigen/MatrixFunctions>

namespace Geometry {

//---------------------------------------------------------------------------------------------------
bool Geodesic(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, const EMetric metric, const double alpha)
{
    if (!HaveSameSize(a, b)) { return false; }                      // Verification same size
    if (!IsSquare(a)) { return false; }                             // Verification square matrix
    if (!InRange(alpha, 0, 1)) { return false; }                    // Verification alpha in [0;1]
    switch (metric)                                                 // Switch metric
    {
        case EMetric::Riemann: return GeodesicRiemann(a, b, g, alpha);
        case EMetric::Euclidian: return GeodesicEuclidian(a, b, g, alpha);
        case EMetric::LogEuclidian: return GeodesicLogEuclidian(a, b, g, alpha);
        case EMetric::Identity:
        case EMetric::LogDet:
        case EMetric::Kullback:
        case EMetric::ALE:
        case EMetric::Harmonic:
        case EMetric::Wasserstein:
        default: return GeodesicIdentity(a, b, g, alpha);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool GeodesicRiemann(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, const double alpha)
{
    const Eigen::MatrixXd sA = a.sqrt(), isA = sA.inverse();
    g                        = sA * (isA * b * isA).pow(alpha) * sA;
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool GeodesicEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, const double alpha)
{
    g = (1 - alpha) * a + alpha * b;
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool GeodesicLogEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, Eigen::MatrixXd& g, const double alpha)
{
    g = ((1 - alpha) * a.log() + alpha * b.log()).exp();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool GeodesicIdentity(const Eigen::MatrixXd& a, const Eigen::MatrixXd& /*b*/, Eigen::MatrixXd& g, const double /*alpha*/)
{
    g = Eigen::MatrixXd::Identity(a.rows(), a.rows());
    return true;
}
//---------------------------------------------------------------------------------------------------

}  // namespace Geometry
