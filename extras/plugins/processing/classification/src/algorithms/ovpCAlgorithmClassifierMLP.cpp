#include "ovpCAlgorithmClassifierMLP.h"
#include "../ovp_defines.h"

#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include <Eigen/Dense>
#include <Eigen/Core>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const MLP_EVALUATION_FUNCTION_NAME = "Evaluation function";
static const char* const MLP_TYPE_NODE_NAME                = "MLP";
static const char* const MLP_NEURON_CONFIG_NODE_NAME       = "Neuron-configuration";
static const char* const MLP_INPUT_NEURON_COUNT_NODE_NAME  = "Input-neuron-count";
static const char* const MLP_HIDDEN_NEURON_COUNT_NODE_NAME = "Hidden-neuron-count";
static const char* const MLP_MAX_NODE_NAME                 = "Maximum";
static const char* const MLP_MIN_NODE_NAME                 = "Minimum";
static const char* const MLP_INPUT_BIAS_NODE_NAME          = "Input-bias";
static const char* const MLP_INPUT_WEIGHT_NODE_NAME        = "Input-weight";
static const char* const MLP_HIDDEN_BIAS_NODE_NAME         = "Hidden-bias";
static const char* const MLP_HIDDEN_WEIGHT_NODE_NAME       = "Hidden-weight";
static const char* const MLP_CLASS_LABEL_NODE_NAME         = "Class-label";

int MLPClassificationCompare(CMatrix& first, CMatrix& second)
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

#define MLP_DEBUG 0
#if MLP_DEBUG
void dumpMatrix(Kernel::ILogManager& rMgr, const MatrixXd& mat, const CString& desc)
{
    rMgr << Kernel::LogLevel_Info << desc << "\n";
    for (int i = 0; i < mat.rows(); ++i) {
        rMgr << Kernel::LogLevel_Info << "Row " << i << ": ";
        for (int j = 0; j < mat.cols(); ++j) {
            rMgr << mat(i, j) << " ";
        }
        rMgr << "\n";
    }
}
#else
void dumpMatrix(Kernel::ILogManager& /*rMgr*/, const Eigen::MatrixXd& /*mat*/, const CString& /*desc*/) { }
#endif


bool CAlgorithmClassifierMLP::initialize()
{
    Kernel::TParameterHandler<int64_t> iHidden(this->getInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount));
    iHidden = 3;

    Kernel::TParameterHandler<XML::IXMLNode*> config(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    config = nullptr;

    Kernel::TParameterHandler<double> iAlpha(this->getInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha));
    iAlpha = 0.01;
    Kernel::TParameterHandler<double> iEpsilon(this->getInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon));
    iEpsilon = 0.000001;
    return true;
}

bool CAlgorithmClassifierMLP::uninitialize() { return true; }

