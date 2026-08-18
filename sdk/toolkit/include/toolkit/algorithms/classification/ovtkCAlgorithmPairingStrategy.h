#pragma once

#include "../ovtkTAlgorithm.h"
#include "../../ovtkIVector.h"
#include "../../ovtkIFeatureVector.h"
#include "../../ovtkIFeatureVectorSet.h"
#include "ovtkCAlgorithmClassifier.h"

#define OVTK_ClassId_Algorithm_PairingStrategy                                      OpenViBE::CIdentifier(0xFD3CB444, 0x58F00765)
#define OVTK_ClassId_Algorithm_PairingStrategyDesc                                  OpenViBE::CIdentifier(0x4341B8D6, 0xC65B7BBB)

#define OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm      OpenViBE::CIdentifier(0xD9E60DF9, 0x20EC8FC9)
#define OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture            OpenViBE::CIdentifier(0x784A9CDF, 0xA41C27F8)

typedef int (*fClassifierComparison)(OpenViBE::CMatrix&, OpenViBE::CMatrix&);

namespace OpenViBE {
namespace Toolkit {
extern OVTK_API void registerClassificationComparisonFunction(const CIdentifier& classID, fClassifierComparison comparision);
extern OVTK_API fClassifierComparison getClassificationComparisonFunction(const CIdentifier& classID);


class OVTK_API CAlgorithmPairingStrategy : public CAlgorithmClassifier
{
public:
    bool process() override;
    void release() override { delete this; }

    virtual bool designArchitecture(const CIdentifier& id, const size_t nClass) = 0;
    bool train(const IFeatureVectorSet& rFeatureVectorSet) override = 0;
    bool classify(const IFeatureVector& rFeatureVector, double& classId, IVector& distance, IVector& probability) override = 0;
    XML::IXMLNode* saveConfig() override = 0;
    bool loadConfig(XML::IXMLNode* pConfiguratioNode) override = 0;
    _IsDerivedFromClass_(CAlgorithmClassifier, OVTK_ClassId_Algorithm_PairingStrategy)
    size_t getNProbabilities() override = 0;
    size_t getNDistances() override = 0;


protected:
    //  std::vector <double> m_classes;
    //The vector will be use when the user will be able to specify class label
    CIdentifier m_subClassifierAlgorithmID = CIdentifier::undefined();
};

class OVTK_API CAlgorithmPairingStrategyDesc : public CAlgorithmClassifierDesc
{
public:
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmClassifierDesc::getAlgorithmPrototype(prototype);
        prototype.addInputParameter(
            OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm, "Algorithm Identifier", Kernel::ParameterType_Identifier);
        prototype.addInputTrigger(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture, "Design Architecture");
        return true;
    }

    _IsDerivedFromClass_(CAlgorithmClassifierDesc, OVTK_ClassId_Algorithm_PairingStrategyDesc)
};
}  // namespace Toolkit
}  // namespace OpenViBE
