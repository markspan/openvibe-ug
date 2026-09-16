// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "ScenarioSerializer.h"

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

namespace OpenViBE {
namespace Designer {

ScenarioSerializer::ScenarioSerializer(Kernel::IKernelContext& ctx) : m_ctx(ctx) {}

QByteArray ScenarioSerializer::toXml(const Kernel::IScenario& scenario) const
{
    QByteArray xml;
    auto& algorithmMgr = m_ctx.getAlgorithmManager();
    const CIdentifier exporterID = algorithmMgr.createAlgorithm(OVP_GD_ClassId_Algorithm_XMLScenarioExporter);
    if (exporterID == CIdentifier::undefined())
    {
        m_ctx.getLogManager() << Kernel::LogLevel_Error << "Failed to create the XML scenario exporter\n";
    }
    else
    {
        Kernel::IAlgorithmProxy& exporter = algorithmMgr.getAlgorithm(exporterID);
        exporter.initialize();

        CMemoryBuffer memoryBuffer;
        Kernel::TParameterHandler<const Kernel::IScenario*> scenarioParam(exporter.getInputParameter(OV_Algorithm_ScenarioExporter_InputParameterId_Scenario));
        Kernel::TParameterHandler<CMemoryBuffer*> bufferParam(exporter.getOutputParameter(OV_Algorithm_ScenarioExporter_OutputParameterId_MemoryBuffer));
        scenarioParam = &scenario;
        bufferParam = &memoryBuffer;

        const bool success = exporter.process();
        exporter.uninitialize();
        algorithmMgr.releaseAlgorithm(exporter);

        if (!success)
        {
            m_ctx.getLogManager() << Kernel::LogLevel_Error << "Failed to export the scenario to XML\n";
        }
        else
        {
            xml = QByteArray(reinterpret_cast<const char*>(memoryBuffer.getDirectPointer()), int(memoryBuffer.getSize()));
        }
    }
    return xml;
}

bool ScenarioSerializer::fromXml(const QByteArray& xml, Kernel::IScenario& into) const
{
    bool res = false;
    if (!xml.isEmpty())
    {
        auto& algorithmMgr = m_ctx.getAlgorithmManager();
        const CIdentifier importerID = algorithmMgr.createAlgorithm(OVP_GD_ClassId_Algorithm_XMLScenarioImporter);
        if (importerID == CIdentifier::undefined())
        {
            m_ctx.getLogManager() << Kernel::LogLevel_Error << "Failed to create the XML scenario importer\n";
        }
        else
        {
            Kernel::IAlgorithmProxy& importer = algorithmMgr.getAlgorithm(importerID);
            importer.initialize();

            const CMemoryBuffer memoryBuffer(reinterpret_cast<const uint8_t*>(xml.constData()), size_t(xml.size()));
            Kernel::TParameterHandler<const CMemoryBuffer*> bufferParam(importer.getInputParameter(OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer));
            Kernel::TParameterHandler<Kernel::IScenario*> scenarioParam(importer.getOutputParameter(OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario));

            into.clear();
            bufferParam = &memoryBuffer;
            scenarioParam = &into;

            res = importer.process();
            importer.uninitialize();
            algorithmMgr.releaseAlgorithm(importer);

            if (!res)
            {
                m_ctx.getLogManager() << Kernel::LogLevel_Error << "Failed to import the scenario from XML\n";
            }
        }
    }
    return res;
}

}  // namespace Designer
}  // namespace OpenViBE
