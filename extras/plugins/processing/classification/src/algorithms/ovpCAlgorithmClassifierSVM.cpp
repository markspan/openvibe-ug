#include "../ovp_defines.h"

#include "ovpCAlgorithmClassifierSVM.h"

#include <sstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <cfloat>   // DBL_EPSILON

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const TYPE_NODE_NAME        = "SVM";
static const char* const PARAM_NODE_NAME       = "Param";
static const char* const SVM_TYPE_NODE_NAME    = "svm_type";
static const char* const KERNEL_TYPE_NODE_NAME = "kernel_type";
static const char* const DEGREE_NODE_NAME      = "degree";
static const char* const GAMMA_NODE_NAME       = "gamma";
static const char* const COEF0_NODE_NAME       = "coef0";
static const char* const MODEL_NODE_NAME       = "Model";
static const char* const NR_CLASS_NODE_NAME    = "nr_class";
static const char* const TOTAL_SV_NODE_NAME    = "total_sv";
static const char* const RHO_NODE_NAME         = "rho";
static const char* const LABEL_NODE_NAME       = "label";
static const char* const PROB_A_NODE_NAME      = "probA";
static const char* const PROB_B_NODE_NAME      = "probB";
static const char* const NR_SV_NODE_NAME       = "nr_sv";
static const char* const SVS_NODE_NAME         = "SVs";
static const char* const SV_NODE_NAME          = "SV";
static const char* const COEF_NODE_NAME        = "coef";
static const char* const VALUE_NODE_NAME       = "value";

int SVMClassificationCompare(CMatrix& first, CMatrix& second)
{
    if (OVFloatEqual(std::fabs(first[0]), std::fabs(second[0]))) { return 0; }
    if (std::fabs(first[0]) > std::fabs(second[0])) { return -1; }
    return 1;
}

bool CAlgorithmClassifierSVM::initialize()
{
    Kernel::TParameterHandler<int64_t> iSVMType(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMType));
    Kernel::TParameterHandler<int64_t> iSVMKernelType(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMKernelType));
    Kernel::TParameterHandler<int64_t> iDegree(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMDegree));
    Kernel::TParameterHandler<double> iGamma(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMGamma));
    Kernel::TParameterHandler<double> iCoef0(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMCoef0));
    Kernel::TParameterHandler<double> iCost(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCost));
    Kernel::TParameterHandler<double> iNu(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMNu));
    Kernel::TParameterHandler<double> iEpsilon(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMEpsilon));
    Kernel::TParameterHandler<double> iCacheSize(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCacheSize));
    Kernel::TParameterHandler<double> iEpsilonTolerance(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMTolerance));
    Kernel::TParameterHandler<bool> iShrinking(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking));
    //TParameterHandler < bool > iProbabilityEstimate(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMProbabilityEstimate));
    Kernel::TParameterHandler<CString*> ip_weight(this->getInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMweight));
    Kernel::TParameterHandler<CString*> ip_weightLabel(this->getInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMWeightLabel));

    iSVMType          = C_SVC;
    iSVMKernelType    = LINEAR;
    iDegree           = 3;
    iGamma            = 0;
    iCoef0            = 0;
    iCost             = 1;
    iNu               = 0.5;
    iEpsilon          = 0.1;
    iCacheSize        = 100;
    iEpsilonTolerance = 0.001;
    iShrinking        = true;
    //iProbabilityEstimate=true;
    *ip_weight      = "";
    *ip_weightLabel = "";

    Kernel::TParameterHandler<XML::IXMLNode*> config(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    config   = nullptr;
    m_prob.y = nullptr;
    m_prob.x = nullptr;

    m_param.weight       = nullptr;
    m_param.weight_label = nullptr;

    m_model           = nullptr;
    m_modelWasTrained = false;

    return CAlgorithmClassifier::initialize();
}

bool CAlgorithmClassifierSVM::uninitialize()
{
    if (m_prob.x != nullptr && m_prob.y != nullptr) {
        for (size_t i = 0; i < size_t(m_prob.l); ++i) { delete[] m_prob.x[i]; }
        delete[] m_prob.y;
        delete[] m_prob.x;
        m_prob.y = nullptr;
        m_prob.x = nullptr;
    }

    if (m_param.weight != nullptr) {
        delete[] m_param.weight;
        m_param.weight = nullptr;
    }

    if (m_param.weight_label != nullptr) {
        delete[] m_param.weight_label;
        m_param.weight_label = nullptr;
    }

    deleteModel(m_model, !m_modelWasTrained);
    m_model           = nullptr;
    m_modelWasTrained = false;

    return CAlgorithmClassifier::uninitialize();
}

