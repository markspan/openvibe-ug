///-------------------------------------------------------------------------------------------------
/// 
/// \file Distance.cpp
/// \brief All functions implementation to estimate the Distance between two Covariance Matrix.
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

#include "geometry/Distance.hpp"
#include "geometry/Basics.hpp"
#include <unsupported/Eigen/MatrixFunctions>

namespace Geometry {

//---------------------------------------------------------------------------------------------------
double Distance(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b, const EMetric metric)
{
    if (!HaveSameSize(a, b)) { return 0; }
    switch (metric) {
        case EMetric::Riemann: return DistanceRiemann(a, b);
        case EMetric::Euclidian: return DistanceEuclidian(a, b);
        case EMetric::LogEuclidian: return DistanceLogEuclidian(a, b);
        case EMetric::LogDet: return DistanceLogDet(a, b);
        case EMetric::Kullback: return DistanceKullbackSym(a, b);
        case EMetric::Wasserstein: return DistanceWasserstein(a, b);
        case EMetric::Identity:
        case EMetric::ALE:
        case EMetric::Harmonic:
        default: return 1.0;
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceRiemann(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b)
{
    const Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> es(a, b);
    const Eigen::ArrayXd result = es.eigenvalues();
    return sqrt(result.log().square().sum());
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b) { return (b - a).norm(); }
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceLogEuclidian(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b) { return DistanceEuclidian(a.log(), b.log()); }
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceLogDet(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b)
{
    return sqrt(log((0.5 * (a + b)).determinant()) - 0.5 * log(a.determinant() * b.determinant()));
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceKullback(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b)
{
    return 0.5 * ((b.inverse() * a).trace() - double(a.rows()) + log(b.determinant() / a.determinant()));
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceKullbackSym(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b) { return DistanceKullback(a, b) + DistanceKullback(b, a); }
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double DistanceWasserstein(const Eigen::MatrixXd& a, const Eigen::MatrixXd& b)
{
    const Eigen::MatrixXd sB = b.sqrt();
    return sqrt((a + b - 2 * (sB * a * sB).sqrt()).trace());
}
//---------------------------------------------------------------------------------------------------

}  // namespace Geometry
