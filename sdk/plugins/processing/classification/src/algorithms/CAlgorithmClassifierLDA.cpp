///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierLDA.cpp
/// \brief Classes implementation for the Algorithm LDA.
/// \author Jussi T. Lindgren (Inria) / Guillaume Serriere (Loria).
/// \version 2.0.
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
#include "CAlgorithmClassifierLDA.hpp"

#include <sstream>
#include <iostream>
#include <algorithm>

#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>

#include "../algorithms/CAlgorithmConditionedCovariance.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const TYPE_NODE_NAME    = "LDA";
static const char* const CLASSES_NODE_NAME = "Classes";
//static const char* const COEFFICIENTS_NODE_NAME = "Weights";
//static const char* const BIAS_DISTANCE_NODE_NAME = "Bias-distance";
//static const char* const COEFFICIENT_PROBABILITY_NODE_NAME = "Coefficient-probability";
static const char* const COMPUTATION_HELPERS_CONFIGURATION_NODE = "Class-config-list";
static const char* const LDA_CONFIG_FILE_VERSION_ATTRIBUTE_NAME = "version";

int LDAClassificationCompare(CMatrix& first, CMatrix& second)
{
    //We first need to find the best classification of each.
    double* buffer        = first.getBuffer();
    const double maxFirst = *(std::max_element(buffer, buffer + first.getBufferElementCount()));

    buffer                 = second.getBuffer();
    const double maxSecond = *(std::max_element(buffer, buffer + second.getBufferElementCount()));

    //Then we just compared them
    if (OVFloatEqual(maxFirst, maxSecond)) { return 0; }
    if (maxFirst > maxSecond) { return -1; }
    return 1;
}


