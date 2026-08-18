///-------------------------------------------------------------------------------------------------
/// 
/// \file Mean.cpp
/// \brief All functions implementation to estimate the mean of Vector of Covariance Matrix.
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
/// - The Approximate joint diagonalization based on pham's algorithm is not implemented.
/// - The Approximate joint diagonalization based log-Euclidean (ALE) Mean doesn't work => Need to implement <see cref="AJDPham"/> and check if it works next.
/// - The Wasserstein Mean Doesn't work so good (after \f$10^{-3}\f$ precision with the pyriemann library).
/// 
///-------------------------------------------------------------------------------------------------

#include "geometry/Metrics.hpp"
#include "geometry/Basics.hpp"
#include "geometry/Geodesic.hpp"
#include "geometry/Distance.hpp"
#include "geometry/Mean.hpp"
#include <unsupported/Eigen/MatrixFunctions>
#include <iostream>

namespace Geometry {

//static const double EPSILON = 0.000000001;    // 10^{-9}
static const double EPSILON  = 0.0001;          // 10^{-4}
static const size_t ITER_MAX = 50;

//---------------------------------------------------------------------------------------------------
bool Mean(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean, const EMetric metric)
{
    if (covs.empty()) { return false; }         // If no matrix in vector
    if (covs.size() == 1)                       // If just one matrix in vector
    {
        mean = covs[0];
        return true;
    }
    if (!HaveSameSize(covs)) {
        std::cout << "Matrices haven't same size." << std::endl;
        return false;
    }

    // Force Square Matrix for non Euclidian and non Identity metric
    if (!IsSquare(covs[0]) && (metric != EMetric::Euclidian && metric != EMetric::Identity)) {
        std::cout << "Non Square Matrix is invalid with " << toString(metric) << " metric." << std::endl;
        return false;
    }

    switch (metric)                             // Switch method
    {
        case EMetric::Riemann: return MeanRiemann(covs, mean);
        case EMetric::Euclidian: return MeanEuclidian(covs, mean);
        case EMetric::LogEuclidian: return MeanLogEuclidian(covs, mean);
        case EMetric::LogDet: return MeanLogDet(covs, mean);
        case EMetric::Kullback: return MeanKullback(covs, mean);
        case EMetric::ALE: return MeanALE(covs, mean);
        case EMetric::Harmonic: return MeanHarmonic(covs, mean);
        case EMetric::Wasserstein: return MeanWasserstein(covs, mean);
        case EMetric::Identity:
        default: return MeanIdentity(covs, mean);
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool AJDPham(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& ajd, double /*epsilon*/, const int /*maxIter*/)
{
    MeanIdentity(covs, ajd);
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanRiemann(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());                 // Number of Matrix             => K
    const Eigen::Index n = covs[0].rows();                      // Number of Features           => N
    size_t i             = 0;                                   // Index of Covariance Matrix   => i
    double nu            = 1.0,                                 // Coefficient change           => nu
           tau           = std::numeric_limits<double>::max(),  // Coefficient change criterion => tau
           crit          = std::numeric_limits<double>::max();  // Current change               => crit
    if (!MeanEuclidian(covs, mean)) { return false; }           // Initial Mean

    while (i < ITER_MAX && EPSILON < crit && EPSILON < nu)      // Stopping criterion
    {
        i++;                                                    // Iteration Criterion
        const Eigen::MatrixXd sC = mean.sqrt(), isC = sC.inverse(); // Square root & Inverse Square root of Mean    => sC & isC
        Eigen::MatrixXd mJ       = Eigen::MatrixXd::Zero(n, n); // Change                       => J
        for (const auto& cov : covs) { mJ += (isC * cov * isC).log(); } // Sum of log(isC*Ci*isC)
        mJ /= k;                                                // Normalization
        crit = mJ.norm();                                       // Current change criterion
        mean = sC * (nu * mJ).exp() * sC;                       // Update Mean                  => M = sC * exp(nu*J) * sC

        const double h = nu * crit;                             // Update Coefficient change
        if (h < tau) {
            nu *= 0.95;
            tau = h;
        }
        else { nu *= 0.5; }
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanEuclidian(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());         // Number of Matrix     => K
    const Eigen::Index n = covs[0].rows();              // Number of Features   => N
    mean                 = Eigen::MatrixXd::Zero(n, n); // Initial Mean
    for (const auto& cov : covs) { mean += cov; }       // Sum of Ci
    mean /= k;                                          // Normalization
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanLogEuclidian(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());         // Number of Matrix     => K
    const Eigen::Index n = covs[0].rows();              // Number of Features   => N
    mean                 = Eigen::MatrixXd::Zero(n, n); // Initial Mean
    for (const auto& cov : covs) { mean += cov.log(); } // Sum of log(Ci)
    mean = (mean / k).exp();                            // Normalization
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanLogDet(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());             // Number of Matrix             => K
    const Eigen::Index n = covs[0].rows();                  // Number of Features           => N
    size_t i             = 0;                               // Index of Covariance Matrix   => i
    double crit          = std::numeric_limits<double>::max();  // Current change           => crit
    if (!MeanEuclidian(covs, mean)) { return false; }       // Initial Mean

    while (i < ITER_MAX && EPSILON < crit)                  // Stopping criterion
    {
        i++;                                                // Iteration Criterion
        Eigen::MatrixXd mJ = Eigen::MatrixXd::Zero(n, n);   // Change                       => J

        for (const auto& cov : covs) { mJ += (0.5 * (cov + mean)).inverse(); }  // Sum of ((Ci+M)/2)^{-1}
        mJ   = (mJ / k).inverse();                          // Normalization
        crit = (mJ - mean).norm();                          // Current change criterion
        mean = mJ;                                          // Update mean
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanKullback(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    Eigen::MatrixXd m1, m2;
    if (!MeanEuclidian(covs, m1)) { return false; }
    if (!MeanHarmonic(covs, m2)) { return false; }
    if (!GeodesicRiemann(m1, m2, mean, 0.5)) { return false; }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanWasserstein(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());             // Number of Matrix             => K
    const Eigen::Index n = covs[0].rows();                  // Number of Features           => N
    size_t i             = 0;                               // Index of Covariance Matrix   => i
    double crit          = std::numeric_limits<double>::max();  // Current change           => crit

    if (!MeanEuclidian(covs, mean)) { return false; }       // Initial Mean
    Eigen::MatrixXd sC = mean.sqrt();                       // Square root of Mean          => sC

    while (i < ITER_MAX && EPSILON < crit)                  // Stopping criterion
    {
        i++;                                                // Iteration Criterion
        Eigen::MatrixXd mJ = Eigen::MatrixXd::Zero(n, n);   // Change                       => J

        for (const auto& cov : covs) { mJ += (sC * cov * sC).sqrt(); }  // Sum of sqrt(sC*Ci*sC)
        mJ /= k;                                            // Normalization

        const Eigen::MatrixXd sJ = mJ.sqrt();               // Square root of change        => sJ
        crit                     = (sJ - sC).norm();        // Current change criterion
        sC                       = sJ;                      // Update sC
    }
    mean = sC * sC;                                         // Un-square root 
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanALE(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());             // Number of Matrix             => K
    const Eigen::Index n = covs[0].rows();                  // Number of Features           => N
    size_t i             = 0;                               // Index of Covariance Matrix   => i
    double crit          = std::numeric_limits<double>::max();  // Change criterion         => crit
    if (!AJDPham(covs, mean)) { return false; }             // Initial Mean
    Eigen::MatrixXd mJ;                                     // Change

    while (i < ITER_MAX && EPSILON < crit)                  // Stopping criterion
    {
        i++;                                                // Iteration Criterion
        mJ = Eigen::MatrixXd::Zero(n, n);                   // Change                       => J

        for (const auto& cov : covs) { mJ += (mean.transpose() * cov * mean).log(); }   // Sum of log(C^T*Ci*C)
        mJ /= k;                                            // Normalization

        Eigen::MatrixXd update = mJ.exp().diagonal().asDiagonal();  // Update Form          => U
        mean                   = mean * update.sqrt().inverse();    // Update Mean M = M * U^{-1/2}

        crit = DistanceRiemann(Eigen::MatrixXd::Identity(n, n), update);
    }

    mJ = Eigen::MatrixXd::Zero(n, n);                       // Last Change                  => J
    for (const auto& cov : covs) { mJ += (mean.transpose() * cov * mean).log(); }   // Sum of log(C^T*Ci*C)
    mJ /= k;                                                // Normalization

    Eigen::MatrixXd mA = mean.inverse();
    mean               = mA.transpose() * mJ.exp() * mA;
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanHarmonic(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    const double k       = double(covs.size());             // Number of Matrix             => K
    const Eigen::Index n = covs[0].rows();                  // Number of Features           => N
    mean                 = Eigen::MatrixXd::Zero(n, n);     // Initial Mean
    for (const auto& cov : covs) { mean += cov.inverse(); } // Sum of Inverse
    mean = (mean / k).inverse();                            // Normalization and inverse
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MeanIdentity(const std::vector<Eigen::MatrixXd>& covs, Eigen::MatrixXd& mean)
{
    mean = Eigen::MatrixXd::Identity(covs[0].rows(), covs[0].cols());
    return true;
}
//---------------------------------------------------------------------------------------------------

}  // namespace Geometry
