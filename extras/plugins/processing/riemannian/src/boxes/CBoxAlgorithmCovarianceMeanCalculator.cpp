///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCovarianceMeanCalculator.cpp
/// \brief Classes implementation for the box computing the mean of the covariance matrix.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/11/2018.
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

#include "CBoxAlgorithmCovarianceMeanCalculator.hpp"
#include <geometry/Mean.hpp>
#include "eigen/convert.hpp"
#include <fstream>

#include "geometry/Basics.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMeanCalculator::initialize()
{
    // Stimulations
    m_i0StimulationCodec.initialize(*this, 0);
    m_iStimulation = m_i0StimulationCodec.getOutputStimulationSet();

    // Classes
    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    m_nbClass                      = size_t(boxContext.getInputCount() - 1);
    m_i1MatrixCodec.resize(m_nbClass);
    m_iMatrix.resize(m_nbClass);
    for (size_t k = 0; k < m_nbClass; ++k) {
        m_i1MatrixCodec[k].initialize(*this, k + 1);
        m_iMatrix[k] = m_i1MatrixCodec[k].getOutputMatrix();
    }

    m_o0MatrixCodec.initialize(*this, 0);
    m_oMatrix = m_o0MatrixCodec.getInputMatrix();

    // Settings
    m_metric          = Geometry::EMetric(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_filename        = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_stimulationName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_logLevel        = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3)));

    this->getLogManager() << m_logLevel << toString(m_metric) << " Metric\n";

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMeanCalculator::uninitialize()
{
    this->getLogManager() << m_logLevel << m_covs.size() << " Matrices Registered, Mean Matrix : \n" << Geometry::MatrixPrint(m_mean) << "\n";

    m_i0StimulationCodec.uninitialize();
    for (auto& codec : m_i1MatrixCodec) { codec.uninitialize(); }
    m_i1MatrixCodec.clear();
    m_iMatrix.clear();
    m_covs.clear();

    m_o0MatrixCodec.uninitialize();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMeanCalculator::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMeanCalculator::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //***** Stimulations *****
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_i0StimulationCodec.decode(i);                                             // Decode the chunk
        if (m_i0StimulationCodec.isBufferReceived())                                // Buffer received
        {
            for (size_t j = 0; j < m_iStimulation->size(); ++j) {
                if (m_iStimulation->getId(j) == m_stimulationName) {
                    OV_ERROR_UNLESS_KRF(Mean(m_covs, m_mean, m_metric), "Mean Compute Error", Kernel::ErrorType::BadProcessing);    // Compute the mean
                    MatrixConvert(m_mean, *m_oMatrix);
                    const uint64_t tStart = boxContext.getInputChunkStartTime(0, i),// Time Code Chunk Start
                                   tEnd   = boxContext.getInputChunkEndTime(0, i);  // Time Code Chunk End
                    m_o0MatrixCodec.encodeBuffer();                                 // Buffer encoded
                    boxContext.markOutputAsReadyToSend(0, tStart, tEnd);            // Makes the output available
                    OV_ERROR_UNLESS_KRF(saveCSV(), "CSV Writing Error", Kernel::ErrorType::BadFileWrite);
                }
            }
        }
    }

    //***** Matrix *****
    for (size_t k = 0; k < m_nbClass; ++k) {
        for (size_t i = 0; i < boxContext.getInputChunkCount(k + 1); ++i) {
            m_i1MatrixCodec[k].decode(i);                                           // Decode the chunk
            OV_ERROR_UNLESS_KRF(m_iMatrix[k]->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);

            if (m_i1MatrixCodec[k].isHeaderReceived() && k == 0)                    // First Header received
            {
                const uint64_t tStart = boxContext.getInputChunkStartTime(1, i),    // Time Code Chunk Start
                               tEnd   = boxContext.getInputChunkEndTime(1, i);      // Time Code Chunk End
                const size_t n        = m_iMatrix[0]->getDimensionSize(0);
                m_oMatrix->resize(n, n);                                            // Update Size and set to 0
                m_oMatrix->setNumLabels();
                m_o0MatrixCodec.encodeHeader();                                     // Header encoded
                boxContext.markOutputAsReadyToSend(0, tStart, tEnd);                // Makes the output available
            }
            else if (m_i1MatrixCodec[k].isBufferReceived())                         // Buffer received
            {
                Eigen::MatrixXd cov;
                MatrixConvert(*m_iMatrix[k], cov);
                m_covs.push_back(cov);
            }
            else if (m_i1MatrixCodec[k].isEndReceived() && k == 0)                  // First End received
            {
                const uint64_t tStart = boxContext.getInputChunkStartTime(1, i),    // Time Code Chunk Start
                               tEnd   = boxContext.getInputChunkEndTime(1, i);      // Time Code Chunk End
                m_o0MatrixCodec.encodeEnd();
                boxContext.markOutputAsReadyToSend(0, tStart, tEnd);                // Makes the output available
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMeanCalculator::saveCSV()
{
    if (m_filename.length() == 0) { return true; }

    std::ofstream file;
    file.open(m_filename.toASCIIString(), std::ios::trunc);
    OV_ERROR_UNLESS_KRF(file.is_open(), "Error opening file [" << m_filename << "] for writing", Kernel::ErrorType::BadFileWrite);

    // Header
    const size_t s = m_mean.rows();
    file << "Time:" << s << "x" << s << ",End Time,";
    for (size_t i = 1; i <= s; ++i) { for (size_t j = 0; j < s; ++j) { file << i << ":,"; } }
    file << "Event Id,Event Date,Event Duration\n";

    // Matrix
    file << "0.0000000000,0.0000000000,";   // Time
    const Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", ", ", "", "", "", ",,,\n");
    file << m_mean.format(fmt);
    file.close();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmCovarianceMeanCalculatorListener::onInputAdded(Kernel::IBox& box, const size_t index)
{
    box.setInputType(index, OV_TypeId_StreamedMatrix);
    std::stringstream name;
    name << "Input Covariance Matrix " << index;
    box.setInputName(index, name.str().c_str());
    return true;
}
//---------------------------------------------------------------------------------------------------
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
