///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRegularizedCSPTrainer.cpp
/// \brief Classes implementation for the Box Regularized CSP Trainer.
/// \author Jussi T. Lindgren (Inria).
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

#include "CBoxAlgorithmRegularizedCSPTrainer.hpp"

#include <sstream>

#include <Eigen/Eigenvalues>
#include <fs/Files.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

// typedef Eigen::Matrix< double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor > MatrixXdRowMajor;

bool CBoxAlgorithmRegularizedCSPTrainer::initialize()
{
    m_stimDecoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);

    m_nClasses = this->getStaticBoxContext().getInputCount() - 1;

    m_covProxies.resize(m_nClasses);

    m_stimID          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_configFilename  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_filtersPerClass = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
    m_saveAsBoxConf   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    OV_ERROR_UNLESS_KRF(m_filtersPerClass > 0, // && m_filtersPerClass%2 == 0,
                        "Invalid filter dimension number [" << m_filtersPerClass << "] (expected value > 0)", // even ?
                        Kernel::ErrorType::BadSetting);

    m_hasBeenInitialized = true;

    m_signalDecoders.resize(m_nClasses);
    for (size_t i = 0; i < m_nClasses; ++i) {
        m_signalDecoders[i].initialize(*this, i + 1);

        const CIdentifier covAlgId = this->getAlgorithmManager().createAlgorithm(Algorithm_OnlineCovariance);
        OV_ERROR_UNLESS_KRF(covAlgId != CIdentifier::undefined(), "Failed to create online covariance algorithm", Kernel::ErrorType::BadResourceCreation);

        m_covProxies[i].cov = &this->getAlgorithmManager().getAlgorithm(covAlgId);
        OV_ERROR_UNLESS_KRF(m_covProxies[i].cov->initialize(), "Failed to initialize online covariance algorithm", Kernel::ErrorType::Internal);

        // Set the params of the cov algorithm
        Kernel::TParameterHandler<uint64_t> updateMethod(m_covProxies[i].cov->getInputParameter(OnlineCovariance_InputParameterId_UpdateMethod));
        Kernel::TParameterHandler<bool> traceNormalization(
            m_covProxies[i].cov->getInputParameter(OnlineCovariance_InputParameterId_TraceNormalization));
        Kernel::TParameterHandler<double> shrinkage(m_covProxies[i].cov->getInputParameter(OnlineCovariance_InputParameterId_Shrinkage));

        updateMethod       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
        traceNormalization = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
        shrinkage          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
    }

    m_tikhonov = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);

    OV_ERROR_UNLESS_KRF(m_configFilename != CString(""), "Output filename is required in box configuration", Kernel::ErrorType::BadSetting);

    return true;
}

bool CBoxAlgorithmRegularizedCSPTrainer::uninitialize()
{
    m_stimDecoder.uninitialize();
    m_encoder.uninitialize();

    if (m_hasBeenInitialized) {
        for (size_t i = 0; i < m_nClasses; ++i) {
            m_signalDecoders[i].uninitialize();
            if (m_covProxies[i].cov) {
                m_covProxies[i].cov->uninitialize();
                getAlgorithmManager().releaseAlgorithm(*m_covProxies[i].cov);
            }
        }
    }
    m_covProxies.clear();

    return true;
}

bool CBoxAlgorithmRegularizedCSPTrainer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmRegularizedCSPTrainer::updateCov(const size_t index)
{
    const Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
    SIncrementalCovarianceProxy& curCovProxy(m_covProxies[index]);
    for (size_t i = 0; i < boxCtx.getInputChunkCount(index + 1); ++i) {
        auto* decoder         = &m_signalDecoders[index];
        const CMatrix* matrix = decoder->getOutputMatrix();

        decoder->decode(i);
        if (decoder->isHeaderReceived()) {
            Kernel::TParameterHandler<CMatrix*> features(curCovProxy.cov->getInputParameter(OnlineCovariance_InputParameterId_InputVectors));

            features->resize(matrix->getDimensionSize(1), matrix->getDimensionSize(0));

            OV_ERROR_UNLESS_KRF(m_filtersPerClass <= matrix->getDimensionSize(0),
                                "Invalid CSP filter dimension of [" << m_filtersPerClass << "] for stream " << i+1
                                << " (expected value must be less than input channel count ["<< matrix->getDimensionSize(1) <<"])",
                                Kernel::ErrorType::BadSetting);

            curCovProxy.cov->activateInputTrigger(OnlineCovariance_Process_Reset, true);

            OV_ERROR_UNLESS_KRF(curCovProxy.cov->process(), "Failed to parametrize covariance algorithm", Kernel::ErrorType::Internal);
        }
        if (decoder->isBufferReceived()) {
            Kernel::TParameterHandler<CMatrix*> features(curCovProxy.cov->getInputParameter(OnlineCovariance_InputParameterId_InputVectors));

            // transpose data
            const size_t nChannels = matrix->getDimensionSize(0);
            const size_t nSamples  = matrix->getDimensionSize(1);

            const Eigen::Map<MatrixXdRowMajor> inputMapper(const_cast<double*>(matrix->getBuffer()), nChannels, nSamples);
            Eigen::Map<MatrixXdRowMajor> outputMapper(features->getBuffer(), nSamples, nChannels);
            outputMapper = inputMapper.transpose();

            curCovProxy.cov->activateInputTrigger(OnlineCovariance_Process_Update, true);
            curCovProxy.cov->process();

            curCovProxy.nBuffers++;
            curCovProxy.nSamples += nSamples;
        }
        // if (decoder->isEndReceived()) { }            // nop
    }
    return true;
}

