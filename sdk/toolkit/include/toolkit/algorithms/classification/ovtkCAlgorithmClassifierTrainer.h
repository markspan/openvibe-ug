#pragma once

#include "../ovtkTAlgorithm.h"
#include "../../ovtkIFeatureVectorSet.h"

#define OVTK_ClassId_Algorithm_ClassifierTrainer                            OpenViBE::CIdentifier(0x5ABC21E6, 0x3C3D349B)
#define OVTK_ClassId_Algorithm_ClassifierTrainerDesc                        OpenViBE::CIdentifier(0x56DD6401, 0x07033341)
#define OVTK_Algorithm_ClassifierTrainer_InputParameterId_FeatureVectorSet  OpenViBE::CIdentifier(0x27C05927, 0x5DE9103A)
#define OVTK_Algorithm_ClassifierTrainer_OutputParameterId_Config           OpenViBE::CIdentifier(0x30590936, 0x61CE5971)
#define OVTK_Algorithm_ClassifierTrainer_InputTriggerId_Train               OpenViBE::CIdentifier(0x34684752, 0x78A46DE2)
#define OVTK_Algorithm_ClassifierTrainer_InputTriggerId_SaveConfig          OpenViBE::CIdentifier(0x79750528, 0x6CC85FC1)
#define OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Success            OpenViBE::CIdentifier(0x7B8C0EFF, 0x26224D6B)
#define OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Failed             OpenViBE::CIdentifier(0x31B97C83, 0x59015D0E)

namespace OpenViBE {
namespace Toolkit {
class OVTK_API CAlgorithmClassifierTrainer : public TAlgorithm<Plugins::IAlgorithm>
{
public:
    void release() override { delete this; }
    bool process() override;
    virtual bool train(const IFeatureVectorSet& dataset) = 0;
    virtual bool saveConfig(CMemoryBuffer& buffer) = 0;

    _IsDerivedFromClass_(TAlgorithm < Plugins::IAlgorithm >, OVTK_ClassId_Algorithm_ClassifierTrainer)
};

class OVTK_API CAlgorithmClassifierTrainerDesc : public Plugins::IAlgorithmDesc
{
public:
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVTK_Algorithm_ClassifierTrainer_InputParameterId_FeatureVectorSet, "Feature vector set", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVTK_Algorithm_ClassifierTrainer_OutputParameterId_Config, "Configuration", Kernel::ParameterType_MemoryBuffer);
        prototype.addInputTrigger(OVTK_Algorithm_ClassifierTrainer_InputTriggerId_Train, "Train");
        prototype.addInputTrigger(OVTK_Algorithm_ClassifierTrainer_InputTriggerId_SaveConfig, "Save configuration");
        prototype.addOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Success, "Success");
        prototype.addOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Failed, "Failed");
        return true;
    }

    _IsDerivedFromClass_(Plugins::IAlgorithmDesc, OVTK_ClassId_Algorithm_ClassifierTrainerDesc)
};
}  // namespace Toolkit
}  // namespace OpenViBE
