#pragma once

#include "../ovtkTAlgorithm.h"
#include "ovtk_scenario_io.h"
#include <openvibe/kernel/scenario/ovIAlgorithmScenarioImporter.h>

#define OVP_ClassId_Algorithm_ScenarioImporter     OpenViBE::CIdentifier(0x1EE72169, 0x2BF146C1)
#define OVP_ClassId_Algorithm_ScenarioImporterDesc OpenViBE::CIdentifier(0x503C0DDE, 0x5D4C6CB2)

namespace OpenViBE {
namespace Toolkit {
class OVTK_API CAlgorithmScenarioImporter : public TAlgorithm<Plugins::IAlgorithmScenarioImporter>
{
public:
    void release() override { delete this; }
    bool process() override;

    _IsDerivedFromClass_(TAlgorithm<Plugins::IAlgorithmScenarioImporter>, OVP_ClassId_Algorithm_ScenarioImporter)
};

class OVTK_API CAlgorithmScenarioImporterDesc : public Plugins::IAlgorithmScenarioImporterDesc
{
public:

    _IsDerivedFromClass_(Plugins::IAlgorithmScenarioImporterDesc, OVP_ClassId_Algorithm_ScenarioImporterDesc)
};
}  // namespace Toolkit
}  // namespace OpenViBE