//
// Returns a sample-weighted average of given covariance matrices that does not include the cov of SkipIndex
//
// @todo error handling is a bit scarce
//
// @note This will recompute the weights on every call, but given how small amount of
// computations we're speaking of, there's not much point in optimizing.
//
bool CBoxAlgorithmRegularizedCSPTrainer::outclassCovAverage(const size_t skipIndex, const std::vector<Eigen::MatrixXd>& cov, Eigen::MatrixXd& covAvg)
{
    if (cov.empty() || skipIndex >= cov.size()) { return false; }

    std::vector<double> classWeights;
    uint64_t totalOutclassSamples = 0;

    // Compute the total number of samples
    for (size_t i = 0; i < m_nClasses; ++i) {
        if (i != skipIndex) {
            totalOutclassSamples += m_covProxies[i].nSamples;
        }
    }

    // Compute weigths for averaging
    classWeights.resize(m_nClasses);
    for (size_t i = 0; i < m_nClasses; ++i) {
        classWeights[i] = i == skipIndex ? 0 : m_covProxies[i].nSamples / double(totalOutclassSamples);
        this->getLogManager() << Kernel::LogLevel_Debug << "Condition " << i + 1 << " averaging weight = " << classWeights[i] << "\n";
    }

    // Average the covs
    covAvg.resizeLike(cov[0]);
    covAvg.setZero();
    for (size_t i = 0; i < m_nClasses; ++i) {
        covAvg += (classWeights[i] * cov[i]); 
    }

    return true;
}

bool CBoxAlgorithmRegularizedCSPTrainer::computeCSP(const std::vector<Eigen::MatrixXd>& cov, std::vector<Eigen::MatrixXd>& sortedEigenVectors,
                                                    std::vector<Eigen::VectorXd>& sortedEigenValues)
{
    this->getLogManager() << Kernel::LogLevel_Info << "Compute CSP Begin\n";
    // We wouldn't need to store all this -- they are kept for debugging purposes
    std::vector<Eigen::VectorXd> eigenValues(m_nClasses);
    std::vector<Eigen::MatrixXd> eigenVectors(m_nClasses), covInv(m_nClasses), covProd(m_nClasses);
    Eigen::MatrixXd tikhonov, outclassCov;
    tikhonov.resizeLike(cov[0]);
    tikhonov.setIdentity();
    tikhonov *= m_tikhonov;

    sortedEigenVectors.resize(m_nClasses);
    sortedEigenValues.resize(m_nClasses);

    // To get the CSP filters, we compute two sets of eigenvectors, eig(inv(sigma2+tikhonov)*sigma1) and eig(inv(sigma1+tikhonov)*sigma2
    // and pick the ones corresponding to the largest eigenvalues as spatial filters [following Lotte & Guan 2011]. Assumes the shrink
    // of the sigmas (if its used) has been performed inside the cov computation algorithm.

    Eigen::EigenSolver<Eigen::MatrixXd> solver;

    for (size_t c = 0; c < m_nClasses; ++c) {
        Eigen::FullPivLU<Eigen::MatrixXd> lu(cov[c] + tikhonov);
        try {
            OV_ERROR_UNLESS_KRF(lu.isInvertible(), "Covariance matrix for condition [" << c + 1 << "] is not invertible!", Kernel::ErrorType::BadProcessing);
            covInv[c] = lu.inverse();
        }
        catch (...) {
            OV_ERROR_KRF("Inversion failed for condition [" << c + 1 << "]", Kernel::ErrorType::BadProcessing);
        }

        // Compute covariance in all the classes except 'classIndex'.
        OV_ERROR_UNLESS_KRF(outclassCovAverage(c, cov, outclassCov), "Outclass cov computation failed for condition [" << c + 1 << "]",
                            Kernel::ErrorType::BadProcessing);

        covProd[c] = covInv[c] * outclassCov;

        try { solver.compute(covProd[c]); }
        catch (...) { OV_ERROR_KRF("EigenSolver failed for condition [" << c + 1 << "]", Kernel::ErrorType::BadProcessing); }

        eigenValues[c]  = solver.eigenvalues().real();
        eigenVectors[c] = solver.eigenvectors().real();

        // Sort the vectors -_-
        std::vector<std::pair<double, int>> indexes;
        indexes.reserve(eigenValues[c].size());
        for (int i = 0; i < eigenValues[c].size(); ++i) { indexes.emplace_back(std::make_pair((eigenValues[c])[i], i)); }
        sort(indexes.begin(), indexes.end(), std::greater<std::pair<double, int>>());

        sortedEigenValues[c].resizeLike(eigenValues[c]);
        sortedEigenVectors[c].resizeLike(eigenVectors[c]);

        for (int i = 0; i < eigenValues[c].size(); ++i) {
            sortedEigenValues[c][i] = eigenValues[c][indexes[i].second];
            //@todo @FIXME This fonction work sometimes randomly
            sortedEigenVectors[c].col(i) = eigenVectors[c].col(indexes[i].second);
        }
    }
    return true;
}