bool CAlgorithmClassifierMLP::train(const Toolkit::IFeatureVectorSet& dataset)
{
    m_labels.clear();

    this->initializeExtraParameterMechanism();
    Eigen::Index hiddenNeuronCount = Eigen::Index(this->getInt64Parameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount));
    double alpha                   = this->getDoubleParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha);
    double epsilon                 = this->getDoubleParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon);
    this->uninitializeExtraParameterMechanism();

    if (hiddenNeuronCount < 1) {
        this->getLogManager() << Kernel::LogLevel_Error << "Invalid amount of neuron in the hidden layer. Fallback to default value (3)\n";
        hiddenNeuronCount = 3;
    }
    if (alpha <= 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Invalid value for learning coefficient (" << alpha << "). Fallback to default value (0.01)\n";
        alpha = 0.01;
    }
    if (epsilon <= 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Invalid value for stop learning condition (" << epsilon <<
                "). Fallback to default value (0.000001)\n";
        epsilon = 0.000001;
    }

    std::map<double, size_t> classCount;
    std::map<double, Eigen::VectorXd> targetList;
    //We need to compute the min and the max of data in order to normalize and center them
    for (size_t i = 0; i < dataset.getFeatureVectorCount(); ++i) { classCount[dataset[i].getLabel()]++; }
    size_t validationElementCount = 0;

    //We generate the list of class
    for (auto iter = classCount.begin(); iter != classCount.end(); ++iter) {
        //We keep 20% percent of the training set for the validation for each class
        validationElementCount += size_t(double(iter->second) * 0.2);
        m_labels.push_back(iter->first);
        iter->second = size_t(double(iter->second) * 0.2);
    }

    const Eigen::Index nbClass  = Eigen::Index(m_labels.size());
    const Eigen::Index nFeature = Eigen::Index(dataset.getFeatureVector(0).getSize());

    //Generate the target vector for each class. To save time and memory, we compute only one vector per class
    //Vector tagret looks like following [0 0 1 0] for class 3 (if 4 classes)
    for (Eigen::Index i = 0; i < nbClass; ++i) {
        Eigen::VectorXd oTarget = Eigen::VectorXd::Zero(nbClass);
        //class 1 is at index 0
        oTarget[Eigen::Index(m_labels[i])] = 1.;
        targetList[m_labels[i]]            = oTarget;
    }

    //We store each normalize vector we get for training. This not optimal in memory but avoid a lot of computation later
    //List of the class of the feature vectors store in the same order are they are in validation/training set(to be able to get the target)
    std::vector<double> oTrainingSet;
    std::vector<double> oValidationSet;
    Eigen::MatrixXd oTrainingDataMatrix(nFeature, dataset.getFeatureVectorCount() - validationElementCount);
    Eigen::MatrixXd oValidationDataMatrix(nFeature, validationElementCount);

    //We don't need to make a shuffle it has already be made by the trainer box
    //We store 20% of the feature vectors for validation
    int validationIndex = 0, trainingIndex = 0;
    for (size_t i = 0; i < dataset.getFeatureVectorCount(); ++i) {
        const Eigen::Map<Eigen::VectorXd> oFeatureVec(const_cast<double*>(dataset.getFeatureVector(i).getBuffer()), Eigen::Index(nFeature));
        Eigen::VectorXd oData = oFeatureVec;
        if (classCount[dataset.getFeatureVector(i).getLabel()] > 0) {
            oValidationDataMatrix.col(validationIndex++) = oData;
            oValidationSet.push_back(dataset.getFeatureVector(i).getLabel());
            --classCount[dataset.getFeatureVector(i).getLabel()];
        }
        else {
            oTrainingDataMatrix.col(trainingIndex++) = oData;
            oTrainingSet.push_back(dataset.getFeatureVector(i).getLabel());
        }
    }

    //We now get the min and the max of the training set for normalization
    m_max = oTrainingDataMatrix.maxCoeff();
    m_min = oTrainingDataMatrix.minCoeff();
    //Normalization of the data. We need to do it to avoid saturation of tanh.
    for (Eigen::Index i = 0; i < oTrainingDataMatrix.cols(); ++i) {
        for (Eigen::Index j = 0; j < oTrainingDataMatrix.rows(); ++j) {
            oTrainingDataMatrix(j, i) = 2 * (oTrainingDataMatrix(j, i) - m_min) / (m_max - m_min) - 1;
        }
    }
    for (Eigen::Index i = 0; i < oValidationDataMatrix.cols(); ++i) {
        for (Eigen::Index j = 0; j < oValidationDataMatrix.rows(); ++j) {
            oValidationDataMatrix(j, i) = 2 * (oValidationDataMatrix(j, i) - m_min) / (m_max - m_min) - 1;
        }
    }

    const double featureCount = double(oTrainingSet.size());
    const double boundValue   = 1.0 / double(nFeature + 1);
    double previousError      = std::numeric_limits<double>::max();
    double cumulativeError    = 0;

    //Let's generate randomly weights and biases
    //We restrain the weight between -1/(fan-in) and 1/(fan-in) to avoid saturation in the worst case
    m_inputWeight = Eigen::MatrixXd::Random(hiddenNeuronCount, nFeature) * boundValue;
    m_inputBias   = Eigen::VectorXd::Random(hiddenNeuronCount) * boundValue;

    m_hiddenWeight = Eigen::MatrixXd::Random(nbClass, hiddenNeuronCount) * boundValue;
    m_hiddenBias   = Eigen::VectorXd::Random(nbClass) * boundValue;

    Eigen::MatrixXd oDeltaInputWeight  = Eigen::MatrixXd::Zero(hiddenNeuronCount, nFeature);
    Eigen::VectorXd oDeltaInputBias    = Eigen::VectorXd::Zero(hiddenNeuronCount);
    Eigen::MatrixXd oDeltaHiddenWeight = Eigen::MatrixXd::Zero(nbClass, hiddenNeuronCount);
    Eigen::VectorXd oDeltaHiddenBias   = Eigen::VectorXd::Zero(nbClass);

    Eigen::MatrixXd oY1, oA2;
    //A1 is the value compute in hidden neuron before applying tanh
    //Y1 is the output vector of hidden layer
    //A2 is the value compute by output neuron before applying transfer function
    //Y2 is the value of output after the transfer function (softmax)
    while (true) {
        oDeltaInputWeight.setZero();
        oDeltaInputBias.setZero();
        oDeltaHiddenWeight.setZero();
        oDeltaHiddenBias.setZero();
        //The first cast of tanh has to been explicit for windows compilation
        oY1.noalias() = ((m_inputWeight * oTrainingDataMatrix).colwise() + m_inputBias).unaryExpr(
            [](double ele) { return tanh(ele); });
        oA2.noalias() = (m_hiddenWeight * oY1).colwise() + m_hiddenBias;
        for (Eigen::Index i = 0; i < Eigen::Index(featureCount); ++i) {
            const Eigen::VectorXd& oTarget = targetList[oTrainingSet[i]];
            const Eigen::VectorXd& oData   = oTrainingDataMatrix.col(i);

            //Now we compute all deltas of output layer
            Eigen::VectorXd oOutputDelta = oA2.col(i) - oTarget;
            for (Eigen::Index j = 0; j < nbClass; ++j) {
                for (Eigen::Index k = 0; k < hiddenNeuronCount; ++k) { oDeltaHiddenWeight(j, k) -= oOutputDelta[j] * oY1.col(i)[k]; }
            }
            oDeltaHiddenBias.noalias() -= oOutputDelta;

            //Now we take care of the hidden layer
            Eigen::VectorXd oHiddenDelta = Eigen::VectorXd::Zero(hiddenNeuronCount);
            for (Eigen::Index j = 0; j < hiddenNeuronCount; ++j) {
                for (Eigen::Index k = 0; k < nbClass; ++k) { oHiddenDelta[j] += oOutputDelta[k] * m_hiddenWeight(k, j); }
                oHiddenDelta[j] *= (1 - pow(oY1.col(i)[j], 2));
            }

            for (Eigen::Index j = 0; j < hiddenNeuronCount; ++j) {
                for (Eigen::Index k = 0; k < nFeature; ++k) { oDeltaInputWeight(j, k) -= oHiddenDelta[j] * oData[k]; }
            }
            oDeltaInputBias.noalias() -= oHiddenDelta;
        }
        //We finish the loop, let's apply deltas
        m_hiddenWeight.noalias() += oDeltaHiddenWeight / featureCount * alpha;
        m_hiddenBias.noalias() += oDeltaHiddenBias / featureCount * alpha;
        m_inputWeight.noalias() += oDeltaInputWeight / featureCount * alpha;
        m_inputBias.noalias() += oDeltaInputBias / featureCount * alpha;

        dumpMatrix(this->getLogManager(), m_hiddenWeight, "m_hiddenWeight");
        dumpMatrix(this->getLogManager(), m_hiddenBias, "m_hiddenBias");
        dumpMatrix(this->getLogManager(), m_inputWeight, "m_inputWeight");
        dumpMatrix(this->getLogManager(), m_inputBias, "m_inputBias");

        //Now we compute the cumulative error in the validation set
        cumulativeError = 0;
        //We don't compute Y2 because we train on the identity
        oA2.noalias() = (m_hiddenWeight * ((m_inputWeight * oValidationDataMatrix).colwise() + m_inputBias).unaryExpr( [](double ele) { return tanh(ele); })).
                        colwise() + m_hiddenBias;
        for (Eigen::Index i = 0; i < Eigen::Index(oValidationSet.size()); ++i) {
            const Eigen::VectorXd& oTarget         = targetList[oValidationSet[i]];
            const Eigen::VectorXd& oIdentityResult = oA2.col(i);

            //Now we need to compute the error
            for (Eigen::Index j = 0; j < nbClass; ++j) { cumulativeError += 0.5 * pow(oIdentityResult[j] - oTarget[j], 2); }
        }
        cumulativeError /= double(oValidationSet.size());
        //If the delta of error is under Epsilon we consider that the training is over
        if (previousError - cumulativeError < epsilon) { break; }
        previousError = cumulativeError;
    }
    dumpMatrix(this->getLogManager(), m_hiddenWeight, "oHiddenWeight");
    dumpMatrix(this->getLogManager(), m_hiddenBias, "oHiddenBias");
    dumpMatrix(this->getLogManager(), m_inputWeight, "oInputWeight");
    dumpMatrix(this->getLogManager(), m_inputBias, "oInputBias");
    return true;
}

