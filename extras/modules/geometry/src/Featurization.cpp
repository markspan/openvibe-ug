///-------------------------------------------------------------------------------------------------
/// 
/// \file Featurization.cpp
/// \brief All functions implementation to transform Covariance matrix to feature vector.
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
/// 
///-------------------------------------------------------------------------------------------------

#include "geometry/Featurization.hpp"
#include <unsupported/Eigen/MatrixFunctions>
#include "geometry/Basics.hpp"

namespace Geometry {

#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887242097
#endif

//---------------------------------------------------------------------------------------------------
bool Featurization(const Eigen::MatrixXd& in, Eigen::RowVectorXd& out, const bool tangent, const Eigen::MatrixXd& ref)
{
    if (tangent) { return TangentSpace(in, out, ref); }
    return SqueezeUpperTriangle(in, out, true);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool UnFeaturization(const Eigen::RowVectorXd& in, Eigen::MatrixXd& out, const bool tangent, const Eigen::MatrixXd& ref)
{
    if (tangent) { return UnTangentSpace(in, out, ref); }
    return UnSqueezeUpperTriangle(in, out, true);
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool SqueezeUpperTriangle(const Eigen::MatrixXd& in, Eigen::RowVectorXd& out, const bool rowMajor)
{
    if (!IsSquare(in)) { return false; }                        // Verification
    const Eigen::Index n = in.rows();                           // Number of Features           => N
    out.resize(n * (n + 1) / 2);                                // Resize

    Eigen::Index idx = 0;                                       // Row Index                    => idx
    // Row Major or Diagonal Method
    if (rowMajor) { for (Eigen::Index i = 0; i < n; ++i) { for (Eigen::Index j = i; j < n; ++j) { out[idx++] = in(i, j); } } }
    else { for (Eigen::Index i = 0; i < n; ++i) { for (Eigen::Index j = i; j < n; ++j) { out[idx++] = in(j, j - i); } } }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool UnSqueezeUpperTriangle(const Eigen::RowVectorXd& in, Eigen::MatrixXd& out, const bool rowMajor)
{
    const Eigen::Index nR = in.size(),                          // Size of Row                  => Nr
                       n  = Eigen::Index((sqrt(1 + 8 * nR) - 1) / 2);   // Number of Features   => N
    if (n == 0) { return false; }                               // Verification
    out.setZero(n, n);                                          // Init

    Eigen::Index idx = 0;                                       // Row Index                    => idx
    // Row Major or Diagonal Method
    if (rowMajor) { for (Eigen::Index i = 0; i < n; ++i) { for (Eigen::Index j = i; j < n; ++j) { out(j, i) = out(i, j) = in[idx++]; } } }
    else { for (Eigen::Index i = 0; i < n; ++i) { for (Eigen::Index j = i; j < n; ++j) { out(j - i, j) = out(j, j - i) = in[idx++]; } } }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool TangentSpace(const Eigen::MatrixXd& in, Eigen::RowVectorXd& out, const Eigen::MatrixXd& ref)
{
    if (!IsSquare(in)) { return false; }                        // Verification
    const Eigen::Index n = in.rows();                           // Number of Features           => N

    const Eigen::MatrixXd sC      = (ref.size() == 0) ? Eigen::MatrixXd::Identity(n, n) : Eigen::MatrixXd(ref.sqrt()),
                          isC     = sC.inverse(),               // Inverse Square root of ref   => isC
                          mJ      = (isC * in * isC).log(),     // Transformation Matrix        => mJ
                          mCoeffs = M_SQRT2 * Eigen::MatrixXd(Eigen::MatrixXd::Ones(n, n).triangularView<Eigen::StrictlyUpper>())
                                    + Eigen::MatrixXd::Identity(n, n);

    Eigen::RowVectorXd vJ, vCoeffs;
    if (!SqueezeUpperTriangle(mJ, vJ, true)) { return false; }  // Get upper triangle of J      => vJ
    if (!SqueezeUpperTriangle(mCoeffs, vCoeffs, true)) { return false; }    // ... of Coefs     => vCoeffs
    out = vCoeffs.cwiseProduct(vJ);                             // element-wise multiplication
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool UnTangentSpace(const Eigen::RowVectorXd& in, Eigen::MatrixXd& out, const Eigen::MatrixXd& ref)
{
    const Eigen::Index n = out.rows();                          // Number of Features           => N
    if (!UnSqueezeUpperTriangle(in, out)) { return false; }

    const Eigen::MatrixXd sC     = (ref.size() == 0) ? Eigen::MatrixXd::Identity(n, n) : Eigen::MatrixXd(ref.sqrt()),
                          coeffs = Eigen::MatrixXd(out.triangularView<Eigen::StrictlyUpper>()) / M_SQRT2;

    out = sC * (Eigen::MatrixXd(out.diagonal().asDiagonal()) + coeffs + coeffs.transpose()).exp() * sC;
    return true;
}
//---------------------------------------------------------------------------------------------------

}  // namespace Geometry