bool CBoxAlgorithmRegularizedCSPTrainer::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    bool shouldTrain = false;
    uint64_t date    = 0, startTime = 0, endTime = 0;

    // Handle input stimulations
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isHeaderReceived()) {
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_stimDecoder.isBufferReceived()) {
            const Kernel::TParameterHandler<CStimulationSet*> stimSet(m_stimDecoder.getOutputStimulationSet());
            for (size_t j = 0; j < stimSet->size(); ++j) {
                if (stimSet->getId(j) == m_stimID) {
                    date        = stimSet->getDate(stimSet->size() - 1);
                    startTime   = boxContext.getInputChunkStartTime(0, i);
                    endTime     = boxContext.getInputChunkEndTime(0, i);
                    shouldTrain = true;
                    break;
                }
            }
        }
        if (m_stimDecoder.isEndReceived()) { m_encoder.encodeEnd(); }
    }

    // Update all covs with the current data chunks (if any)
    for (size_t i = 0; i < m_nClasses; ++i) { if (!updateCov(i)) { return false; } }

    if (shouldTrain) {
        this->getLogManager() << Kernel::LogLevel_Info << "Received train stimulation - be patient\n";

        const CMatrix* input   = m_signalDecoders[0].getOutputMatrix();
        const size_t nChannels = input->getDimensionSize(0);

        this->getLogManager() << Kernel::LogLevel_Debug << "Computing eigen vector decomposition...\n";

        // Get out the covariances
        std::vector<Eigen::MatrixXd> cov(m_nClasses);

        for (size_t i = 0; i < m_nClasses; ++i) {
            OV_ERROR_UNLESS_KRF(m_covProxies[i].nSamples >= 2,
                                "Invalid sample count of [" <<m_covProxies[i].nSamples << "] for condition number " << i << " (expected value > 2)",
                                Kernel::ErrorType::BadProcessing);

            Kernel::TParameterHandler<CMatrix*> op_cov(
                m_covProxies[i].cov->getOutputParameter(OnlineCovariance_OutputParameterId_CovarianceMatrix));

            // Get regularized cov
            m_covProxies[i].cov->activateInputTrigger(OnlineCovariance_Process_GetCov, true);
            OV_ERROR_UNLESS_KRF(m_covProxies[i].cov->process(), "Failed to retrieve regularized covariance", Kernel::ErrorType::Internal);

            const Eigen::Map<MatrixXdRowMajor> covMapper(op_cov->getBuffer(), nChannels, nChannels);
            cov[i] = covMapper;

            // Get vanilla cov
            m_covProxies[i].cov->activateInputTrigger(OnlineCovariance_Process_GetCovRaw, true);
            OV_ERROR_UNLESS_KRF(m_covProxies[i].cov->process(), "Failed to retrieve vanilla covariance", Kernel::ErrorType::Internal);
        }

        // Sanity check
        for (size_t i = 1; i < m_nClasses; ++i) {
            OV_ERROR_UNLESS_KRF(cov[i-1].rows() == cov[i].rows() && cov[i-1].cols() == cov[i].cols(),
                                "Mismatch between the number of channel in both input streams", Kernel::ErrorType::BadValue);
        }

        this->getLogManager() << Kernel::LogLevel_Info << "Data covariance dims are [" << (size_t)cov[0].rows() << "x" << (size_t)cov[0].cols()
                << "]. Number of samples per condition : \n";
        for (size_t i = 0; i < m_nClasses; ++i) {
            this->getLogManager() << Kernel::LogLevel_Info << "  cond " << i + 1 << " = " << m_covProxies[i].nBuffers << " chunks, sized "
                    << input->getDimensionSize(1) << " -> " << m_covProxies[i].nSamples << " samples\n";
            // this->getLogManager() << Kernel::LogLevel_Info << "Using shrinkage coeff " << m_Shrinkage << " ...\n";
        }

        // Compute the actual CSP using the obtained covariance matrices
        std::vector<Eigen::MatrixXd> sortedVectors;
        std::vector<Eigen::VectorXd> sortedValues;
        OV_ERROR_UNLESS_KRF(computeCSP(cov, sortedVectors, sortedValues), "Failure when computing CSP", Kernel::ErrorType::BadProcessing);

        // Create a CMatrix mapper that can spool the filters to a file
        CMatrix selectedVectors;
        selectedVectors.resize(m_filtersPerClass * m_nClasses, nChannels);

        Eigen::Map<MatrixXdRowMajor> selectedVectorsMapper(selectedVectors.getBuffer(), m_filtersPerClass * m_nClasses, nChannels);

        for (size_t c = 0; c < m_nClasses; ++c) {
            selectedVectorsMapper.block(c * m_filtersPerClass, 0, m_filtersPerClass, nChannels)
                    = sortedVectors[c].block(0, 0, nChannels, m_filtersPerClass).transpose();

            this->getLogManager() << Kernel::LogLevel_Info << "The " << m_filtersPerClass << " filter(s) for cond " << c + 1 << " cover "
                    << 100.0 * sortedValues[c].head(m_filtersPerClass).sum() / sortedValues[c].sum() << "% of corresp. eigenvalues\n";
        }

        if (m_saveAsBoxConf) {
            std::ofstream file;
            file.open(m_configFilename.toASCIIString(), std::ofstream::binary);
            OV_ERROR_UNLESS_KRF(file.is_open(), "Failed to open file located at [" << m_configFilename << "]", Kernel::ErrorType::BadFileRead);

            file << "<OpenViBE-SettingsOverride>\n";
            file << "\t<SettingValue>";
            const size_t n = m_filtersPerClass * m_nClasses * nChannels;
            for (size_t i = 0; i < n; ++i) {
                file << std::scientific << selectedVectors.getBuffer()[i] << " ";
            }
            file << "</SettingValue>\n";
            file << "\t<SettingValue>" << m_filtersPerClass * m_nClasses << "</SettingValue>\n";
            file << "\t<SettingValue>" << nChannels << "</SettingValue>\n";
            file << "\t<SettingValue></SettingValue>\n";
            file << "</OpenViBE-SettingsOverride>\n";

            file.close();
        }
        else {
            for (size_t i = 0; i < selectedVectors.getDimensionSize(0); ++i) {
                std::stringstream label;
                label << "Cond " << i / m_filtersPerClass + 1 << " filter " << i % m_filtersPerClass + 1;
                selectedVectors.setDimensionLabel(0, i, label.str().c_str());
            }

            OV_ERROR_UNLESS_KRF(Toolkit::Matrix::saveToTextFile(selectedVectors, m_configFilename, 10),
                                "Failed to save file to location [" << m_configFilename << "]", Kernel::ErrorType::BadFileWrite);
        }

        this->getLogManager() << Kernel::LogLevel_Info << "Regularized CSP Spatial filter trained successfully.\n";

        // Clean data, so if there's a new train stimulation, we'll start again.
        // @note possibly this should be a parameter in the future to allow incremental training
        for (auto& c : m_covProxies) {
            c.cov->activateInputTrigger(OnlineCovariance_Process_Reset, true);
            c.nSamples = 0;
            c.nBuffers = 0;
        }

        m_encoder.getInputStimulationSet()->clear();
        m_encoder.getInputStimulationSet()->push_back(OVTK_StimulationId_TrainCompleted, date, 0);
        m_encoder.encodeBuffer();

        boxContext.markOutputAsReadyToSend(0, startTime, endTime);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