bool CAlgorithmClassifierMLP::classify(const Toolkit::IFeatureVector& sample, double& classLabel, Toolkit::IVector& distance, Toolkit::IVector& probability)
{
    if (sample.getSize() != size_t(m_inputWeight.cols())) {
        this->getLogManager() << Kernel::LogLevel_Error << "Classifier expected " << size_t(m_inputWeight.cols()) << " features, got " << sample.getSize()
                << "\n";
        return false;
    }

    const Eigen::Map<Eigen::VectorXd> oFeatureVec(const_cast<double*>(sample.getBuffer()), Eigen::Index(sample.getSize()));
    Eigen::VectorXd oData = oFeatureVec;
    //we normalize and center data on 0 to avoid saturation
    for (Eigen::Index j = 0; j < Eigen::Index(sample.getSize()); ++j) { oData[j] = 2 * (oData[j] - m_min) / (m_max - m_min) - 1; }

    const size_t classCount = m_labels.size();

    Eigen::VectorXd oA2 = m_hiddenBias + (m_hiddenWeight * (m_inputBias + (m_inputWeight * oData)).unaryExpr([](double ele) { return tanh(ele); }));

    //The final transfer function is the softmax
    Eigen::VectorXd oY2 = oA2.unaryExpr([](double ele) { return exp(ele); });
    oY2 /= oY2.sum();

    distance.setSize(classCount);
    probability.setSize(classCount);

    //We use A2 as the classification values output, and the Y2 as the probability
    double max        = oY2[0];
    size_t classFound = 0;
    distance[0]       = oA2[0];
    probability[0]    = oY2[0];
    for (Eigen::Index i = 1; i < Eigen::Index(classCount); ++i) {
        if (oY2[i] > max) {
            max        = oY2[i];
            classFound = i;
        }
        distance[i]    = oA2[i];
        probability[i] = oY2[i];
    }

    classLabel = m_labels[classFound];

    return true;
}