#define LDA_DEBUG 0
#if LDA_DEBUG
void CAlgorithmClassifierLDA::dumpMatrix(ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
{
    rMgr << Kernel::LogLevel_Info << desc << "\n";
    for (int i = 0 ; i < mat.rows() ; i++) 
    {
        rMgr << Kernel::LogLevel_Info << "Row " << i << ": ";
        for (int j = 0 ; j < mat.cols() ; j++) { rMgr << mat(i,j) << " "; }
        rMgr << "\n";
    }
}
#else
void CAlgorithmClassifierLDA::dumpMatrix(Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { }
#endif

bool CAlgorithmClassifierLDA::initialize()
{
    // Initialize the Conditioned Covariance Matrix algorithm
    m_covAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(Algorithm_ConditionedCovariance));

    OV_ERROR_UNLESS_KRF(m_covAlgorithm->initialize(), "Failed to initialize covariance algorithm", Kernel::ErrorType::Internal);

    // This is the weight parameter local to this module and automatically exposed to the GUI. Its redirected to the corresponding parameter of the cov alg.
    Kernel::TParameterHandler<double> ip_shrinkage(this->getInputParameter(ClassifierLDA_InputParameterId_Shrinkage));
    ip_shrinkage.setReferenceTarget(m_covAlgorithm->getInputParameter(ConditionedCovariance_InputParameterId_Shrinkage));

    Kernel::TParameterHandler<bool> ip_diagonalCov(this->getInputParameter(ClassifierLDA_InputParameterId_DiagonalCov));
    ip_diagonalCov = false;

    Kernel::TParameterHandler<XML::IXMLNode*> op_configuration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    op_configuration = nullptr;

    return CAlgorithmClassifier::initialize();
}

bool CAlgorithmClassifierLDA::uninitialize()
{
    OV_ERROR_UNLESS_KRF(m_covAlgorithm->uninitialize(), "Failed to uninitialize covariance algorithm", Kernel::ErrorType::Internal);
    this->getAlgorithmManager().releaseAlgorithm(*m_covAlgorithm);
    return CAlgorithmClassifier::uninitialize();
}

bool CAlgorithmClassifierLDA::train(const Toolkit::IFeatureVectorSet& dataset)
{
    OV_ERROR_UNLESS_KRF(this->initializeExtraParameterMechanism(), "Failed to unitialize extra parameters", Kernel::ErrorType::Internal);

    //We need to clear list because a instance of this class should support more that one training.
    m_labels.clear();
    m_discriminantFunctions.clear();

    const bool useShrinkage = this->getBooleanParameter(ClassifierLDA_InputParameterId_UseShrinkage);

    bool diagonalCov;
    if (useShrinkage) {
        this->getDoubleParameter(ClassifierLDA_InputParameterId_Shrinkage);
        diagonalCov = this->getBooleanParameter(ClassifierLDA_InputParameterId_DiagonalCov);
    }
    else {
        //If we don't use shrinkage we need to set lambda to 0.
        Kernel::TParameterHandler<double> ip_shrinkage(this->getInputParameter(ClassifierLDA_InputParameterId_Shrinkage));
        ip_shrinkage = 0.0;

        Kernel::TParameterHandler<bool> ip_diagonalCov(this->getInputParameter(ClassifierLDA_InputParameterId_DiagonalCov));
        ip_diagonalCov = false;
        diagonalCov    = false;
    }

    OV_ERROR_UNLESS_KRF(this->uninitializeExtraParameterMechanism(), "Failed to ininitialize extra parameters", Kernel::ErrorType::Internal);

    // IO to the covariance alg
    Kernel::TParameterHandler<CMatrix*> op_mean(m_covAlgorithm->getOutputParameter(ConditionedCovariance_OutputParameterId_Mean));
    Kernel::TParameterHandler<CMatrix*> op_covMatrix(m_covAlgorithm->getOutputParameter(ConditionedCovariance_OutputParameterId_CovarianceMatrix));
    Kernel::TParameterHandler<CMatrix*> ip_dataset(m_covAlgorithm->getInputParameter(ConditionedCovariance_InputParameterId_FeatureVectorSet));

    const size_t nRows = dataset.getFeatureVectorCount();
    const size_t nCols = (nRows > 0 ? dataset[0].getSize() : 0);
    this->getLogManager() << Kernel::LogLevel_Debug << "Feature set input dims [" << dataset.getFeatureVectorCount() << "x" << nCols << "]\n";

    OV_ERROR_UNLESS_KRF(nRows != 0 && nCols != 0, "Input data has a zero-size dimension, dims = [" << nRows << "x" << nCols << "]",
                        Kernel::ErrorType::BadInput);

    // The max amount of classes to be expected
    Kernel::TParameterHandler<uint64_t> ip_pNClasses(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_NClasses));
    m_nClasses = size_t(ip_pNClasses);

    // Count the classes actually present
    std::vector<size_t> nClasses;
    nClasses.resize(m_nClasses);

    for (size_t i = 0; i < dataset.getFeatureVectorCount(); ++i) {
        size_t classIdx = size_t(dataset[i].getLabel());
        nClasses[classIdx]++;
    }

    // Get class labels
    for (size_t i = 0; i < m_nClasses; ++i) {
        m_labels.push_back(double(i));
        m_discriminantFunctions.push_back(CAlgorithmLDADiscriminantFunction());
    }

    // Per-class means and a global covariance are used to form the LDA model
    std::vector<Eigen::MatrixXd> classMeans(m_nClasses);
    Eigen::MatrixXd globalCov = Eigen::MatrixXd::Zero(nCols, nCols);

    // We need the means per class
    for (size_t classIdx = 0; classIdx < m_nClasses; classIdx++) {
        if (nClasses[classIdx] > 0) {
            // const double label = m_labels[l_classIdx];
            const size_t examplesInClass = nClasses[classIdx];

            // Copy all the data of the class to a matrix
            CMatrix classData;
            classData.resize(examplesInClass, nCols);
            double* buffer = classData.getBuffer();
            for (size_t i = 0; i < nRows; ++i) {
                if (size_t(dataset[i].getLabel()) == classIdx) {
                    memcpy(buffer, dataset[i].getBuffer(), nCols * sizeof(double));
                    buffer += nCols;
                }
            }

            // Get the mean out of it
            Eigen::Map<MatrixXdRowMajor> dataMapper(classData.getBuffer(), examplesInClass, nCols);
            const Eigen::MatrixXd classMean = dataMapper.colwise().mean().transpose();
            classMeans[classIdx]            = classMean;
        }
        else {
            Eigen::MatrixXd tmp;
            tmp.resize(nCols, 1);
            tmp.setZero();
            classMeans[classIdx] = tmp;
        }
    }

    // We need a global covariance, use the regularized cov algorithm
    {
        ip_dataset->resize(nRows, nCols);
        double* buffer = ip_dataset->getBuffer();

        // Insert all data as the input of the cov algorithm
        for (size_t i = 0; i < nRows; ++i) {
            memcpy(buffer, dataset[i].getBuffer(), nCols * sizeof(double));
            buffer += nCols;
        }

        // Compute cov
        if (!m_covAlgorithm->process()) { OV_ERROR_KRF("Global covariance computation failed", Kernel::ErrorType::Internal); }

        // Get the results from the cov algorithm
        Eigen::Map<MatrixXdRowMajor> covMapper(op_covMatrix->getBuffer(), nCols, nCols);
        globalCov = covMapper;
    }

    //dumpMatrix(this->getLogManager(), mean[l_classIdx], "Mean");
    //dumpMatrix(this->getLogManager(), globalCov, "Shrinked cov");

    if (diagonalCov) {
        for (Eigen::Index i = 0; i < nCols; ++i) {
            for (Eigen::Index j = i + 1; j < nCols; ++j) {
                globalCov(i, j) = 0.0;
                globalCov(j, i) = 0.0;
            }
        }
    }

    // Get the pseudoinverse of the global cov using eigen decomposition for self-adjoint matrices
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver;
    solver.compute(globalCov);
    Eigen::VectorXd eigenValues = solver.eigenvalues();
    const double tolerance      = 1e-5*eigenValues(nCols-1); // Eigenvalues are sorted in increasing order.
    for (Eigen::Index i = 0; i < nCols; ++i) { if (eigenValues(i) >= tolerance) { eigenValues(i) = 1.0 / eigenValues(i); } }
    const Eigen::MatrixXd globalCovInv = solver.eigenvectors() * eigenValues.asDiagonal() * solver.eigenvectors().inverse();

    // const MatrixXd globalCovInv = globalCov.inverse();
    //We send the bias and the weight of each class to ComputationHelper
    for (size_t i = 0; i < getClassCount(); ++i) {
        const double examplesInClass = double(nClasses[i]);
        if (examplesInClass > 0) {
            const size_t totalExamples = dataset.getFeatureVectorCount();

            // This formula e.g. in Hastie, Tibshirani & Friedman: "Elements...", 2nd ed., p. 109
            const Eigen::VectorXd weigth = (globalCovInv * classMeans[i]);
            const Eigen::MatrixXd inter  = -0.5 * classMeans[i].transpose() * weigth;
            const double bias            = inter(0, 0) + std::log(examplesInClass / double(totalExamples));

            this->getLogManager() << Kernel::LogLevel_Debug << "Bias for " << i << " is " << bias << ", from " << examplesInClass / double(totalExamples)
                    << ", " << examplesInClass << "/" << totalExamples << ", int = " << inter(0, 0) << "\n";
            // dumpMatrix(this->getLogManager(), perClassMeans[i], "Means");

            m_discriminantFunctions[i].SetWeight(weigth);
            m_discriminantFunctions[i].SetBias(bias);
        }
        else { this->getLogManager() << Kernel::LogLevel_Debug << "Class " << i << " has no examples\n"; }
    }

    // Hack for classes with zero examples, give them valid models but such that will always lose
    size_t nonZeroClassIdx = 0;
    for (size_t i = 0; i < getClassCount(); ++i) {
        if (nClasses[i] > 0) {
            nonZeroClassIdx = i;
            break;
        }
    }
    for (size_t i = 0; i < getClassCount(); ++i) {
        if (nClasses[i] == 0) {
            m_discriminantFunctions[i].SetWeight(m_discriminantFunctions[nonZeroClassIdx].GetWeight());
            m_discriminantFunctions[i].SetBias(m_discriminantFunctions[nonZeroClassIdx].GetBias() - 1.0); // Will always lose to the orig
        }
    }

    m_nCols = nCols;

    // Debug output
    //dumpMatrix(this->getLogManager(), globalCov, "Global cov");
    //dumpMatrix(this->getLogManager(), eigenValues, "Eigenvalues");
    //dumpMatrix(this->getLogManager(), eigenSolver.eigenvectors(), "Eigenvectors");
    //dumpMatrix(this->getLogManager(), globalCovInv, "Global cov inverse");
    //dumpMatrix(this->getLogManager(), m_coefficients, "Hyperplane weights");

    return true;
}

