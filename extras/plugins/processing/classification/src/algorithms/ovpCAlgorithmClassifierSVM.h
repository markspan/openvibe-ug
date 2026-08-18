#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>

#include <stack>
#include "../../../../../contrib/packages/libSVM/svm.h"

#define OVP_ClassId_Algorithm_ClassifierSVM                                     CIdentifier(0x50486EC2, 0x6F2417FC)
#define OVP_ClassId_Algorithm_ClassifierSVM_DecisionAvailable                   CIdentifier(0x21A61E69, 0xD522CE01)
#define OVP_ClassId_Algorithm_ClassifierSVMDesc                                 CIdentifier(0x272B056E, 0x0C6502AC)

#define OVP_Algorithm_ClassifierSVM_InputParameterId_SVMType                    CIdentifier(0x0C347BBA, 0x180577F9)
#define OVP_Algorithm_ClassifierSVM_InputParameterId_SVMKernelType              CIdentifier(0x1952129C, 0x6BEF38D7)
#define OVP_Algorithm_ClassifierSVM_InputParameterId_SVMDegree                  CIdentifier(0x0E284608, 0x7323390E)
#define OVP_Algorithm_ClassifierSVM_InputParameterId_SVMGamma                   CIdentifier(0x5D4A358F, 0x29043846)
#define OVP_Algorithm_ClassifierSVM_InputParameterId_SVMCoef0                   CIdentifier(0x724D5EC5, 0x13E56658)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCost                    CIdentifier(0x353662E8, 0x041D7610)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMNu                      CIdentifier(0x62334FC3, 0x49594D32)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMEpsilon                 CIdentifier(0x09896FD2, 0x523775BA)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCacheSize               CIdentifier(0x4BCE65A7, 0x6A103468)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMTolerance               CIdentifier(0x2658168C, 0x0914687C)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking               CIdentifier(0x63F5286A, 0x6A9D18BF)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMProbabilityEstimate     CIdentifier(0x05DC16EA, 0x5DBD51C2)
#define OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMweight                  CIdentifier(0x0BA132BE, 0x17DD3B8F)
#define OVP_Algorithm_ClassifierSVM_InputParameterId_SVMWeightLabel             CIdentifier(0x22C27048, 0x5CC6214A)

#define OVP_TypeId_SVMType                                                      CIdentifier(0x2AF426D1, 0x72FB7BAC)
#define OVP_TypeId_SVMKernelType                                                CIdentifier(0x54BB0016, 0x6AA27496)

namespace OpenViBE {
namespace Plugins {
namespace Classification {
int SVMClassificationCompare(CMatrix& first, CMatrix& second);


class CAlgorithmClassifierSVM final : public Toolkit::CAlgorithmClassifier
{
public:
    CAlgorithmClassifierSVM() { }

    bool initialize() override;
    bool uninitialize() override;

    bool train(const Toolkit::IFeatureVectorSet& dataset) override;
    bool classify(const Toolkit::IFeatureVector& sample, double& classLabel, Toolkit::IVector& distance, Toolkit::IVector& probability) override;

    XML::IXMLNode* saveConfig() override;
    bool loadConfig(XML::IXMLNode* configNode) override;
    static CString paramToString(svm_parameter* param);
    CString modelToString() const;
    CString problemToString(svm_problem* prob) const;

    size_t getNProbabilities() override { return 1; }
    size_t getNDistances() override { return 0; }

    _IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierSVM)

protected:
    std::vector<double> m_class;
    struct svm_parameter m_param;

    //struct svm_parameter *m_param;    // set by parse_command_line
    struct svm_problem m_prob;          // set by read_problem
    struct svm_model* m_model = nullptr;

    bool m_modelWasTrained = false;     // true if from svm_train(), false if loaded

    int m_indexSV      = 0;
    size_t m_nFeatures = 0;
    CMemoryBuffer m_config;
    //todo a modifier en fonction de svn_save_model
    //vector m_coefficients;

private:
    void loadParamNodeConfiguration(XML::IXMLNode* paramNode);
    void loadModelNodeConfiguration(XML::IXMLNode* modelNode);
    void loadModelSVsNodeConfiguration(const XML::IXMLNode* svsNodeParam);

    void setParameter();

    static void deleteModel(svm_model* model, bool freeSupportVectors);
};

class CAlgorithmClassifierSVMDesc final : public Toolkit::CAlgorithmClassifierDesc
{
public:
    void release() override { }

    CString getName() const override { return "SVM classifier"; }
    CString getAuthorName() const override { return "Laurent Bougrain / Baptiste Payan"; }
    CString getAuthorCompanyName() const override { return "UHP_Nancy1/LORIA INRIA/LORIA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ClassifierSVM; }
    IPluginObject* create() override { return new CAlgorithmClassifierSVM; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmClassifierDesc::getAlgorithmPrototype(prototype);
        prototype.addInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMType, "SVM type", Kernel::ParameterType_Enumeration,OVP_TypeId_SVMType);
        prototype.addInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMKernelType, "Kernel type", Kernel::ParameterType_Enumeration,
                                    OVP_TypeId_SVMKernelType);
        prototype.addInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMDegree, "Degree", Kernel::ParameterType_Integer);
        prototype.addInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMGamma, "Gamma", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMCoef0, "Coef 0", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCost, "Cost", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMNu, "Nu", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMEpsilon, "Epsilon", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMCacheSize, "Cache size", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMTolerance, "Epsilon tolerance", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMShrinking, "Shrinking", Kernel::ParameterType_Boolean);
        //prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMProbabilityEstimate,"Probability estimate",Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_ALgorithm_ClassifierSVM_InputParameterId_SVMweight, "Weight", Kernel::ParameterType_String);
        prototype.addInputParameter(OVP_Algorithm_ClassifierSVM_InputParameterId_SVMWeightLabel, "Weight Label", Kernel::ParameterType_String);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierSVMDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
