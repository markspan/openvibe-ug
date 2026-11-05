///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxXDAWNTrainer.cpp
/// \brief Classes implementation for the box XDAWN Trainer.
/// \author Yann Renard (Mensia Technologies).
/// \version 1.0.
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
#include "CBoxXDAWNTrainer.hpp"

#include "fs/Files.h"

#include <fstream>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

//--------------------------------------------------------------------------------
CBoxXDAWNTrainer::CBoxXDAWNTrainer() {}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxXDAWNTrainer::initialize()
{
    m_trainStimulationID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_filename           = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)).toASCIIString();

    OV_ERROR_UNLESS_KRF(m_filename.length() != 0, "this box's setting \"Spatial filter configuration\" is empty. Would you be so very kind as to choose a file? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    if (FS::Files::fileExists(m_filename.c_str())) {
        std::ofstream file;
        file.open(m_filename, std::ios_base::out | std::ios_base::trunc);
        OV_ERROR_UNLESS_KRF(file.is_open(), "the file [" << m_filename << "] from this box's setting \"Spatial filter configuration\" exists, but could not be opened for writing. Perhaps it is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileRead);
        file.close();
    }

    const int filterDimension = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    OV_ERROR_UNLESS_KRF(filterDimension > 0, "The dimension of the filter must be strictly positive.\n", Kernel::ErrorType::OutOfBound);

    m_filterDim = size_t(filterDimension);

    m_saveAsBoxConfig = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);


    m_stimDecoder.initialize(*this, 0);
    m_signalDecoder[0].initialize(*this, 1);
    m_signalDecoder[1].initialize(*this, 2);
    m_stimEncoder.initialize(*this, 0);

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxXDAWNTrainer::uninitialize()
{
    m_stimDecoder.uninitialize();
    m_signalDecoder[0].uninitialize();
    m_signalDecoder[1].uninitialize();
    m_stimEncoder.uninitialize();

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxXDAWNTrainer::processInput(const size_t index)
{
    if (index == 0) { this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxXDAWNTrainer::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    bool train = false;

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_stimEncoder.getInputStimulationSet()->clear();
        m_stimDecoder.decode(i);

        if (m_stimDecoder.isHeaderReceived()) { m_stimEncoder.encodeHeader(); }
        if (m_stimDecoder.isBufferReceived()) {
            for (size_t j = 0; j < m_stimDecoder.getOutputStimulationSet()->size(); ++j) {
                const uint64_t stimulationId = m_stimDecoder.getOutputStimulationSet()->getId(j);

                if (stimulationId == m_trainStimulationID) {
                    train = true;

                    m_stimEncoder.getInputStimulationSet()->push_back(
                        OVTK_StimulationId_TrainCompleted, m_stimDecoder.getOutputStimulationSet()->getDate(j), 0);
                }
            }

            m_stimEncoder.encodeBuffer();
        }
        if (m_stimDecoder.isEndReceived()) { m_stimEncoder.encodeEnd(); }

        boxCtx.markOutputAsReadyToSend(0, boxCtx.getInputChunkStartTime(0, i), boxCtx.getInputChunkEndTime(0, i));
    }

    if (train) {
        std::vector<size_t> erpSampleIndexes;
        std::array<Eigen::MatrixXd, 2> X; // X[0] is session matrix, X[1] is averaged ERP
        std::array<Eigen::MatrixXd, 2> C; // Covariance matrices
        std::array<size_t, 2> n;
        size_t nChannel = 0;

        this->getLogManager() << Kernel::LogLevel_Info << "Received train stimulation...\n";

        // Decodes input signals

        for (size_t j = 0; j < 2; ++j) {
            n[j] = 0;

            for (size_t i = 0; i < boxCtx.getInputChunkCount(j + 1); ++i) {
                Toolkit::TSignalDecoder<CBoxXDAWNTrainer>& decoder = m_signalDecoder[j];
                decoder.decode(i);

                CMatrix* matrix       = decoder.getOutputMatrix();
                nChannel              = matrix->getDimensionSize(0);
                const size_t nSample  = matrix->getDimensionSize(1);
                const size_t sampling = size_t(decoder.getOutputSamplingRate());

                if (decoder.isHeaderReceived()) {
                    OV_ERROR_UNLESS_KRF(sampling > 0, "Input sampling frequency is equal to 0. Plugin can not process.\n", Kernel::ErrorType::OutOfBound);
                    OV_ERROR_UNLESS_KRF(nChannel > 0, "For condition " << j + 1 << " got no channel in signal stream.\n", Kernel::ErrorType::OutOfBound);
                    OV_ERROR_UNLESS_KRF(nSample > 0, "For condition " << j + 1 << " got no samples in signal stream.\n", Kernel::ErrorType::OutOfBound);
                    OV_ERROR_UNLESS_KRF(m_filterDim <= nChannel, "The filter dimension must not be superior than the channel count.\n",
                                        Kernel::ErrorType::OutOfBound);

                    if (!n[0]) // Initialize signal buffer (X[0]) only when receiving input signal header.
                    {
                        X[j].resize(nChannel, (boxCtx.getInputChunkCount(j + 1) - 1) * nSample);
                    }
                    else // otherwise, only ERP averaging buffer (X[1]) is reset
                    {
                        X[j] = Eigen::MatrixXd::Zero(nChannel, nSample);
                    }
                }

                if (decoder.isBufferReceived()) {
                    Eigen::MatrixXd A = Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                        matrix->getBuffer(), nChannel, nSample);

                    switch (j) {
                        case 0: // Session                          
                            X[j].block(0, n[j] * A.cols(), A.rows(), A.cols()) = A;
                            break;

                        case 1: // ERP
                            X[j] = X[j] + A; // Computes sumed ERP

                            // $$$ Assumes continuous session signal starting at date 0
                            {
                                size_t erpSampleIndex = size_t(((boxCtx.getInputChunkStartTime(j + 1, i) >> 16) * sampling) >> 16);
                                erpSampleIndexes.push_back(erpSampleIndex);
                            }
                            break;

                        default: break;
                    }

                    n[j]++;
                }

#if 0
                if (decoder.isEndReceived())
                {
                }
#endif
            }

            OV_ERROR_UNLESS_KRF(n[j] != 0, "Did not have input signal for condition " << j + 1 << "\n", Kernel::ErrorType::BadValue);

            switch (j) {
                case 0: // Session
                    break;

                case 1: // ERP
                    X[j] = X[j] / double(n[j]); // Averages ERP
                    break;

                default: break;
            }
        }

        // We need equal number of channels
        OV_ERROR_UNLESS_KRF(X[0].rows() == X[1].rows(),
                            "Dimension mismatch, first input had " << size_t(X[0].rows()) << " channels while second input had "
                            << size_t(X[1].rows()) << " channels\n", Kernel::ErrorType::BadValue);

        // Grabs usefull values

        const size_t sampleCountSession = X[0].cols();
        const size_t sampleCountERP     = X[1].cols();

        // Now we compute matrix D

        const Eigen::MatrixXd DI = Eigen::MatrixXd::Identity(sampleCountERP, sampleCountERP);
        Eigen::MatrixXd D        = Eigen::MatrixXd::Zero(sampleCountERP, sampleCountSession);

        for (const size_t sampleIndex : erpSampleIndexes) { D.block(0, sampleIndex, sampleCountERP, sampleCountERP) += DI; }

        // Computes covariance matrices

        C[0] = X[0] * X[0].transpose();
        C[1] = /*Y * Y.transpose();*/ X[1] * /* D.transpose() * */ (D * D.transpose()).fullPivLu().inverse() /* * D */ * X[1].transpose();

        // Solves generalized eigen decomposition

        const Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver(C[0].selfadjointView<Eigen::Lower>(), C[1].selfadjointView<Eigen::Lower>());

        if (eigenSolver.info() != Eigen::Success) {
            const enum Eigen::ComputationInfo error = eigenSolver.info();
            const char* errorMessage                = "unknown";

            switch (error) {
                case Eigen::NumericalIssue: errorMessage = "Numerical issue";
                    break;
                case Eigen::NoConvergence: errorMessage = "No convergence";
                    break;
                // case Eigen::InvalidInput: errorMessage="Invalid input"; break; // FIXME
                default: break;
            }

            OV_ERROR_KRF("Could not solve generalized eigen decomposition, got error[" << CString(errorMessage) << "]\n", Kernel::ErrorType::BadProcessing);
        }

        // Create a CMatrix mapper that can spool the filters to a file

        CMatrix eigenVectors;
        eigenVectors.resize(m_filterDim, nChannel);

        Eigen::Map<MatrixXdRowMajor> vectorsMapper(eigenVectors.getBuffer(), m_filterDim, nChannel);

        vectorsMapper.block(0, 0, m_filterDim, nChannel) = eigenSolver.eigenvectors().block(0, 0, nChannel, m_filterDim).transpose();

        // Saves filters
        if (!saveFilter(eigenVectors, nChannel)) { return false; }
        this->getLogManager() << Kernel::LogLevel_Info << "Training finished and saved to [" << m_filename << "]!\n";
    }

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxXDAWNTrainer::saveFilter(const CMatrix& m, const size_t nChannel)
{
    if (m_saveAsBoxConfig) {
        std::ofstream file;
        file.open(m_filename, std::ios_base::out | std::ios_base::trunc);
        if (!file.is_open()) {
            getLogManager() << Kernel::LogLevel_Error << "The file [" << m_filename << "] could not be opened for writing...";
            return false;
        }

        file << "<OpenViBE-SettingsOverride>" << std::endl;
        file << "\t<SettingValue>";
        for (size_t i = 0; i < m.getBufferElementCount(); ++i) { file << std::scientific << m.getBuffer()[i] << " "; }
        file << "</SettingValue>" << std::endl;
        file << "\t<SettingValue>" << m_filterDim << "</SettingValue>" << std::endl;
        file << "\t<SettingValue>" << nChannel << "</SettingValue>" << std::endl;
        file << "\t<SettingValue></SettingValue>" << std::endl;
        file << "</OpenViBE-SettingsOverride>";
        file.close();
    }
    else {
        if (!m.toTextFile(m_filename)) {
            getLogManager() << Kernel::LogLevel_Error << "Unable to save to [" << m_filename << "]\n";
            return false;
        }
    }
    return true;
}
//--------------------------------------------------------------------------------

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