void CAlgorithmClassifierSVM::deleteModel(svm_model* model, const bool freeSupportVectors)
{
    if (model != nullptr) {
        delete[] model->rho;
        delete[] model->probA;
        delete[] model->probB;
        delete[] model->label;
        delete[] model->nSV;

        for (size_t i = 0; i < size_t(model->nr_class - 1); ++i) { delete[] model->sv_coef[i]; }
        delete[] model->sv_coef;

        // We need the following depending on how the model was allocated. If we got it from svm_train,
        // the support vectors are pointers to the problem structure which is freed elsewhere. 
        // If we loaded the model from disk, we allocated the vectors separately.
        if (freeSupportVectors) { for (size_t i = 0; i < size_t(model->l); ++i) { delete[] model->SV[i]; } }
        delete[] model->SV;

        delete model;
        model = nullptr;
    }
}

void CAlgorithmClassifierSVM::setParameter()
{
    this->initializeExtraParameterMechanism();

    m_param.svm_type    = int(this->getEnumerationParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMType, OVP_TypeId_SVMType));
    m_param.kernel_type = int(this->getEnumerationParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMKernelType, OVP_TypeId_SVMKernelType));
    m_param.degree      = int(this->getInt64Parameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMDegree));
    m_param.gamma       = this->getDoubleParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMGamma);
    m_param.coef0       = this->getDoubleParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMCoef0);
    m_param.C           = this->getDoubleParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCost);
    m_param.nu          = this->getDoubleParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMNu);
    m_param.p           = this->getDoubleParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMEpsilon);
    m_param.cache_size  = this->getDoubleParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCacheSize);
    m_param.eps         = this->getDoubleParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMTolerance);
    m_param.shrinking   = int(this->getBooleanParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking));
    //  m_param.probability        = this->getBooleanParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking);
    m_param.probability            = 1;
    const CString paramWeight      = *this->getCStringParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMweight);
    const CString paramWeightLabel = *this->getCStringParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMWeightLabel);

    this->uninitializeExtraParameterMechanism();

    std::vector<double> weights;
    std::stringstream ssWeight(paramWeight.toASCIIString());
    double value;
    while (ssWeight >> value) { weights.push_back(value); }

    m_param.nr_weight = int(weights.size());
    double* weight    = new double[weights.size()];
    for (uint32_t i = 0; i < weights.size(); ++i) { weight[i] = weights[i]; }
    m_param.weight = weight;//nullptr;

    std::vector<int64_t> labels;
    std::stringstream ssLabel(paramWeightLabel.toASCIIString());
    int64_t iValue;
    while (ssLabel >> iValue) { labels.push_back(iValue); }

    //the number of weight label need to be equal to the number of weight
    while (labels.size() < weights.size()) { labels.push_back(int64_t(labels.size() + 1)); }

    int* label = new int[weights.size()];
    for (size_t i = 0; i < weights.size(); ++i) { label[i] = int(labels[i]); }
    m_param.weight_label = label;//nullptr;
}