XML::IXMLNode* CAlgorithmClassifierMLP::saveConfig()
{
    XML::IXMLNode* rootNode = XML::createNode(MLP_TYPE_NODE_NAME);

    std::stringstream classes;
    for (int i = 0; i < m_hiddenBias.size(); ++i) { classes << m_labels[i] << " "; }
    XML::IXMLNode* classLabelNode = XML::createNode(MLP_CLASS_LABEL_NODE_NAME);
    classLabelNode->setPCData(classes.str().c_str());
    rootNode->addChild(classLabelNode);

    XML::IXMLNode* configuration = XML::createNode(MLP_NEURON_CONFIG_NODE_NAME);

    //The input and output neuron count are not mandatory but they facilitate a lot the loading process
    XML::IXMLNode* tempNode = XML::createNode(MLP_INPUT_NEURON_COUNT_NODE_NAME);
    dumpData(tempNode, int64_t(m_inputWeight.cols()));
    configuration->addChild(tempNode);

    tempNode = XML::createNode(MLP_HIDDEN_NEURON_COUNT_NODE_NAME);
    dumpData(tempNode, int64_t(m_inputWeight.rows()));
    configuration->addChild(tempNode);
    rootNode->addChild(configuration);

    tempNode = XML::createNode(MLP_MIN_NODE_NAME);
    dumpData(tempNode, m_min);
    rootNode->addChild(tempNode);

    tempNode = XML::createNode(MLP_MAX_NODE_NAME);
    dumpData(tempNode, m_max);
    rootNode->addChild(tempNode);

    tempNode = XML::createNode(MLP_INPUT_WEIGHT_NODE_NAME);
    dumpData(tempNode, m_inputWeight);
    rootNode->addChild(tempNode);

    tempNode = XML::createNode(MLP_INPUT_BIAS_NODE_NAME);
    dumpData(tempNode, m_inputBias);
    rootNode->addChild(tempNode);

    tempNode = XML::createNode(MLP_HIDDEN_BIAS_NODE_NAME);
    dumpData(tempNode, m_hiddenBias);
    rootNode->addChild(tempNode);

    tempNode = XML::createNode(MLP_HIDDEN_WEIGHT_NODE_NAME);
    dumpData(tempNode, m_hiddenWeight);
    rootNode->addChild(tempNode);

    return rootNode;
}

