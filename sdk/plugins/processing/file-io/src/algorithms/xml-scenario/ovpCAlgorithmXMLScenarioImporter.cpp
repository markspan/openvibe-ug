#include "ovpCAlgorithmXMLScenarioImporter.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/validators/common/Grammar.hpp>

XERCES_CPP_NAMESPACE_USE

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

namespace {
class _AutoBind_
{
public:
    explicit _AutoBind_(const std::string& value) : m_value(value) { }
    operator CString() const { return m_value.c_str(); }

    operator CIdentifier() const
    {
        CIdentifier res;
        res.fromString(m_value);
        return res;
    }

    operator size_t() { return atoi(m_value.c_str()); }
protected:
    const std::string& m_value;
};

std::string xercesToString(const XMLCh* xercesString)
{
    std::string transcodedString(XMLString::transcode(xercesString));
    return std::string{transcodedString.c_str()};
}

class CErrorHandler final : public HandlerBase
{
public:

    explicit CErrorHandler(Kernel::IAlgorithmContext& algorithmCtx)
        : m_algorithmContext(algorithmCtx) { }

    void fatalError(const SAXParseException& exception) override { this->error(exception); }

    void error(const SAXParseException& exception) override
    {
        // we just issue a trace here because the calling method
        // implements a fallback mechanism and we don't want to populate
        // the error manager if the importer returns gracefully.
        m_algorithmContext.getLogManager() << Kernel::LogLevel_Error << "Failed to validate xml: error [" << xercesToString(exception.getMessage())
                << "], line number [" << size_t(exception.getLineNumber()) << "]" << "\n";
    }

    void warning(const SAXParseException& exception) override
    {
        OV_WARNING("Warning while validating xml: warning [" << xercesToString(exception.getMessage()) << "], line number ["
                   << size_t(exception.getLineNumber()) << "]", m_algorithmContext.getLogManager());
    }

private:
    Kernel::IAlgorithmContext& m_algorithmContext;
};
} //namespace

CAlgorithmXMLScenarioImporter::CAlgorithmXMLScenarioImporter() { m_reader = createReader(*this); }
CAlgorithmXMLScenarioImporter::~CAlgorithmXMLScenarioImporter() { m_reader->release(); }

void CAlgorithmXMLScenarioImporter::openChild(const char* name, const char** /*attributeName*/, const char** /*attributeValue*/, const size_t /*nAttribute*/)
{
    m_nodes.push(name);

    std::string& top = m_nodes.top();

    if (top == "OpenViBE-Scenario" && m_status == EParsingStatus::Nothing)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_OpenViBEScenario);
    }
    else if (top == "Attribute" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::ScenarioAttribute;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute);
    }
    else if (top == "Setting" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::ScenarioSetting;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting);
    } 
    else if (top == "Connection" && m_status == EParsingStatus::ScenarioSetting)
    {
        m_status = EParsingStatus::ScenarioSettingConnection;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection);
    }
    else if (top == "Input" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::ScenarioInput;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input);
    }
    else if (top == "Output" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::ScenarioOutput;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output);
    }

    else if (top == "Box" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::Box;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Box);
    }
    else if (top == "Input" && m_status == EParsingStatus::Box)
    {
        m_status = EParsingStatus::BoxInput;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input);
    }
    else if (top == "Output" && m_status == EParsingStatus::Box)
    {
        m_status = EParsingStatus::BoxOutput;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output);
    }
    else if (top == "Setting" && m_status == EParsingStatus::Box)
    {
        m_status = EParsingStatus::BoxSetting;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting);
    }
    else if (top == "Attribute" && m_status == EParsingStatus::Box)
    {
        m_status = EParsingStatus::BoxAttribute;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute);
    }

    else if (top == "Comment" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::Comment;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Comment);
    }
    else if (top == "Attribute" && m_status == EParsingStatus::Comment)
    {
        m_status = EParsingStatus::CommentAttribute;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute);
    }

    else if (top == "Entry" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::MetadataEntry;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry);
    }

    else if (top == "Link" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::Link;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Link);
    }
    else if (top == "Source" && m_status == EParsingStatus::Link)
    {
        m_status = EParsingStatus::LinkSource;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source);
    }
    else if (top == "Target" && m_status == EParsingStatus::Link)
    {
        m_status = EParsingStatus::LinkTarget;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target);
    }
    else if (top == "Attribute" && m_status == EParsingStatus::Link)
    {
        m_status = EParsingStatus::LinkAttribute;
        m_ctx->processStart(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute);
    }
}

