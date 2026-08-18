#pragma once

#include "../../plugins/ovIAlgorithm.h"
#include "../../plugins/ovIAlgorithmDesc.h"
#include "../algorithm/ovIAlgorithmProto.h"
#include "../../CMemoryBuffer.hpp"

#define OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario        OpenViBE::CIdentifier(0x29574C87, 0x7BA77780)
#define OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer     OpenViBE::CIdentifier(0x600463A3, 0x474B7F66)

namespace OpenViBE {
namespace Plugins {
class OV_API IAlgorithmScenarioImporterContext : public IObject
{
public:

    virtual bool processStart(const CIdentifier& id) = 0;
    virtual bool processIdentifier(const CIdentifier& id, const CIdentifier& value) = 0;
    virtual bool processString(const CIdentifier& id, const CString& value) = 0;
    virtual bool processUInteger(const CIdentifier& id, uint64_t value) = 0;
    virtual bool processStop() = 0;

    _IsDerivedFromClass_(IObject, CIdentifier::undefined())
};

class OV_API IAlgorithmScenarioImporter : public IAlgorithm
{
public:
    virtual bool import(IAlgorithmScenarioImporterContext& ctx, const CMemoryBuffer& buffer) = 0;
};


class OV_API IAlgorithmScenarioImporterDesc : public IAlgorithmDesc
{
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addOutputParameter(OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario, "Scenario", Kernel::ParameterType_Object);
        prototype.addInputParameter(OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer, "Memory buffer", Kernel::ParameterType_MemoryBuffer);
        return true;
    }
};
}  // namespace Plugins
}  // namespace OpenViBE
