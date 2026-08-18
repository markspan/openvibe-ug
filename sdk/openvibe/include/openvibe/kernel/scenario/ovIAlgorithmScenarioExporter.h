#pragma once

#include "../../plugins/ovIAlgorithm.h"
#include "../../plugins/ovIAlgorithmDesc.h"
#include "../algorithm/ovIAlgorithmProto.h"
#include "../../CMemoryBuffer.hpp"

#define OV_Algorithm_ScenarioExporter_InputParameterId_Scenario         OpenViBE::CIdentifier(0x5B9C0D54, 0x04BA2957)
#define OV_Algorithm_ScenarioExporter_OutputParameterId_MemoryBuffer    OpenViBE::CIdentifier(0x64030633, 0x419E3A33)

namespace OpenViBE {
namespace Plugins {
class OV_API IAlgorithmScenarioExporter : public IAlgorithm
{
public:
    virtual bool exportStart(CMemoryBuffer& buffer, const CIdentifier& id) = 0;
    virtual bool exportIdentifier(CMemoryBuffer& buffer, const CIdentifier& id, const CIdentifier& value) = 0;
    virtual bool exportString(CMemoryBuffer& buffer, const CIdentifier& id, const CString& value) = 0;
    virtual bool exportUInteger(CMemoryBuffer& buffer, const CIdentifier& id, uint64_t value) = 0;
    virtual bool exportStop(CMemoryBuffer& buffer) = 0;

    _IsDerivedFromClass_(IAlgorithm, CIdentifier::undefined())
};

class OV_API IAlgorithmScenarioExporterDesc : public IAlgorithmDesc
{
public:
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OV_Algorithm_ScenarioExporter_InputParameterId_Scenario, "Scenario", Kernel::ParameterType_Object);
        prototype.addOutputParameter(OV_Algorithm_ScenarioExporter_OutputParameterId_MemoryBuffer, "Memory buffer", Kernel::ParameterType_MemoryBuffer);
        return true;
    }

    _IsDerivedFromClass_(IAlgorithmDesc, CIdentifier::undefined())
};
}  // namespace Plugins
}  // namespace OpenViBE