void CAlgorithmXMLScenarioImporter::processChildData(const char* data)
{
    std::string& top = m_nodes.top();

    switch (m_status)
    {
        case EParsingStatus::Box:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_ID, _AutoBind_(data)); }
            if (top == "AlgorithmClassIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_AlgorithmClassIdD, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Name, _AutoBind_(data)); }
            break;
        case EParsingStatus::BoxInput:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_ID, _AutoBind_(data)); }
            if (top == "TypeIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_TypeID, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_Name, _AutoBind_(data)); }
            break;
        case EParsingStatus::BoxOutput:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_ID, _AutoBind_(data)); }
            if (top == "TypeIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_TypeID, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_Name, _AutoBind_(data)); }
            break;
        case EParsingStatus::BoxSetting:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_ID, _AutoBind_(data)); }
            if (top == "TypeIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_TypeID, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Name, _AutoBind_(data)); }
            if (top == "DefaultValue") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_DefaultValue, _AutoBind_(data)); }
            if (top == "Value") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Value, _AutoBind_(data)); }
            if (top == "Modifiability") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Modifiability, _AutoBind_(data)); }
            break;
        case EParsingStatus::BoxAttribute:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute_ID, _AutoBind_(data)); }
            if (top == "Value") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute_Value, _AutoBind_(data)); }
            break;

        case EParsingStatus::Comment:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Comment_ID, _AutoBind_(data)); }
            if (top == "Text") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Text, _AutoBind_(data)); }
            break;

        case EParsingStatus::MetadataEntry:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_ID, _AutoBind_(data)); }
            if (top == "Type") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_Type, _AutoBind_(data)); }
            if (top == "Data") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_Data, _AutoBind_(data)); }
            break;

        case EParsingStatus::CommentAttribute:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute_ID, _AutoBind_(data)); }
            if (top == "Value") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute_Value, _AutoBind_(data)); }
            break;

        case EParsingStatus::Link:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Link_ID, _AutoBind_(data)); }
            break;
        case EParsingStatus::LinkSource:
            if (top == "BoxIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxID, _AutoBind_(data)); }
            if (top == "BoxOutputIndex") { m_ctx->processUInteger(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxOutputIdx, _AutoBind_(data)); }
            if (top == "BoxOutputIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxOutputID, _AutoBind_(data)); }
            break;
        case EParsingStatus::LinkTarget:
            if (top == "BoxIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxID, _AutoBind_(data)); }
            if (top == "BoxInputIndex") { m_ctx->processUInteger(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxInputIdx, _AutoBind_(data)); }
            if (top == "BoxInputIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxInputID, _AutoBind_(data)); }
            break;
        case EParsingStatus::LinkAttribute:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute_ID, _AutoBind_(data)); }
            if (top == "Value") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute_Value, _AutoBind_(data)); }
            break;

        case EParsingStatus::ScenarioSetting:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_ID, _AutoBind_(data)); }
            if (top == "TypeIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_TypeID, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Name, _AutoBind_(data)); }
            if (top == "DefaultValue") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_DefaultValue, _AutoBind_(data)); }
            if (top == "Value") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Value, _AutoBind_(data)); }
            break;

        case EParsingStatus::ScenarioSettingConnection:
            if (top == "BoxIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection_BoxID, _AutoBind_(data)); }
            if (top == "BoxParameterName") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection_ParamName, _AutoBind_(data)); }
            break;
        case EParsingStatus::ScenarioInput:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_ID, _AutoBind_(data)); }
            if (top == "TypeIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_TypeID, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_Name, _AutoBind_(data)); }
            if (top == "LinkedBoxIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxID, _AutoBind_(data)); }
            if (top == "LinkedBoxInputIndex")
            {
                m_ctx->processUInteger(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxInputIdx, _AutoBind_(data));
            }
            if (top == "LinkedBoxInputIdentifier")
            {
                m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxInputID, _AutoBind_(data));
            }
            break;

        case EParsingStatus::ScenarioOutput:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_ID, _AutoBind_(data)); }
            if (top == "TypeIdentifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_TypeID, _AutoBind_(data)); }
            if (top == "Name") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_Name, _AutoBind_(data)); }
            if (top == "LinkedBoxIdentifier")
            {
                m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxID, _AutoBind_(data));
            }
            if (top == "LinkedBoxOutputIndex")
            {
                m_ctx->processUInteger(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxOutputIdx, _AutoBind_(data));
            }
            if (top == "LinkedBoxOutputIdentifier")
            {
                m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxOutputID, _AutoBind_(data));
            }
            break;

        case EParsingStatus::ScenarioAttribute:
            if (top == "Identifier") { m_ctx->processIdentifier(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute_ID, _AutoBind_(data)); }
            if (top == "Value") { m_ctx->processString(OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute_Value, _AutoBind_(data)); }
            break;
        default: break;
    }
}

