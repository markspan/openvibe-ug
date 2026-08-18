///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixAffineTransformation.cpp
/// \brief Classes implementation for the box Affine Transformation.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 28/08/2019.
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

#include "CBoxAlgorithmMatrixAffineTransformation.hpp"
#include "eigen/convert.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixAffineTransformation::initialize()
{
    // Matrix
    m_iMatrixCodec.initialize(*this, 0);
    m_iMatrix = m_iMatrixCodec.getOutputMatrix();
    m_oMatrixCodec.initialize(*this, 0);
    m_oMatrix = m_oMatrixCodec.getInputMatrix();

    m_ifilename  = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)).toASCIIString();
    m_ofilename  = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)).toASCIIString();
    m_continuous = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    OV_ERROR_UNLESS_KRF(loadXML(), "Loading XML Error", Kernel::ErrorType::BadFileRead);
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixAffineTransformation::uninitialize()
{
    if (!m_continuous && !m_samples.empty() && m_ofilename.length() != 0) {
        OV_ERROR_UNLESS_KRF(m_bias.ComputeBias(m_samples), "Bias Compute Error", Kernel::ErrorType::BadProcessing);
    }
    OV_ERROR_UNLESS_KRF(saveXML(), "Saving XML Error", Kernel::ErrorType::BadFileWrite);
    m_iMatrixCodec.uninitialize();
    m_oMatrixCodec.uninitialize();
    m_samples.clear();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixAffineTransformation::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixAffineTransformation::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    //***** Matrix *****
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_iMatrixCodec.decode(i);                                           // Decode the chunk
        OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);
        if (m_iMatrixCodec.isHeaderReceived())                              // Header received
        {
            m_oMatrix->copyDescription(*m_iMatrix);                         // Update Size and set to 0
            m_oMatrixCodec.encodeHeader();
        }
        if (m_iMatrixCodec.isBufferReceived())                              // Buffer received
        {
            Eigen::MatrixXd in, out;
            MatrixConvert(*m_iMatrix, in);
            m_samples.push_back(in);
            if (m_continuous) { m_bias.UpdateBias(in); }                    // We update each time
            // We wan't to apply a bias without bias Identity matrix is used
            if (m_bias.GetBias().size() == 0) { m_bias.SetBias(Eigen::MatrixXd::Identity(in.rows(), in.cols())); }
            m_bias.ApplyBias(in, out);
            MatrixConvert(out, *m_oMatrix);
            m_oMatrixCodec.encodeBuffer();
        }
        else if (m_iMatrixCodec.isEndReceived()) { m_oMatrixCodec.encodeEnd(); }    // End received

        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i);    // Time Code Chunk Start
        const uint64_t tEnd   = boxContext.getInputChunkEndTime(0, i);      // Time Code Chunk End
        boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixAffineTransformation::loadXML()
{
    if (m_ifilename.length() == 0) { return true; } // The bias haven't initialization
    return m_bias.LoadXML(m_ifilename);             // The bias have initialization
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixAffineTransformation::saveXML() const
{
    if (m_ofilename.length() == 0) { return true; } // The bias isn't saved
    return m_bias.SaveXML(m_ofilename);             // The bias is saved
}
//---------------------------------------------------------------------------------------------------
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