bool CAlgorithmClassifierSVM::train(const Toolkit::IFeatureVectorSet& dataset)
{
    if (m_prob.x != nullptr && m_prob.y != nullptr) {
        for (size_t i = 0; i < size_t(m_prob.l); ++i) { delete[] m_prob.x[i]; }
        delete[] m_prob.y;
        delete[] m_prob.x;
        m_prob.y = nullptr;
        m_prob.x = nullptr;
    }
    // default Param values
    //std::cout<<"param config"<<std::endl;
    this->setParameter();
    this->getLogManager() << Kernel::LogLevel_Trace << paramToString(&m_param);

    //configure m_prob
    //std::cout<<"prob config"<<std::endl;
    m_prob.l    = int(dataset.getFeatureVectorCount());
    m_nFeatures = dataset[0].getSize();

    m_prob.y = new double[m_prob.l];
    m_prob.x = new svm_node*[m_prob.l];

    //std::cout<< "number vector:"<<l_oProb.l<<" size of vector:"<<m_nFeatures<<std::endl;

    for (size_t i = 0; i < size_t(m_prob.l); ++i) {
        m_prob.x[i] = new svm_node[m_nFeatures + 1];
        m_prob.y[i] = dataset[i].getLabel();
        for (size_t j = 0; j < m_nFeatures; ++j) {
            m_prob.x[i][j].index = int(j + 1);
            m_prob.x[i][j].value = dataset[i].getBuffer()[j];
        }
        m_prob.x[i][m_nFeatures].index = -1;
    }

    // Gamma of zero is interpreted as a request for automatic selection
    if (std::fabs(m_param.gamma) <= DBL_EPSILON) { m_param.gamma = 1.0 / (m_nFeatures > 0 ? double(m_nFeatures) : 1.0); }

    if (m_param.kernel_type == PRECOMPUTED) {
        for (size_t i = 0; i < size_t(m_prob.l); ++i) {
            if (m_prob.x[i][0].index != 0) {
                this->getLogManager() << Kernel::LogLevel_Error << "Wrong input format: first column must be 0:sample_serial_number\n";
                return false;
            }
            if (m_prob.x[i][0].value <= 0 || m_prob.x[i][0].value > double(m_nFeatures)) {
                this->getLogManager() << Kernel::LogLevel_Error << "Wrong input format: sample_serial_number out of range\n";
                return false;
            }
        }
    }

    this->getLogManager() << Kernel::LogLevel_Trace << problemToString(&m_prob);

    //make a model
    //std::cout<<"svm_train"<<std::endl;
    if (m_model != nullptr) {
        //std::cout<<"delete model"<<std::endl;
        deleteModel(m_model, !m_modelWasTrained);
        m_model           = nullptr;
        m_modelWasTrained = false;
    }
    m_model = svm_train(&m_prob, &m_param);

    if (m_model == nullptr) {
        this->getLogManager() << Kernel::LogLevel_Error << "the training with SVM had failed\n";
        return false;
    }

    m_modelWasTrained = true;

    //std::cout<<"log model"<<std::endl;
    this->getLogManager() << Kernel::LogLevel_Trace << modelToString();

    return true;
}

bool CAlgorithmClassifierSVM::classify(const Toolkit::IFeatureVector& sample, double& classLabel, Toolkit::IVector& distance, Toolkit::IVector& probability)
{
    //std::cout<<"classify"<<std::endl;
    if (m_model == nullptr) {
        this->getLogManager() << Kernel::LogLevel_Error << "Classification is impossible with a model equalling nullptr\n";
        return false;
    }
    if (m_model->nr_class == 0 || m_model->rho == nullptr) {
        this->getLogManager() << Kernel::LogLevel_Error << "The model wasn't loaded correctly\n";
        return false;
    }
    if (m_nFeatures != sample.getSize()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Classifier expected " << m_nFeatures << " features, got " << sample.getSize() << "\n";
        return false;
    }
    if (std::fabs(m_model->param.gamma) <= DBL_EPSILON &&
        (m_model->param.kernel_type == POLY || m_model->param.kernel_type == RBF || m_model->param.kernel_type == SIGMOID)) {
        m_model->param.gamma = 1.0 / (m_nFeatures > 0 ? double(m_nFeatures) : 1.0);
        this->getLogManager() << Kernel::LogLevel_Warning << "The SVM model had gamma=0. Setting it to [" << m_model->param.gamma << "].\n";
    }

    //std::cout<<"create X"<<std::endl;
    svm_node* x = new svm_node[sample.getSize() + 1];
    //std::cout<<"featureVector.getSize():"<<featureVector.getSize()<<"m_numberOfFeatures"<<m_numberOfFeatures<<std::endl;
    for (uint32_t i = 0; i < sample.getSize(); ++i) {
        x[i].index = int(i + 1);
        x[i].value = sample.getBuffer()[i];
        //std::cout<< X[i].index << ";"<<X[i].value<<" ";
    }
    x[sample.getSize()].index = -1;

    //std::cout<<"create ProbEstimates"<<std::endl;
    double* probEstimates = new double[m_model->nr_class];
    for (size_t i = 0; i < size_t(m_model->nr_class); ++i) { probEstimates[i] = 0; }

    classLabel = svm_predict_probability(m_model, x, probEstimates);

    //std::cout<<classLabel<<std::endl;
    //std::cout<<"probability"<<std::endl;

    //If we are not in these modes, label is nullptr and there is no probability
    if (m_model->param.svm_type == C_SVC || m_model->param.svm_type == NU_SVC) {
        probability.setSize(m_model->nr_class);
        this->getLogManager() << Kernel::LogLevel_Trace << "Label predict: " << classLabel << "\n";

        for (size_t i = 0; i < size_t(m_model->nr_class); ++i) {
            this->getLogManager() << Kernel::LogLevel_Trace << "index:" << i << " label:" << m_model->label[i] << " probability:" << probEstimates[i] << "\n";
            probability[(m_model->label[i])] = probEstimates[i];
        }
    }
    else { probability.setSize(0); }

    //The hyperplane distance is disabled for SVM
    distance.setSize(0);

    //std::cout<<";"<<classLabel<<";"<<distance[0] <<";"<<ProbEstimates[0]<<";"<<ProbEstimates[1]<<std::endl;
    //std::cout<<"Label predict "<<classLabel<< " proba:"<<distance[0]<<std::endl;
    //std::cout<<"end classify"<<std::endl;
    delete[] x;
    delete[] probEstimates;

    return true;
}