void CAlgorithmXMLScenarioImporter::closeChild()
{
    std::string& top = m_nodes.top();

    if (top == "OpenViBE-Scenario" && m_status == EParsingStatus::Scenario)
    {
        m_status = EParsingStatus::Nothing;
        m_ctx->processStop();
    }
    else if (top == "Setting" && m_status == EParsingStatus::ScenarioSetting)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }
    else if (top == "Connection" && m_status == EParsingStatus::ScenarioSettingConnection) {
        m_status = EParsingStatus::ScenarioSetting;
        m_ctx->processStop();
    }
    else if (top == "Input" && m_status == EParsingStatus::ScenarioInput)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }
    else if (top == "Output" && m_status == EParsingStatus::ScenarioOutput)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }
    else if (top == "Attribute" && m_status == EParsingStatus::ScenarioAttribute)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }

    else if (top == "Box" && m_status == EParsingStatus::Box)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }
    else if (top == "Input" && m_status == EParsingStatus::BoxInput)
    {
        m_status = EParsingStatus::Box;
        m_ctx->processStop();
    }

    else if (top == "Output" && m_status == EParsingStatus::BoxOutput)
    {
        m_status = EParsingStatus::Box;
        m_ctx->processStop();
    }
    else if (top == "Setting" && m_status == EParsingStatus::BoxSetting)
    {
        m_status = EParsingStatus::Box;
        m_ctx->processStop();
    }
    else if (top == "Attribute" && m_status == EParsingStatus::BoxAttribute)
    {
        m_status = EParsingStatus::Box;
        m_ctx->processStop();
    }

    else if (top == "Comment" && m_status == EParsingStatus::Comment)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }
    else if (top == "Attribute" && m_status == EParsingStatus::CommentAttribute)
    {
        m_status = EParsingStatus::Comment;
        m_ctx->processStop();
    }

    else if (top == "Entry" && m_status == EParsingStatus::MetadataEntry)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }

    else if (top == "Link" && m_status == EParsingStatus::Link)
    {
        m_status = EParsingStatus::Scenario;
        m_ctx->processStop();
    }
    else if (top == "Source" && m_status == EParsingStatus::LinkSource)
    {
        m_status = EParsingStatus::Link;
        m_ctx->processStop();
    }
    else if (top == "Target" && m_status == EParsingStatus::LinkTarget)
    {
        m_status = EParsingStatus::Link;
        m_ctx->processStop();
    }
    else if (top == "Attribute" && m_status == EParsingStatus::LinkAttribute)
    {
        m_status = EParsingStatus::Link;
        m_ctx->processStop();
    }

    m_nodes.pop();
}

