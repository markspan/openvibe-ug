///-------------------------------------------------------------------------------------------------
///
/// \file spectrumTools.hpp
/// \brief Tools for spectrum computation - to be merged with connectivity measures
/// \author Arthur Desbois
/// \version 1.0
///
/// \copyright Copyright(C) 2021-2022 Inria
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


#include <openvibe/ov_all.h>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

/// \brief Computes the autoregressive model coefficients & PSD based on those coefficients.
/// \param samples Vector containing the samples of the signal
/// \param arCoeffsOutput Vector containing AR Coeffs
/// \param arPsd Vector containing the AR-based PSD of the signal
/// \param order The autoregressive model order
/// \param psdSize The size of the output PSD, which is also the size of the FFT applied on the AR coefficients
/// \param detrend Activate the DC removal at the start of the process
bool autoregressiveCoeffsAndPsd(const std::vector<Eigen::VectorXd>& samples, std::vector<Eigen::VectorXd>& arCoeffsOutput, std::vector<Eigen::VectorXd>& arPsd, const uint64_t order, const uint64_t psdSize, const bool detrend);

/// \brief Initialize the exp(2i pi f) matrix needed in spectral estimation based on MVAR model computation
/// \param order The autoregressive model order
/// \param psdSize The size of the output PSD, which is also the size of the FFT applied on the AR coefficients
/// \param sampRate The sampling rate of the signals
Eigen::MatrixXcd generateExpMat(const uint64_t order, const uint64_t psdSize, const uint64_t sampRate);

/// \brief Computes a MVAR estimation on the input signals, and the PSD/CPSD matrix based on this model
/// \param samples1 Vector containing the samples of the first signal
/// \param samples2 Vector containing the samples of the second signal
/// \param mvarMatrix Matrix containing the PSDs (on the diagonal) and CPSDs (other entries) of the signals.
/// \param order The autoregressive model order
/// \param psdSize The size of the output PSD, which is also the size of the FFT applied on the AR coefficients
/// \param sampRate The sampling rate of the signals
/// \param detrend Activate the DC removal at the start of the process
/// \param expMat Matrix of exp(2i pi f) for PSD/CPSD computation. Use generateExpMat() to generate it.
bool mvarSpectralEstimation(const Eigen::VectorXd& samples1, const Eigen::VectorXd& samples2, std::vector<std::vector<Eigen::VectorXcd>>& mvarMatrix, 
    const uint64_t order, const uint64_t psdSize, const uint64_t sampRate, const bool detrend, const Eigen::MatrixXcd& expMat);

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

