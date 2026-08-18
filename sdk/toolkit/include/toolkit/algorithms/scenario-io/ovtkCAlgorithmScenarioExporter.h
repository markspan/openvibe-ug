#pragma once

#include "../ovtkTAlgorithm.h"
#include "ovtk_scenario_io.h"

#include <openvibe/kernel/scenario/ovIAlgorithmScenarioExporter.h>

#define OVTK_ClassId_Algorithm_ScenarioExporter     OpenViBE::CIdentifier(0x7C281FEA, 0x40B66277)
#define OVTK_ClassId_Algorithm_ScenarioExporterDesc OpenViBE::CIdentifier(0x49A9778E, 0x7BB377F9)

namespace OpenViBE {
namespace Toolkit {
class OVTK_API CAlgorithmScenarioExporter : public TAlgorithm<Plugins::IAlgorithmScenarioExporter>
{
public:
    void release() override { delete this; }
    bool process() override;

    _IsDerivedFromClass_Final_(TAlgorithm<Plugins::IAlgorithmScenarioExporter>, OVTK_ClassId_Algorithm_ScenarioExporter)
};

class OVTK_API CAlgorithmScenarioExporterDesc : public Plugins::IAlgorithmScenarioExporterDesc
{
public:

    _IsDerivedFromClass_(Plugins::IAlgorithmScenarioExporterDesc, OVTK_ClassId_Algorithm_ScenarioExporterDesc)
};
}  // namespace Toolkit
}  // namespace OpenViBE