bool CAlgorithmXMLScenarioImporter::validateXML(const unsigned char* buffer, const size_t size)
{
    // implementation of the fallback mechanism

    // error manager is used to differentiate errors from invalid xml
    this->getErrorManager().releaseErrors();

    if (this->validateXMLAgainstSchema((Directories::getDataDir() + "/kernel/openvibe-scenario-v3.xsd"), buffer, size)) {
        return true;
    }
    if (this->getErrorManager().hasError())
    {
        // this is not a validation error thus we return directly
        return false;
    }

    if (this->validateXMLAgainstSchema((Directories::getDataDir() + "/kernel/openvibe-scenario-v2.xsd"), buffer, size)) 
    {
        this->getLogManager() << Kernel::LogLevel_Error << 
                "Importing scenario with v2 format: v2 scenario is deprecated. Please update to v3 format to continue\n" 
                << " You can update your scenarios with the tool provided in the OpenViBE suite: openvibe-scenario-update-2to3";
        return false;
    }
    if (this->validateXMLAgainstSchema((Directories::getDataDir() + "/kernel/openvibe-scenario-v1.xsd"), buffer, size))
    {
        this->getLogManager() << Kernel::LogLevel_Trace <<
                "Importing scenario with legacy format: v1 scenario might be deprecated in the future so upgrade to v2 format when possible\n";
        return true;
    }
    if (this->getErrorManager().hasError())
    {
        // this is not a validation error thus we return directly
        return false;
    }

    if (this->validateXMLAgainstSchema((Directories::getDataDir() + "/kernel/openvibe-scenario-legacy.xsd"), buffer, size))
    {
        OV_WARNING_K("Importing scenario with legacy format: legacy scenario might be deprecated in the future so upgrade to v3 format when possible");
        return true;
    }
    if (this->getErrorManager().hasError())
    {
        // this is not a validation error thus we return directly
        return false;
    }

    OV_ERROR_KRF("Failed to validate scenario against XSD schemas", Kernel::ErrorType::BadXMLSchemaValidation);
}

bool CAlgorithmXMLScenarioImporter::validateXMLAgainstSchema(const char* validationSchema, const unsigned char* buffer, const size_t size)
{
    this->getLogManager() << Kernel::LogLevel_Trace << "Validating XML against schema [" << validationSchema << "]\n";

    size_t errorCount;
    XMLPlatformUtils::Initialize();

    { // scope the content here to ensure unique_ptr contents are destroyed before the call to XMLPlatformUtils::Terminate();
        const std::unique_ptr<MemBufInputSource> xercesBuffer(new MemBufInputSource(buffer, size, "xml memory buffer"));

        std::unique_ptr<XercesDOMParser> parser(new XercesDOMParser());
        parser->setValidationScheme(XercesDOMParser::Val_Always);
        parser->setDoNamespaces(true);
        parser->setDoSchema(true);
        parser->setValidationConstraintFatal(true);
        parser->setValidationSchemaFullChecking(true);
        parser->setExternalNoNamespaceSchemaLocation(validationSchema);

        const std::unique_ptr<ErrorHandler> errorHandler(new CErrorHandler(this->getAlgorithmContext()));
        parser->setErrorHandler(errorHandler.get());

        parser->parse(*xercesBuffer);
        errorCount = parser->getErrorCount();
    }

    XMLPlatformUtils::Terminate();

    return (errorCount == 0);
}

bool CAlgorithmXMLScenarioImporter::import(IAlgorithmScenarioImporterContext& rContext, const CMemoryBuffer& memoryBuffer)
{
    m_ctx = &rContext;
    if (!this->validateXML(memoryBuffer.getDirectPointer(), memoryBuffer.getSize())) { return false; }  // error handling is handled in validateXML
    return m_reader->processData(memoryBuffer.getDirectPointer(), memoryBuffer.getSize());
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