bool CAlgorithmClassifierLDA::classify(const Toolkit::IFeatureVector& sample, double& classId, Toolkit::IVector& distance, Toolkit::IVector& probability)
{
    OV_ERROR_UNLESS_KRF(!m_discriminantFunctions.empty(), "this LDA classifier has not learned anything, so it cannot judge this sample. Might we humbly suggest training it again? Thank you most kindly.", Kernel::ErrorType::BadConfig);

    OV_ERROR_UNLESS_KRF(sample.getSize() == m_discriminantFunctions[0].GetNWeight(),
                        "Classifier expected " << m_discriminantFunctions[0].GetNWeight() << " features, got " << sample.getSize(),
                        Kernel::ErrorType::BadInput);

    const Eigen::Map<Eigen::VectorXd> featureVec(const_cast<double*>(sample.getBuffer()), sample.getSize());
    const Eigen::VectorXd weights = featureVec;
    const size_t nClass           = getClassCount();

    std::vector<double> buffer(nClass);
    std::vector<double> probabBuffer(nClass);
    //We ask for all computation helper to give the corresponding class value
    for (size_t i = 0; i < nClass; ++i) { buffer[i] = m_discriminantFunctions[i].GetValue(weights); }

    //p(Ck | x) = exp(ak) / sum[j](exp (aj))
    // with aj = (Weight for class j).transpose() * x + (Bias for class j)

    //Exponential can lead to nan results, so we reduce the computation and instead compute
    // p(Ck | x) = 1 / sum[j](exp(aj - ak))

    //All ak are given by computation helper
    errno = 0;
    for (size_t i = 0; i < nClass; ++i) {
        double expSum = 0.;
        for (size_t j = 0; j < nClass; ++j) { expSum += exp(buffer[j] - buffer[i]); }
        probabBuffer[i] = 1 / expSum;
        // std::cout << "p " << i << " = " << probabilityValue[i] << ", v=" << valueArray[i] << ", " << errno << "\n";
    }

    //Then we just find the highest probability and take it as a result
    const size_t classIdx = size_t(std::distance(buffer.begin(), std::max_element(buffer.begin(), buffer.end())));

    distance.setSize(nClass);
    probability.setSize(nClass);

    for (size_t i = 0; i < nClass; ++i) {
        distance[i]    = buffer[i];
        probability[i] = probabBuffer[i];
    }

    classId = m_labels[classIdx];

    return true;
}

