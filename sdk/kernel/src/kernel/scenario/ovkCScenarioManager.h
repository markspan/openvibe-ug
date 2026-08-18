#pragma once

#include "../ovkTKernelObject.h"

#include <map>

namespace OpenViBE {
namespace Kernel {
class CScenario;

class CScenarioManager final : public TKernelObject<IScenarioManager>
{
public:

    explicit CScenarioManager(const IKernelContext& ctx);
    ~CScenarioManager() override;
    void cloneScenarioImportersAndExporters(const IScenarioManager& scenarioManager) override;
    CIdentifier getNextScenarioIdentifier(const CIdentifier& previousID) const override;
    bool isScenario(const CIdentifier& scenarioID) const override;
    bool createScenario(CIdentifier& scenarioID) override;
    bool importScenario(CIdentifier& scenarioID, const CMemoryBuffer& iMemoryBuffer, const CIdentifier& scenarioImporterAlgorithmID) override;
    bool importScenarioFromFile(CIdentifier& scenarioID, const CString& fileName, const CIdentifier& scenarioImporterAlgorithmID) override;
    bool importScenarioFromFile(CIdentifier& scenarioID, const CIdentifier& importContext, const CString& fileName) override;
    bool registerScenarioImporter(const CIdentifier& importContext, const CString& fileNameExtension,
                                  const CIdentifier& scenarioImporterAlgorithmIdentifier) override;
    bool unregisterScenarioImporter(const CIdentifier& importContext, const CString& fileNameExtension) override;
    CIdentifier getNextScenarioImportContext(const CIdentifier& importContext) const override;
    CString getNextScenarioImporter(const CIdentifier& importContext, const CString& fileNameExtension) const override;
    CIdentifier getScenarioImporterAlgorithmIdentifier(const CIdentifier& importContext, const CString& fileNameExtension) const override;
    bool exportScenario(CMemoryBuffer& oMemoryBuffer, const CIdentifier& scenarioID, const CIdentifier& scenarioExporterAlgorithmID) const override;
    bool exportScenarioToFile(const CString& fileName, const CIdentifier& scenarioID, const CIdentifier& scenarioExporterAlgorithmID) const override;
    bool exportScenarioToFile(const CIdentifier& exportContext, const CString& fileName, const CIdentifier& scenarioID) override;
    bool registerScenarioExporter(const CIdentifier& exportContext, const CString& fileNameExtension,
                                  const CIdentifier& scenarioExporterAlgorithmIdentifier) override;
    bool unregisterScenarioExporter(const CIdentifier& exportContext, const CString& fileNameExtension) override;
    CIdentifier getNextScenarioExportContext(const CIdentifier& exportContext) const override;
    CString getNextScenarioExporter(const CIdentifier& exportContext, const CString& fileNameExtension) const override;
    CIdentifier getScenarioExporterAlgorithmIdentifier(const CIdentifier& exportContext, const CString& fileNameExtension) const override;
    bool releaseScenario(const CIdentifier& scenarioID) override;
    IScenario& getScenario(const CIdentifier& scenarioID) override;

    _IsDerivedFromClass_Final_(TKernelObject<IScenarioManager>, OVK_ClassId_Kernel_Scenario_ScenarioManager)

protected:

    CIdentifier getUnusedIdentifier() const;

    std::map<CIdentifier, CScenario*> m_scenarios;
private:
    /// Scenario Import Context -> File Name Extension -> Scenario Importer Identifier
    std::map<CIdentifier, std::map<std::string, CIdentifier>> m_importers;
    /// Scenario Export Context -> File Name Extension -> Scenario Exporter Identifier
    std::map<CIdentifier, std::map<std::string, CIdentifier>> m_exporters;

    IScenario& getScenario(const CIdentifier& scenarioID) const;
};
}  // namespace Kernel
}  // namespace OpenViBE
