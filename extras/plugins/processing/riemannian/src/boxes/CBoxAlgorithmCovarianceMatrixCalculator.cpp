///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCovarianceMatrixCalculator.cpp
/// \brief Classes implementation for the box computing the covariance matrix
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 16/10/2018.
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

#include "CBoxAlgorithmCovarianceMatrixCalculator.hpp"
#include "eigen/convert.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixCalculator::initialize()
{
    m_i0SignalCodec.initialize(*this, 0);
    m_o0MatrixCodec.initialize(*this, 0);
    m_iMatrix = m_i0SignalCodec.getOutputMatrix();
    m_oMatrix = m_o0MatrixCodec.getInputMatrix();

    //***** Settings *****
    m_est      = Geometry::EEstimator(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_center   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_logLevel = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));

    this->getLogManager() << m_logLevel << toString(m_est) << " Estimator" << (m_center ? ", Center Data " : "") << "\n";
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixCalculator::uninitialize()
{
    m_i0SignalCodec.uninitialize();
    m_o0MatrixCodec.uninitialize();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixCalculator::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixCalculator::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_i0SignalCodec.decode(i);                                          // Decode the chunk
        OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);

        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i),    // Time Code Chunk Start
                       tEnd   = boxContext.getInputChunkEndTime(0, i);      // Time Code Chunk End
        const auto nChannels  = size_t(m_iMatrix->getDimensionSize(0));

        if (m_i0SignalCodec.isHeaderReceived())                             // Header received
        {
            m_oMatrix->resize(nChannels, nChannels);                        // Update Size and set to 0
            m_oMatrix->setNumLabels();                                      // Change label to have 1 to N label on each dim
            m_o0MatrixCodec.encodeHeader();                                 // Header encoded
        }
        else if (m_i0SignalCodec.isBufferReceived())                        // Buffer received
        {
            OV_ERROR_UNLESS_KRF(covarianceMatrix(), "Covariance Matrix Processing Error", Kernel::ErrorType::BadProcessing);    // Compute Covariance
            m_o0MatrixCodec.encodeBuffer();                                 // Buffer encoded
        }
        else if (m_i0SignalCodec.isEndReceived()) { m_o0MatrixCodec.encodeEnd(); }  // End receivded and encoded

        boxContext.markOutputAsReadyToSend(0, tStart, tEnd);                // Makes the output available
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixCalculator::covarianceMatrix() const
{
    Eigen::MatrixXd mS, mCov;
    if (!MatrixConvert(*m_iMatrix, mS)) { return false; }
    const Geometry::EStandardization s = m_center ? Geometry::EStandardization::Center : Geometry::EStandardization::None;
    if (!CovarianceMatrix(mS, mCov, m_est, s)) { return false; }
    if (!MatrixConvert(mCov, *m_oMatrix)) { return false; }
    return true;
}
//---------------------------------------------------------------------------------------------------
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
