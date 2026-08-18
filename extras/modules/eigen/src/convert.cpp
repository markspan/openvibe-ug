///-------------------------------------------------------------------------------------------------
/// 
/// \file convert.cpp
/// \brief All functions to Convert OpenViBE::CMatrix and Eigen::MatrixXd, links to Eigen function, manipulate OpenVibe::CMatrix and more.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/11/2021.
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

#include "convert.hpp"

namespace OpenViBE {
//---------------------------------------------------------------------------------------------------
bool MatrixConvert(const CMatrix& in, Eigen::MatrixXd& out)
{
    if (in.getDimensionCount() != 2) { return false; }
    out.resize(in.getDimensionSize(0), in.getDimensionSize(1));

    // double loop to avoid the problem of row major and column major storage
    size_t idx           = 0;
    const double* buffer = in.getBuffer();
    for (size_t i = 0, nR = out.rows(); i < nR; ++i) { for (size_t j = 0, nC = out.cols(); j < nC; ++j) { out(i, j) = buffer[idx++]; } }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MatrixConvert(const Eigen::MatrixXd& in, CMatrix& out)
{
    if (in.rows() == 0 || in.cols() == 0) { return false; }
    const size_t nR = in.rows(), nC = in.cols();
    out.resize(nR, nC);
    out.setNumLabels();

    // double loop to avoid the problem of row major and column major storage
    size_t idx     = 0;
    double* buffer = out.getBuffer();
    for (size_t i = 0; i < nR; ++i) { for (size_t j = 0; j < nC; ++j) { buffer[idx++] = in(i, j); } }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MatrixConvert(const Eigen::RowVectorXd& in, CMatrix& out)
{
    if (in.size() == 0) { return false; }
    out.resize(in.size());
    //one row system copy doesn't cause problem
    std::copy_n(in.data(), out.getSize(), out.getBuffer());
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MatrixConvert(const CMatrix& in, Eigen::RowVectorXd& out)
{
    if (in.getDimensionCount() != 1) { return false; }
    out.resize(in.getDimensionSize(0));
    //one row system copy doesn't cause problem
    std::copy_n(in.getBuffer(), in.getSize(), out.data());
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MatrixConvert(const std::vector<double>& in, CMatrix& out)
{
    if (in.empty()) { return false; }
    out.resize(in.size());
    //one row system copy doesn't cause problem
    std::copy_n(in.data(), out.getSize(), out.getBuffer());
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool MatrixConvert(const std::vector<Eigen::MatrixXd>& in, CMatrix& out, const size_t order)
{
    // Check inputs
    if (in.empty() || in[0].rows() == 0 || in[0].cols() == 0) { return false; }
    std::vector<size_t> d{ in.size(), size_t(in[0].rows()), size_t(in[0].cols()) };
    for (const auto& m : in) { if (m.rows() != d[1] || m.cols() != d[2]) { return false; } }

    // Fill Buffer
    size_t idx = 0;
    if (order == 0) {
        out.resize({ d[0], d[1], d[2] });
        double* buffer = out.getBuffer();
        for (size_t i = 0; i < d[0]; ++i) { for (size_t j = 0; j < d[1]; ++j) { for (size_t k = 0; k < d[2]; ++k) { buffer[idx++] = in[i](j, k); } } }
    }
    else if (order == 1) {
        out.resize({ d[0], d[2], d[1] });
        double* buffer = out.getBuffer();
        for (size_t i = 0; i < d[0]; ++i) { for (size_t j = 0; j < d[2]; ++j) { for (size_t k = 0; k < d[1]; ++k) { buffer[idx++] = in[i](k, j); } } }
    }
    else if (order == 2) {
        out.resize({ d[1], d[0], d[2] });
        double* buffer = out.getBuffer();
        for (size_t i = 0; i < d[1]; ++i) { for (size_t j = 0; j < d[0]; ++j) { for (size_t k = 0; k < d[2]; ++k) { buffer[idx++] = in[j](i, k); } } }
    }
    else if (order == 3) {
        out.resize({ d[1], d[2], d[0] });
        double* buffer = out.getBuffer();
        for (size_t i = 0; i < d[1]; ++i) { for (size_t j = 0; j < d[2]; ++j) { for (size_t k = 0; k < d[0]; ++k) { buffer[idx++] = in[k](i, j); } } }
    }
    else if (order == 4) {
        out.resize({ d[2], d[0], d[1] });
        double* buffer = out.getBuffer();
        for (size_t i = 0; i < d[2]; ++i) { for (size_t j = 0; j < d[0]; ++j) { for (size_t k = 0; k < d[1]; ++k) { buffer[idx++] = in[j](k, i); } } }
    }
    else if (order == 5) {
        out.resize({ d[2], d[1], d[0] });
        double* buffer = out.getBuffer();
        for (size_t i = 0; i < d[2]; ++i) { for (size_t j = 0; j < d[1]; ++j) { for (size_t k = 0; k < d[0]; ++k) { buffer[idx++] = in[k](j, i); } } }
    }
    else { return false; }

    return true;
}
//---------------------------------------------------------------------------------------------------
}  // namespace OpenViBE