XML::IXMLNode* CAlgorithmClassifierSVM::saveConfig()
{
    //xml file
    //std::cout<<"model save"<<std::endl;

    std::vector<CString> coefs;
    std::vector<CString> values;

    //std::cout<<"model save: rho"<<std::endl;
    std::stringstream ssRho;
    ssRho << std::scientific << m_model->rho[0];

    for (size_t i = 1; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ssRho << " " << m_model->rho[i]; }

    //std::cout<<"model save: sv_coef and SV"<<std::endl;
    for (size_t i = 0; i < size_t(m_model->l); ++i) {
        std::stringstream ssCoef;
        std::stringstream ssValue;

        ssCoef << m_model->sv_coef[0][i];
        for (int j = 1; j < m_model->nr_class - 1; ++j) { ssCoef << " " << m_model->sv_coef[j][i]; }

        const svm_node* p = m_model->SV[i];

        if (m_model->param.kernel_type == PRECOMPUTED) { ssValue << "0:" << double(p->value); }
        else {
            if (p->index != -1) {
                ssValue << p->index << ":" << p->value;
                p++;
            }
            while (p->index != -1) {
                ssValue << " " << p->index << ":" << p->value;
                p++;
            }
        }
        coefs.emplace_back(ssCoef.str().c_str());
        values.emplace_back(ssValue.str().c_str());
    }

    XML::IXMLNode* svmNode = XML::createNode(TYPE_NODE_NAME);

    //Param node
    XML::IXMLNode* paramNode = XML::createNode(PARAM_NODE_NAME);
    XML::IXMLNode* tempNode  = XML::createNode(SVM_TYPE_NODE_NAME);
    tempNode->setPCData(get_svm_type(m_model->param.svm_type));
    paramNode->addChild(tempNode);

    tempNode = XML::createNode(KERNEL_TYPE_NODE_NAME);
    tempNode->setPCData(get_kernel_type(m_model->param.kernel_type));
    paramNode->addChild(tempNode);

    if (m_model->param.kernel_type == POLY) {
        std::stringstream ss;
        ss << m_model->param.degree;

        tempNode = XML::createNode(DEGREE_NODE_NAME);
        tempNode->setPCData(ss.str().c_str());
        paramNode->addChild(tempNode);
    }
    if (m_model->param.kernel_type == POLY || m_model->param.kernel_type == RBF || m_model->param.kernel_type == SIGMOID) {
        std::stringstream ss;
        ss << m_model->param.gamma;

        tempNode = XML::createNode(GAMMA_NODE_NAME);
        tempNode->setPCData(ss.str().c_str());
        paramNode->addChild(tempNode);
    }
    if (m_model->param.kernel_type == POLY || m_model->param.kernel_type == SIGMOID) {
        std::stringstream ss;
        ss << m_model->param.coef0;

        tempNode = XML::createNode(COEF0_NODE_NAME);
        tempNode->setPCData(ss.str().c_str());
        paramNode->addChild(tempNode);
    }
    svmNode->addChild(paramNode);
    //End param node

    //Model Node
    XML::IXMLNode* modelNode = XML::createNode(MODEL_NODE_NAME);
    {
        tempNode = XML::createNode(NR_CLASS_NODE_NAME);
        std::stringstream ssNrClass;
        ssNrClass << m_model->nr_class;
        tempNode->setPCData(ssNrClass.str().c_str());
        modelNode->addChild(tempNode);

        tempNode = XML::createNode(TOTAL_SV_NODE_NAME);
        std::stringstream ssTotalSv;
        ssTotalSv << m_model->l;
        tempNode->setPCData(ssTotalSv.str().c_str());
        modelNode->addChild(tempNode);

        tempNode = XML::createNode(RHO_NODE_NAME);
        tempNode->setPCData(ssRho.str().c_str());
        modelNode->addChild(tempNode);

        if (m_model->label != nullptr) {
            std::stringstream ss;
            ss << m_model->label[0];
            for (size_t i = 1; i < size_t(m_model->nr_class); ++i) { ss << " " << m_model->label[i]; }

            tempNode = XML::createNode(LABEL_NODE_NAME);
            tempNode->setPCData(ss.str().c_str());
            modelNode->addChild(tempNode);
        }
        if (m_model->probA != nullptr) {
            std::stringstream ss;
            ss << std::scientific << m_model->probA[0];
            for (size_t i = 1; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss << " " << m_model->probA[i]; }

            tempNode = XML::createNode(PROB_A_NODE_NAME);
            tempNode->setPCData(ss.str().c_str());
            modelNode->addChild(tempNode);
        }
        if (m_model->probB != nullptr) {
            std::stringstream ss;
            ss << std::scientific << m_model->probB[0];
            for (size_t i = 1; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss << " " << m_model->probB[i]; }

            tempNode = XML::createNode(PROB_B_NODE_NAME);
            tempNode->setPCData(ss.str().c_str());
            modelNode->addChild(tempNode);
        }
        if (m_model->nSV != nullptr) {
            std::stringstream ss;
            ss << m_model->nSV[0];
            for (size_t i = 1; i < size_t(m_model->nr_class); ++i) { ss << " " << m_model->nSV[i]; }

            tempNode = XML::createNode(NR_SV_NODE_NAME);
            tempNode->setPCData(ss.str().c_str());
            modelNode->addChild(tempNode);
        }

        XML::IXMLNode* svsNode = XML::createNode(SVS_NODE_NAME);
        {
            for (size_t i = 0; i < size_t(m_model->l); ++i) {
                XML::IXMLNode* svNode = XML::createNode(SV_NODE_NAME);
                {
                    tempNode = XML::createNode(COEF_NODE_NAME);
                    tempNode->setPCData(coefs[i]);
                    svNode->addChild(tempNode);

                    tempNode = XML::createNode(VALUE_NODE_NAME);
                    tempNode->setPCData(values[i]);
                    svNode->addChild(tempNode);
                }
                svsNode->addChild(svNode);
            }
        }
        modelNode->addChild(svsNode);
    }
    svmNode->addChild(modelNode);
    return svmNode;
}

