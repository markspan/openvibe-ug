#pragma once

#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <xml/IWriter.h>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CAlgorithmXMLScenarioExporter final : public Toolkit::CAlgorithmScenarioExporter, public XML::IWriterCallback
{
public:
    CAlgorithmXMLScenarioExporter();
    ~CAlgorithmXMLScenarioExporter() override;
    bool exportStart(CMemoryBuffer& memoryBuffer, const CIdentifier& id) override;
    bool exportIdentifier(CMemoryBuffer& memoryBuffer, const CIdentifier& id, const CIdentifier& value) override;
    bool exportString(CMemoryBuffer& memoryBuffer, const CIdentifier& id, const CString& value) override;
    bool exportUInteger(CMemoryBuffer& memoryBuffer, const CIdentifier& id, uint64_t value) override;
    bool exportStop(CMemoryBuffer& memoryBuffer) override;

    _IsDerivedFromClass_Final_(Toolkit::CAlgorithmScenarioExporter, OVP_ClassId_Algorithm_XMLScenarioExporter)

protected:
    void write(const char* str) override; // XML::IWriterCallback

    XML::IWriter* m_writer         = nullptr;
    CMemoryBuffer* m_pMemoryBuffer = nullptr;
};

class CAlgorithmXMLScenarioExporterDesc final : public Toolkit::CAlgorithmScenarioExporterDesc
{
public:
    void release() override { }

    CString getName() const override { return "XML Scenario exporter"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "A sample XML scenario exporter"; }
    CString getDetailedDescription() const override { return "This scenario exporter uses simple XML format to output the scenario"; }
    CString getCategory() const override { return "File reading and writing/XML Scenario"; }
    CString getVersion() const override { return "1.0"; }
    // virtual CString getFileExtension() const       { return "xml;XML"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_XMLScenarioExporter; }
    IPluginObject* create() override { return new CAlgorithmXMLScenarioExporter(); }

    _IsDerivedFromClass_Final_(Toolkit::CAlgorithmScenarioExporterDesc, OVP_ClassId_Algorithm_XMLScenarioExporterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
