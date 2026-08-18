#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define OVP_ClassId_Algorithm_ClassifierMLP                                 CIdentifier(0xF3FAB4BE, 0xDC401260)
#define OVP_ClassId_Algorithm_ClassifierMLP_DecisionAvailable               CIdentifier(0xF3FAB4BE, 0xDC401261)
#define OVP_ClassId_Algorithm_ClassifierMLPDesc                             CIdentifier(0xF3FAB4BE, 0xDC401262)

#define Algorithm_PairwiseDecision_HT                                       CIdentifier(0xD24F7F19, 0xA744FAD2)
#define Algorithm_PairwiseDecision_Voting                                   CIdentifier(0xA111B830, 0x4679BAFD)
#define Algorithm_PairwiseStrategy_PKPD                                     CIdentifier(0x26EF6DDA, 0xF137053C)

#define OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount      CIdentifier(0xF3FAB4BE, 0xDC401263)
#define OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon                CIdentifier(0xF3FAB4BE, 0xDC401264)
#define OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha                  CIdentifier(0xF3FAB4BE, 0xDC401265)

#include <Eigen/Dense>

#include <xml/IXMLNode.h>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
int MLPClassificationCompare(CMatrix& first, CMatrix& second);

class CAlgorithmClassifierMLP final : public Toolkit::CAlgorithmClassifier
{
public:
    bool initialize() override;
    bool uninitialize() override;

    bool train(const Toolkit::IFeatureVectorSet& dataset) override;
    bool classify(const Toolkit::IFeatureVector& sample, double& classLabel,
                  Toolkit::IVector& distance, Toolkit::IVector& probability) override;

    XML::IXMLNode* saveConfig() override;
    bool loadConfig(XML::IXMLNode* configNode) override;

    size_t getNProbabilities() override { return m_labels.size(); }
    size_t getNDistances() override { return m_labels.size(); }

    _IsDerivedFromClass_Final_(CAlgorithmClassifier, OVP_ClassId_Algorithm_ClassifierMLP)

private:
    //Helpers for load or sotre data in XMLNode
    static void dumpData(XML::IXMLNode* node, Eigen::MatrixXd& matrix);
    static void dumpData(XML::IXMLNode* node, Eigen::VectorXd& vector);
    static void dumpData(XML::IXMLNode* node, int64_t value);
    static void dumpData(XML::IXMLNode* node, double value);

    static void loadData(const XML::IXMLNode* node, Eigen::MatrixXd& matrix, size_t nRow, size_t nCol);
    static void loadData(const XML::IXMLNode* node, Eigen::VectorXd& vector);
    static void loadData(const XML::IXMLNode* node, int64_t& value);
    static void loadData(const XML::IXMLNode* node, double& value);

    std::vector<double> m_labels;

    Eigen::MatrixXd m_inputWeight;
    Eigen::VectorXd m_inputBias;

    Eigen::MatrixXd m_hiddenWeight;
    Eigen::VectorXd m_hiddenBias;

    double m_min = 0;
    double m_max = 0;
};

class CAlgorithmClassifierMLPDesc final : public Toolkit::CAlgorithmClassifierDesc
{
public:
    void release() override { }

    CString getName() const override { return "MLP Classifier"; }
    CString getAuthorName() const override { return "Guillaume Serrière"; }
    CString getAuthorCompanyName() const override { return "Inria / Loria"; }
    CString getShortDescription() const override { return "Multi-layer perceptron algorithm"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "0.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ClassifierMLP; }
    IPluginObject* create() override { return new CAlgorithmClassifierMLP; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmClassifierDesc::getAlgorithmPrototype(prototype);

        prototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_HiddenNeuronCount, "Number of neurons in hidden layer",
                                    Kernel::ParameterType_Integer);
        prototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Epsilon, "Learning stop condition", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_ClassifierMLP_InputParameterId_Alpha, "Learning coefficient", Kernel::ParameterType_Float);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, OVP_ClassId_Algorithm_ClassifierMLPDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