bool CAlgorithmClassifierSVM::loadConfig(XML::IXMLNode* configNode)
{
    if (m_model != nullptr) {
        //std::cout<<"delete m_model load config"<<std::endl;
        deleteModel(m_model, !m_modelWasTrained);
        m_model           = nullptr;
        m_modelWasTrained = false;
    }
    //std::cout<<"load config"<<std::endl;
    m_model        = new svm_model();
    m_model->rho   = nullptr;
    m_model->probA = nullptr;
    m_model->probB = nullptr;
    m_model->label = nullptr;
    m_model->nSV   = nullptr;
    m_indexSV      = -1;

    loadParamNodeConfiguration(configNode->getChildByName(PARAM_NODE_NAME));
    loadModelNodeConfiguration(configNode->getChildByName(MODEL_NODE_NAME));

    this->getLogManager() << Kernel::LogLevel_Trace << modelToString();
    return true;
}

void CAlgorithmClassifierSVM::loadParamNodeConfiguration(XML::IXMLNode* paramNode)
{
    //svm_type
    XML::IXMLNode* tempNode = paramNode->getChildByName(SVM_TYPE_NODE_NAME);
    for (int i = 0; get_svm_type(i) != nullptr; ++i) { if (strcmp(get_svm_type(i), tempNode->getPCData()) == 0) { m_model->param.svm_type = i; } }
    if (get_svm_type(m_model->param.svm_type) == nullptr) {
        this->getLogManager() << Kernel::LogLevel_Error << "load configuration error: bad value for the parameter svm_type\n";
    }

    //kernel_type
    tempNode = paramNode->getChildByName(KERNEL_TYPE_NODE_NAME);
    for (int i = 0; get_kernel_type(i) != nullptr; ++i) { if (strcmp(get_kernel_type(i), tempNode->getPCData()) == 0) { m_model->param.kernel_type = i; } }
    if (get_kernel_type(m_model->param.kernel_type) == nullptr) {
        this->getLogManager() << Kernel::LogLevel_Error << "load configuration error: bad value for the parameter kernel_type\n";
    }

    //Following parameters aren't required

    //degree
    tempNode = paramNode->getChildByName(DEGREE_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        ss >> m_model->param.degree;
    }

    //gamma
    tempNode = paramNode->getChildByName(GAMMA_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        ss >> m_model->param.gamma;
    }

    //coef0
    tempNode = paramNode->getChildByName(COEF0_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        ss >> m_model->param.coef0;
    }
}

