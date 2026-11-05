///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCovarianceMatrixToFeatureVector.cpp
/// \brief Classes implementation for the box computing the Feature vector with the covariance matrix.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/10/2018.
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmCovarianceMatrixToFeatureVector.hpp"
#include "eigen/convert.hpp"
#include <fstream>

#include "geometry/Basics.hpp"
#include "geometry/Featurization.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixToFeatureVector::initialize()
{
    //***** Codec Initialization *****
    m_i0MatrixCodec.initialize(*this, 0);
    m_o0FeatureCodec.initialize(*this, 0);
    m_iMatrix = m_i0MatrixCodec.getOutputMatrix();
    m_oMatrix = m_o0FeatureCodec.getInputMatrix();

    //***** Settings Initialization *****
    m_tangentSpace = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_logLevel     = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
    if (m_tangentSpace) {
        this->getLogManager() << m_logLevel << "Tangent Space\n";
        OV_ERROR_UNLESS_KRF(initRef(), "the reference matrix from this box's setting \"Filename to Reference Matrix (CSV, empty for Identity)\" could not be prepared, as explained just before this message, if there is an explanation. Please forgive us.", Kernel::ErrorType::BadSetting);
    }
    else { this->getLogManager() << m_logLevel << "Squeeze Upper Matrix\n"; }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixToFeatureVector::uninitialize()
{
    m_i0MatrixCodec.uninitialize();
    m_o0FeatureCodec.uninitialize();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixToFeatureVector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixToFeatureVector::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_i0MatrixCodec.decode(i);                                          // Decode the chunk
        OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2 && m_iMatrix->getDimensionSize(0) == m_iMatrix->getDimensionSize(1),
                            "Invalid Input Signal", Kernel::ErrorType::BadInput);

        const size_t nChannels = size_t(m_iMatrix->getDimensionSize(0));
        const uint64_t tStart  = boxContext.getInputChunkStartTime(0, i),   // Time Code Chunk Start
                       tEnd    = boxContext.getInputChunkEndTime(0, i);     // Time Code Chunk End

        if (m_i0MatrixCodec.isHeaderReceived())                             // Header received
        {
            m_oMatrix->resize(nChannels * (nChannels + 1) / 2);             // Update Size and set to 0
            m_o0FeatureCodec.encodeHeader();                                // Header encoded
        }
        else if (m_i0MatrixCodec.isBufferReceived())                        // Buffer received
        {
            OV_ERROR_UNLESS_KRF(featurization(), "Featurization Processing Error", Kernel::ErrorType::BadProcessing); // Transformation
            m_o0FeatureCodec.encodeBuffer();                                // Buffer encoded
        }
        else if (m_i0MatrixCodec.isEndReceived()) { m_o0FeatureCodec.encodeEnd(); } // End receivded and encoded

        boxContext.markOutputAsReadyToSend(0, tStart, tEnd);                // Makes the output available
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixToFeatureVector::featurization() const
{
    Eigen::MatrixXd cov;
    Eigen::RowVectorXd v;
    if (!MatrixConvert(*m_iMatrix, cov)) { return false; }
    if (!Geometry::Featurization(cov, v, m_tangentSpace, m_ref)) { return false; }
    if (!MatrixConvert(v, *m_oMatrix)) { return false; }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMatrixToFeatureVector::initRef()
{
    //***** Open the CSV *****
    const CString name = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    if (name.length() == 0) {
        this->getLogManager() << m_logLevel << "Empty reference Matrix\n";
        return true;
    }
    std::ifstream file(name, std::ifstream::in);
    OV_ERROR_UNLESS_KRF(file.is_open(), "the file [" << name << "] from this box's setting \"Filename to Reference Matrix (CSV, empty for Identity)\" could not be opened. Perhaps it does not exist, or the path is misspelt. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadFileRead);

    //***** Parse the CSV *****
    std::string line;
    getline(file, line);    // Header
    getline(file, line);    // matrix line
    const std::vector<std::string> data = Geometry::Split(line, ",");

    //***** Transform to MatrixXd *****
    const auto first = data.begin() + 2, last = data.end() - 3;
    const std::vector<std::string> mat(first, last);
    const Eigen::Index n = Eigen::Index(sqrt(mat.size()));
    OV_ERROR_UNLESS_KRF(size_t(n*n) == mat.size(), "the reference matrix in the file [" << name << "] holds " << mat.size() << " values, while a matrix with " << n << " columns needs " << n * n << ". Would you be so very kind as to check the file? Thank you for your patience.", Kernel::ErrorType::BadFileParsing);
    m_ref.resize(n, n);
    Eigen::Index idx = 0;
    for (Eigen::Index i = 0; i < n; ++i) { for (Eigen::Index j = 0; j < n; ++j) { m_ref(i, j) = stod(mat[idx++]); } }

    //***** Log Information *****
    this->getLogManager() << m_logLevel << "REF Matrix : \n" << Geometry::MatrixPrint(m_ref) << "\n";

    //***** Close the CSV *****
    file.close();
    return true;
}
//---------------------------------------------------------------------------------------------------
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