bool CAlgorithmClassifierMLP::loadConfig(XML::IXMLNode* configNode)
{
    m_labels.clear();
    std::stringstream data(configNode->getChildByName(MLP_CLASS_LABEL_NODE_NAME)->getPCData());
    double temp;
    while (data >> temp) { m_labels.push_back(temp); }

    int64_t featureSize, hiddenNeuronCount;
    const XML::IXMLNode* neuronConfigNode = configNode->getChildByName(MLP_NEURON_CONFIG_NODE_NAME);

    loadData(neuronConfigNode->getChildByName(MLP_HIDDEN_NEURON_COUNT_NODE_NAME), hiddenNeuronCount);
    loadData(neuronConfigNode->getChildByName(MLP_INPUT_NEURON_COUNT_NODE_NAME), featureSize);

    loadData(configNode->getChildByName(MLP_MAX_NODE_NAME), m_max);
    loadData(configNode->getChildByName(MLP_MIN_NODE_NAME), m_min);

    loadData(configNode->getChildByName(MLP_INPUT_WEIGHT_NODE_NAME), m_inputWeight, hiddenNeuronCount, featureSize);
    loadData(configNode->getChildByName(MLP_INPUT_BIAS_NODE_NAME), m_inputBias);
    loadData(configNode->getChildByName(MLP_HIDDEN_WEIGHT_NODE_NAME), m_hiddenWeight, m_labels.size(), hiddenNeuronCount);
    loadData(configNode->getChildByName(MLP_HIDDEN_BIAS_NODE_NAME), m_hiddenBias);

    return true;
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode* node, Eigen::MatrixXd& matrix)
{
    std::stringstream data;

    data << std::scientific;
    for (Eigen::Index i = 0; i < matrix.rows(); ++i) { for (Eigen::Index j = 0; j < matrix.cols(); ++j) { data << " " << matrix(i, j); } }

    node->setPCData(data.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode* node, Eigen::VectorXd& vector)
{
    std::stringstream data;

    data << std::scientific;
    for (Eigen::Index i = 0; i < vector.size(); ++i) { data << " " << vector[i]; }

    node->setPCData(data.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode* node, const int64_t value)
{
    std::stringstream data;
    data << value;
    node->setPCData(data.str().c_str());
}

void CAlgorithmClassifierMLP::dumpData(XML::IXMLNode* node, const double value)
{
    std::stringstream data;
    data << std::scientific;
    data << value;
    node->setPCData(data.str().c_str());
}

void CAlgorithmClassifierMLP::loadData(const XML::IXMLNode* node, Eigen::MatrixXd& matrix, const size_t nRow, const size_t nCol)
{
    matrix = Eigen::MatrixXd(nRow, nCol);
    std::stringstream data(node->getPCData());

    std::vector<double> coefs;
    double value;
    while (data >> value) { coefs.push_back(value); }

    size_t index = 0;
    for (size_t i = 0; i < nRow; ++i) {
        for (size_t j = 0; j < nCol; ++j) {
            matrix(int(i), int(j)) = coefs[index];
            ++index;
        }
    }
}

void CAlgorithmClassifierMLP::loadData(const XML::IXMLNode* node, Eigen::VectorXd& vector)
{
    std::stringstream data(node->getPCData());
    std::vector<double> coefs;
    double value;
    while (data >> value) { coefs.push_back(value); }
    vector = Eigen::VectorXd(coefs.size());

    for (Eigen::Index i = 0; i < Eigen::Index(coefs.size()); ++i) { vector[i] = coefs[i]; }
}

void CAlgorithmClassifierMLP::loadData(const XML::IXMLNode* node, int64_t& value)
{
    std::stringstream data(node->getPCData());
    data >> value;
}

void CAlgorithmClassifierMLP::loadData(const XML::IXMLNode* node, double& value)
{
    std::stringstream data(node->getPCData());
    data >> value;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