void CAlgorithmClassifierSVM::loadModelNodeConfiguration(XML::IXMLNode* modelNode)
{
    //nr_class
    XML::IXMLNode* tempNode = modelNode->getChildByName(NR_CLASS_NODE_NAME);
    std::stringstream ssNrClass(tempNode->getPCData());
    ssNrClass >> m_model->nr_class;
    //total_sv
    tempNode = modelNode->getChildByName(TOTAL_SV_NODE_NAME);
    std::stringstream ssTotalSv(tempNode->getPCData());
    ssTotalSv >> m_model->l;
    //rho
    tempNode = modelNode->getChildByName(RHO_NODE_NAME);
    std::stringstream ssRho(tempNode->getPCData());
    m_model->rho = new double[m_model->nr_class * (m_model->nr_class - 1) / 2];
    for (size_t i = 0; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ssRho >> m_model->rho[i]; }

    //label
    tempNode = modelNode->getChildByName(LABEL_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        m_model->label = new int[m_model->nr_class];
        for (size_t i = 0; i < size_t(m_model->nr_class); ++i) { ss >> m_model->label[i]; }
    }
    //probA
    tempNode = modelNode->getChildByName(PROB_A_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        m_model->probA = new double[m_model->nr_class * (m_model->nr_class - 1) / 2];
        for (size_t i = 0; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss >> m_model->probA[i]; }
    }
    //probB
    tempNode = modelNode->getChildByName(PROB_B_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        m_model->probB = new double[m_model->nr_class * (m_model->nr_class - 1) / 2];
        for (size_t i = 0; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss >> m_model->probB[i]; }
    }
    //nr_sv
    tempNode = modelNode->getChildByName(NR_SV_NODE_NAME);
    if (tempNode != nullptr) {
        std::stringstream ss(tempNode->getPCData());
        m_model->nSV = new int[m_model->nr_class];
        for (size_t i = 0; i < size_t(m_model->nr_class); ++i) { ss >> m_model->nSV[i]; }
    }

    loadModelSVsNodeConfiguration(modelNode->getChildByName(SVS_NODE_NAME));
}

void CAlgorithmClassifierSVM::loadModelSVsNodeConfiguration(const XML::IXMLNode* svsNodeParam)
{
    //Reserve all memory space required
    m_model->sv_coef = new double*[m_model->nr_class - 1];
    for (size_t i = 0; i < size_t(m_model->nr_class - 1); ++i) { m_model->sv_coef[i] = new double[m_model->l]; }
    m_model->SV = new svm_node*[m_model->l];

    //Now fill SV
    for (size_t i = 0; i < svsNodeParam->getChildCount(); ++i) {
        const XML::IXMLNode* tempNode = svsNodeParam->getChild(i);
        std::stringstream coefData(tempNode->getChildByName(COEF_NODE_NAME)->getPCData());
        for (int j = 0; j < m_model->nr_class - 1; ++j) { coefData >> m_model->sv_coef[j][i]; }

        std::stringstream ss(tempNode->getChildByName(VALUE_NODE_NAME)->getPCData());
        std::vector<int> svmIdx;
        std::vector<double> svmValue;
        char separateChar;
        while (!ss.eof()) {
            int index;
            double value;
            ss >> index;
            ss >> separateChar;
            ss >> value;
            svmIdx.push_back(index);
            svmValue.push_back(value);
        }

        m_nFeatures    = svmIdx.size();
        m_model->SV[i] = new svm_node[svmIdx.size() + 1];
        for (size_t j = 0; j < svmIdx.size(); ++j) {
            m_model->SV[i][j].index = svmIdx[j];
            m_model->SV[i][j].value = svmValue[j];
        }
        m_model->SV[i][svmIdx.size()].index = -1;
    }
}

