#include <vector>

#include "ovp_defines.h"
#include "toolkit/algorithms/classification/ovtkCAlgorithmPairingStrategy.h" //For comparision mecanism

#include "algorithms/ovpCAlgorithmClassifierSVM.h"

#include "box-algorithms/ovpCBoxAlgorithmOutlierRemoval.h"

#include "algorithms/ovpCAlgorithmClassifierMLP.h"

#include<cmath>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

OVP_Declare_Begin()
    // SVM related
    context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Support Vector Machine (SVM)",
                                                      OVP_ClassId_Algorithm_ClassifierSVM.id());
    Toolkit::registerClassificationComparisonFunction(OVP_ClassId_Algorithm_ClassifierSVM, SVMClassificationCompare);
    OVP_Declare_New(CAlgorithmClassifierSVMDesc)

    context.getTypeManager().registerEnumerationType(OVP_TypeId_SVMType, "SVM Type");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType, "C-SVC", C_SVC);
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMType, "Nu-SVC", NU_SVC);

    context.getTypeManager().registerEnumerationType(OVP_TypeId_SVMKernelType, "SVM Kernel Type");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType, "Linear", LINEAR);
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType, "Polinomial", POLY);
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType, "Radial basis function", RBF);
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_SVMKernelType, "Sigmoid", SIGMOID);

    context.getTypeManager().registerEnumerationType(OVP_ClassId_Algorithm_ClassifierSVM_DecisionAvailable, "Pairwise Decision Strategy");
    context.getTypeManager().registerEnumerationEntry(OVP_ClassId_Algorithm_ClassifierSVM_DecisionAvailable, "PKPD", Algorithm_PairwiseStrategy_PKPD.id());
    context.getTypeManager().registerEnumerationEntry(OVP_ClassId_Algorithm_ClassifierSVM_DecisionAvailable, "Voting", Algorithm_PairwiseDecision_Voting.id());
    context.getTypeManager().registerEnumerationEntry(OVP_ClassId_Algorithm_ClassifierSVM_DecisionAvailable, "HT", Algorithm_PairwiseDecision_HT.id());

    context.getTypeManager().registerEnumerationType(OVP_TypeId_OneVsOne_DecisionAlgorithms, "One vs One Decision Algorithms");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_OneVsOne_DecisionAlgorithms, "Support Vector Machine (SVM)",
                                                      OVP_ClassId_Algorithm_ClassifierSVM_DecisionAvailable.id());

    //MLP sectionE
    OVP_Declare_New(CAlgorithmClassifierMLPDesc)
    context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Multi-layer Perceptron",
                                                      OVP_ClassId_Algorithm_ClassifierMLP.id());
    Toolkit::registerClassificationComparisonFunction(OVP_ClassId_Algorithm_ClassifierMLP, MLPClassificationCompare);

    // Register boxes
    OVP_Declare_New(CBoxAlgorithmOutlierRemovalDesc)

OVP_Declare_End()

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE


bool OVFloatEqual(const double first, const double second)
{
    const double epsilon = 0.000001;
    return epsilon > fabs(first - second);
}
