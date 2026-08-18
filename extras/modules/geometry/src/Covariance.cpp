///-------------------------------------------------------------------------------------------------
/// 
/// \file Covariance.cpp
/// \brief All functions implementation to estimate the Covariance Matrix.
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
/// - List of Estimator inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a> (<a href="https://github.com/alexandrebarachant/pyRiemann/blob/master/LICENSE">License</a>).
/// - <a href="http://scikit-learn.org/stable/modules/generated/sklearn.covariance.LedoitWolf.html">Ledoit and Wolf Estimator</a> inspired by <a href="http://scikit-learn.org">sklearn</a> (<a href="https://github.com/scikit-learn/scikit-learn/blob/master/COPYING">License</a>).
/// - <a href="http://scikit-learn.org/stable/modules/generated/sklearn.covariance.OAS.html">Oracle Approximating Shrinkage (OAS) Estimator</a> Inspired by <a href="http://scikit-learn.org">sklearn</a> (<a href="https://github.com/scikit-learn/scikit-learn/blob/master/COPYING">License</a>).
/// - <b>Minimum Covariance Determinant (MCD) Estimator isn't implemented. </b>
/// 
///-------------------------------------------------------------------------------------------------

#include "geometry/Covariance.hpp"
#include <algorithm>        // std::min/max
#include <cmath>            // std::fabs for unix
#include <cfloat>           // DBL_EPSILON for unix

namespace Geometry {

//***********************************************************
//******************** COVARIANCES BASES ********************
//***********************************************************
//---------------------------------------------------------------------------------------------------
double Variance(const Eigen::RowVectorXd& x)
{
    const Eigen::Index S = x.cols();                        // Number of Samples                => S
    if (S == 0) { return 0; }                               // If false input

    const double mu = x.mean();
    return x.cwiseProduct(x).sum() / double(S) - mu * mu;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
double Covariance(const Eigen::RowVectorXd& x, const Eigen::RowVectorXd& y)
{
    const Eigen::Index xS = x.cols(), yS = y.cols();        // Number of Samples                => S
    if (xS == 0 || xS != yS) { return 0; }                  // If false input
    return (x.cwiseProduct(y).sum() - x.sum() * y.sum() / double(xS)) / double(xS);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool ShrunkCovariance(Eigen::MatrixXd& cov, const double shrinkage)
{
    if (!InRange(shrinkage, 0, 1)) { return false; }        // Verification
    const Eigen::Index n = cov.rows();                      // Number of Features               => N

    const double coef = shrinkage * cov.trace() / double(n);    // Diagonal Coefficient
    cov               = (1 - shrinkage) * cov;              // Shrinkage
    for (Eigen::Index i = 0; i < n; ++i) { cov(i, i) += coef; } // Add Diagonal Coefficient

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool ShrunkCovariance(const Eigen::MatrixXd& in, Eigen::MatrixXd& out, const double shrinkage)
{
    out = in;
    return ShrunkCovariance(out, shrinkage);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrix(const Eigen::MatrixXd& in, Eigen::MatrixXd& out, const EEstimator estimator, const EStandardization standard)
{
    if (!IsNotEmpty(in)) { return false; }                  // Verification
    Eigen::MatrixXd sample;
    MatrixStandardization(in, sample, standard);            // Standardization
    switch (estimator)                                      // Switch Method
    {
        case EEstimator::COV: return CovarianceMatrixCOV(sample, out);
        case EEstimator::SCM: return CovarianceMatrixSCM(sample, out);
        case EEstimator::LWF: return CovarianceMatrixLWF(sample, out);
        case EEstimator::OAS: return CovarianceMatrixOAS(sample, out);
        case EEstimator::MCD: return CovarianceMatrixMCD(sample, out);
        case EEstimator::COR: return CovarianceMatrixCOR(sample, out);
        case EEstimator::IDE: return CovarianceMatrixIDE(sample, out);
        default: return CovarianceMatrixIDE(sample, out);
    }
}
//---------------------------------------------------------------------------------------------------

//***********************************************************
//***********************************************************
//***********************************************************

//***********************************************************
//******************** COVARIANCES TYPES ********************
//***********************************************************
//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixCOV(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov)
{
    const Eigen::Index n = samples.rows();                  // Number of Features               => N

    cov.resize(n, n);                                       // Init size of matrix
    for (Eigen::Index i = 0; i < n; ++i) {
        const Eigen::RowVectorXd ri = samples.row(i);
        cov(i, i)                   = Variance(ri);         // Diagonal Value

        for (Eigen::Index j = i + 1; j < n; ++j) {
            const Eigen::RowVectorXd rj = samples.row(j);
            cov(i, j)                   = cov(j, i) = Covariance(ri, rj);   // Symetric covariance
        }
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixSCM(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov)
{
    cov = samples * samples.transpose();                    // X*X^T
    cov /= cov.trace();                                     // X*X^T / trace(X*X^T)
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixLWF(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov)
{
    const Eigen::Index n = samples.rows(), S = samples.cols();  // Number of Features & Samples     => N & S

    CovarianceMatrixCOV(samples, cov);                          // Initial Covariance Matrix        => Cov
    const double mu        = cov.trace() / double(n);
    Eigen::MatrixXd mDelta = cov;                               // mDelta = cov - mu * I_n
    for (Eigen::Index i = 0; i < n; ++i) { mDelta(i, i) -= mu; }
    const Eigen::MatrixXd x2   = samples.cwiseProduct(samples), // Squared each sample              => X^2
                          cov2 = cov.cwiseProduct(cov);         // Squared each element of Cov      => Cov^2

    const double delta     = mDelta.cwiseProduct(mDelta).sum() / double(n),
                 beta      = 1. / double(n * S) * (x2 * x2.transpose() / double(S) - cov2).sum(),
                 shrinkage = std::min(beta, delta) / delta;     // Assure shrinkage <= 1

    return ShrunkCovariance(cov, shrinkage);                    // Shrinkage of the matrix
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixOAS(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov)
{
    const Eigen::Index n = samples.rows(), S = samples.cols();  // Number of Features & Samples     => N & S
    CovarianceMatrixCOV(samples, cov);                          // Initial Covariance Matrix        => Cov

    // Compute Shrinkage : Formula from Chen et al.'s
    const double mu        = cov.trace() / double(n),
                 mu2       = mu * mu,
                 alpha     = cov.cwiseProduct(cov).mean(),
                 num       = alpha + mu2,
                 den       = double(S + 1) * (alpha - mu2 / double(n)),
                 shrinkage = (std::fabs(den) <= DBL_EPSILON) ? 1.0 : std::min(num / den, 1.0);

    return ShrunkCovariance(cov, shrinkage);                    // Shrinkage of the matrix
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixMCD(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov) { return CovarianceMatrixIDE(samples, cov); }
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixCOR(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov)
{
    const Eigen::Index n = samples.rows();                  // Number of Features               => N
    CovarianceMatrixCOV(samples, cov);                      // Initial Covariance Matrix        => Cov
    const Eigen::MatrixXd d = cov.diagonal().cwiseSqrt();   // Squared root of diagonal

    for (Eigen::Index i = 0; i < n; ++i) { for (Eigen::Index j = 0; j < n; ++j) { cov(i, j) /= d(i) * d(j); } }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CovarianceMatrixIDE(const Eigen::MatrixXd& samples, Eigen::MatrixXd& cov)
{
    cov = Eigen::MatrixXd::Identity(samples.rows(), samples.rows());
    return true;
}
//---------------------------------------------------------------------------------------------------
//***********************************************************
//***********************************************************
//***********************************************************

}  // namespace Geometry