CString CAlgorithmClassifierSVM::paramToString(svm_parameter* param)
{
    if (param == nullptr) { return std::string("Param: nullptr\n").c_str(); }

    std::stringstream ss;
    ss << "Param:\n";
    ss << "\tsvm_type: " << get_svm_type(param->svm_type) << "\n";
    ss << "\tkernel_type: " << get_kernel_type(param->kernel_type) << "\n";
    ss << "\tdegree: " << param->degree << "\n";
    ss << "\tgamma: " << param->gamma << "\n";
    ss << "\tcoef0: " << param->coef0 << "\n";
    ss << "\tnu: " << param->nu << "\n";
    ss << "\tcache_size: " << param->cache_size << "\n";
    ss << "\tC: " << param->C << "\n";
    ss << "\teps: " << param->eps << "\n";
    ss << "\tp: " << param->p << "\n";
    ss << "\tshrinking: " << param->shrinking << "\n";
    ss << "\tprobability: " << param->probability << "\n";
    ss << "\tnr weight: " << param->nr_weight << "\n";
    std::stringstream label;
    for (size_t i = 0; i < size_t(param->nr_weight); ++i) { label << param->weight_label[i] << ";"; }
    ss << "\tweight label: " << label.str() << "\n";
    std::stringstream weight;
    for (size_t i = 0; i < size_t(param->nr_weight); ++i) { weight << param->weight[i] << ";"; }
    ss << "\tweight: " << weight.str() << "\n";
    return ss.str().c_str();
}


CString CAlgorithmClassifierSVM::modelToString() const
{
    if (m_model == nullptr) { return std::string("Model: nullptr\n").c_str(); }

    std::stringstream ss;
    ss << paramToString(&m_model->param);
    ss << "Model:" << "\n";
    ss << "\tnr_class: " << m_model->nr_class << "\n";
    ss << "\ttotal_sv: " << m_model->l << "\n";
    ss << "\trho: ";
    if (m_model->rho != nullptr) {
        ss << m_model->rho[0];
        for (size_t i = 1; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss << " " << m_model->rho[i]; }
    }
    ss << "\n";
    ss << "\tlabel: ";
    if (m_model->label != nullptr) {
        ss << m_model->label[0];
        for (size_t i = 1; i < size_t(m_model->nr_class); ++i) { ss << " " << m_model->label[i]; }
    }
    ss << "\n";
    ss << "\tprobA: ";
    if (m_model->probA != nullptr) {
        ss << m_model->probA[0];
        for (size_t i = 1; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss << " " << m_model->probA[i]; }
    }
    ss << "\n";
    ss << "\tprobB: ";
    if (m_model->probB != nullptr) {
        ss << m_model->probB[0];
        for (size_t i = 1; i < size_t(m_model->nr_class * (m_model->nr_class - 1) / 2); ++i) { ss << " " << m_model->probB[i]; }
    }
    ss << "\n";
    ss << "\tnr_sv: ";
    if (m_model->nSV != nullptr) {
        ss << m_model->nSV[0];
        for (size_t i = 1; i < size_t(m_model->nr_class); ++i) { ss << " " << m_model->nSV[i]; }
    }
    ss << "\n";

    return ss.str().c_str();
}

CString CAlgorithmClassifierSVM::problemToString(svm_problem* prob) const
{
    if (prob == nullptr) { return std::string("Problem: nullptr\n").c_str(); }
    std::stringstream ss;
    ss << "Problem\ttotal sv: " << prob->l << "\n\tnb features: " << m_nFeatures << "\n";
    return ss.str().c_str();
}
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
