#include <cstdio>
#include <cstring>
#include "ovpCAlgorithmXMLScenarioExporter.h"

//___________________________________________________________________//
//                                                                   //

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

//___________________________________________________________________//
//                                                                   //

CAlgorithmXMLScenarioExporter::CAlgorithmXMLScenarioExporter() { m_writer = createWriter(*this); }
CAlgorithmXMLScenarioExporter::~CAlgorithmXMLScenarioExporter() { m_writer->release(); }

void CAlgorithmXMLScenarioExporter::write(const char* str) { m_pMemoryBuffer->append(reinterpret_cast<const uint8_t*>(str), strlen(str)); }

bool CAlgorithmXMLScenarioExporter::exportStart(CMemoryBuffer& memoryBuffer, const CIdentifier& id)
{
    m_pMemoryBuffer = &memoryBuffer;

    CString name;

    if (id == OVTK_Algorithm_ScenarioExporter_NodeId_OpenViBEScenario) { name = "OpenViBE-Scenario"; }

    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Settings) { name = "Settings"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting) { name = "Setting"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_TypeID) { name = "TypeIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_DefaultValue) { name = "DefaultValue"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Value) { name = "Value"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connections) { name = "Connections"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection) { name = "Connection"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection_BoxID) { name = "BoxIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection_ParamName) { name = "BoxParameterName"; }


    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Inputs) { name = "Inputs"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input) { name = "Input"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_TypeID) { name = "TypeIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxID) { name = "LinkedBoxIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxInputIdx) { name = "LinkedBoxInputIndex"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxInputID) { name = "LinkedBoxInputIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Outputs) { name = "Outputs"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output) { name = "Output"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_TypeID) { name = "TypeIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxID) { name = "LinkedBoxIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxOutputIdx) { name = "LinkedBoxOutputIndex"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxOutputID) { name = "LinkedBoxOutputIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_FormatVersion) { name = "FormatVersion"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Creator) { name = "Creator"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_CreatorVersion) { name = "CreatorVersion"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Boxes) { name = "Boxes"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box) { name = "Box"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_AlgorithmClassIdD) { name = "AlgorithmClassIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Inputs) { name = "Inputs"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input) { name = "Input"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_TypeID) { name = "TypeIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Outputs) { name = "Outputs"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output) { name = "Output"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_TypeID) { name = "TypeIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Settings) { name = "Settings"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting) { name = "Setting"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_TypeID) { name = "TypeIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Name) { name = "Name"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_DefaultValue) { name = "DefaultValue"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Value) { name = "Value"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Modifiability) { name = "Modifiability"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attributes) { name = "Attributes"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute) { name = "Attribute"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute_Value) { name = "Value"; }

    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comments) { name = "Comments"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment) { name = "Comment"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Text) { name = "Text"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attributes) { name = "Attributes"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute) { name = "Attribute"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute_Value) { name = "Value"; }

    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Metadata) { name = "Metadata"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry) { name = "Entry"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_Type) { name = "Type"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_Data) { name = "Data"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Links) { name = "Links"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link) { name = "Link"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source) { name = "Source"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxID) { name = "BoxIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxOutputIdx) { name = "BoxOutputIndex"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxOutputID) { name = "BoxOutputIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target) { name = "Target"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxID) { name = "BoxIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxInputIdx) { name = "BoxInputIndex"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxInputID) { name = "BoxInputIdentifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attributes) { name = "Attributes"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute) { name = "Attribute"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute_Value) { name = "Value"; }

    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attributes) { name = "Attributes"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute) { name = "Attribute"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute_ID) { name = "Identifier"; }
    else if (id == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute_Value) { name = "Value"; }
        //
    else { OV_ERROR_KRF("(start) Unexpected node identifier " << id.str(), Kernel::ErrorType::BadArgument); }

    m_writer->openChild(name.toASCIIString());

    return true;
}

bool CAlgorithmXMLScenarioExporter::exportIdentifier(CMemoryBuffer& memoryBuffer, const CIdentifier& id, const CIdentifier& value)
{
    m_pMemoryBuffer = &memoryBuffer;
    OV_ERROR_UNLESS_KRF(this->exportStart(memoryBuffer, id), "Exporting identifier failed", Kernel::ErrorType::Internal);
    m_writer->setChildData(value.str().c_str());
    this->exportStop(memoryBuffer);
    return true;
}

bool CAlgorithmXMLScenarioExporter::exportString(CMemoryBuffer& memoryBuffer, const CIdentifier& id, const CString& value)
{
    m_pMemoryBuffer = &memoryBuffer;
    OV_ERROR_UNLESS_KRF(this->exportStart(memoryBuffer, id), "Exporting string failed", Kernel::ErrorType::Internal);
    m_writer->setChildData(value.toASCIIString());
    this->exportStop(memoryBuffer);
    return true;
}

bool CAlgorithmXMLScenarioExporter::exportUInteger(CMemoryBuffer& memoryBuffer, const CIdentifier& id, const uint64_t value)
{
    m_pMemoryBuffer = &memoryBuffer;
    OV_ERROR_UNLESS_KRF(this->exportStart(memoryBuffer, id), "Exporting uint failed", Kernel::ErrorType::Internal);
    m_writer->setChildData(std::to_string(value).c_str());
    this->exportStop(memoryBuffer);
    return true;
}

bool CAlgorithmXMLScenarioExporter::exportStop(CMemoryBuffer& memoryBuffer)
{
    m_pMemoryBuffer = &memoryBuffer;
    m_writer->closeChild();
    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
