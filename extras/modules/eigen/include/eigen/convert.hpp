///-------------------------------------------------------------------------------------------------
/// 
/// \file convert.hpp
/// \brief All functions to Convert OpenViBE::CMatrix and Eigen::MatrixXd.
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

#pragma once

#include <openvibe/ov_all.h>
#include <Eigen/Dense>

namespace OpenViBE {
/// <summary>   Convert OpenViBE Matrix to Eigen Matrix. </summary>
/// <param name="in">   The Eigen Matrix. </param>
/// <param name="out">  The OpenViBE Matrix. </param>
OV_API bool MatrixConvert(const CMatrix& in, Eigen::MatrixXd& out);

/// <summary>   Convert Eigen Matrix to OpenViBE Matrix (It doesn't use Memory::copy because of Eigne store in column major by default). </summary>
/// <param name="in">   The Eigen Matrix. </param>
/// <param name="out">  The OpenViBE Matrix. </param>
OV_API bool MatrixConvert(const Eigen::MatrixXd& in, CMatrix& out);

/// <summary>   Convert Eigen Row Vector to OpenViBE Matrix with one dimension. </summary>
/// <param name="in">   The Eigen Row Vector. </param>
/// <param name="out">  The OpenViBE Matrix. </param>
OV_API bool MatrixConvert(const Eigen::RowVectorXd& in, CMatrix& out);

/// <summary>   Convert OpenViBE Matrix with one dimension to Eigen Row Vector. </summary>
/// <param name="in">   The OpenViBE Matrix. </param>
/// <param name="out">  The Eigen Row Vector. </param>
OV_API bool MatrixConvert(const CMatrix& in, Eigen::RowVectorXd& out);

/// <summary>   Convert vector double to OpenViBE Matrix with one dimension. </summary>
/// <param name="in">   The Vector of double. </param>
/// <param name="out">  The OpenViBE Matrix. </param>
OV_API bool MatrixConvert(const std::vector<double>& in, CMatrix& out);

/// <summary>   Convert vector of 2D Matrices into OpenViBE Matrix with 3 dimensions. </summary>
/// <param name="in">   The Vector of Matrices. </param>
/// <param name="out">  The OpenViBE Matrix. </param>
/// <param name="order"> Order of dimensions :
/// 0 : Vector on first dimension, Matrices row on second, Matrices column on third (0,1,2).
/// 1 : Vector on first dimension, Matrices column on second, Matrices row on third (0,2,1).
/// 2 : Matrices row on first dimension, Vector on second, Matrices column on third (1,0,2).
/// 3 : Matrices row on first dimension, Matrices column on second, Vector on third (1,2,0).
/// 4 : Matrices column on first dimension, Vector on second, Matrices row on third (2,0,1).
/// 5 : Matrices column on first dimension, Matrices row on second, Vector on third (2,1,0).
/// </param>
OV_API bool MatrixConvert(const std::vector<Eigen::MatrixXd>& in, CMatrix& out, const size_t order = 0);

}  // namespace OpenViBE