XML::IXMLNode* CAlgorithmClassifierLDA::saveConfig()
{
    XML::IXMLNode* algorithmNode = XML::createNode(TYPE_NODE_NAME);
    algorithmNode->addAttribute(LDA_CONFIG_FILE_VERSION_ATTRIBUTE_NAME, "1");

    // Write the classifier to an .xml
    std::stringstream classes;

    for (size_t i = 0; i < getClassCount(); ++i) { classes << m_labels[i] << " "; }

    //Only new version should be recorded so we don't need to test
    XML::IXMLNode* helpersConfig = XML::createNode(COMPUTATION_HELPERS_CONFIGURATION_NODE);
    for (size_t i = 0; i < m_discriminantFunctions.size(); ++i) { helpersConfig->addChild(m_discriminantFunctions[i].GetConfiguration()); }

    XML::IXMLNode* tmpNode = XML::createNode(CLASSES_NODE_NAME);
    tmpNode->setPCData(classes.str().c_str());
    algorithmNode->addChild(tmpNode);
    algorithmNode->addChild(helpersConfig);

    return algorithmNode;
}


//Extract a double from the PCDATA of a node
double getFloatFromNode(const XML::IXMLNode* node)
{
    std::stringstream ss(node->getPCData());
    double res;
    ss >> res;
    return res;
}

bool CAlgorithmClassifierLDA::loadConfig(XML::IXMLNode* node)
{
    OV_ERROR_UNLESS_KRF(node->hasAttribute(LDA_CONFIG_FILE_VERSION_ATTRIBUTE_NAME),
                        "this classifier was trained with an older version of LDA, which we can no longer read. Might we humbly suggest training it again? Thank you most kindly.", Kernel::ErrorType::BadConfig);

    m_labels.clear();
    m_discriminantFunctions.clear();

    XML::IXMLNode* tmpNode = node->getChildByName(CLASSES_NODE_NAME);

    OV_ERROR_UNLESS_KRF(tmpNode != nullptr, "Failed to retrieve xml node", Kernel::ErrorType::BadParsing);

    loadClassesFromNode(tmpNode);


    //We send corresponding data to the computation helper
    const XML::IXMLNode* configsNode = node->getChildByName(COMPUTATION_HELPERS_CONFIGURATION_NODE);

    for (size_t i = 0; i < configsNode->getChildCount(); ++i) {
        m_discriminantFunctions.push_back(CAlgorithmLDADiscriminantFunction());
        m_discriminantFunctions[i].LoadConfig(configsNode->getChild(i));
    }

    return true;
}

void CAlgorithmClassifierLDA::loadClassesFromNode(const XML::IXMLNode* node)
{
    std::stringstream ss(node->getPCData());
    double value;
    while (ss >> value) { m_labels.push_back(value); }
    m_nClasses = m_labels.size();
}

//Load the weight vector
void CAlgorithmClassifierLDA::loadCoefsFromNode(const XML::IXMLNode* node)
{
    std::stringstream ss(node->getPCData());

    std::vector<double> coefs;
    double value;
    while (ss >> value) { coefs.push_back(value); }

    m_weights.resize(1, coefs.size());
    m_nCols = coefs.size();
    for (size_t i = 0; i < coefs.size(); ++i) { m_weights(0, i) = coefs[i]; }
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
