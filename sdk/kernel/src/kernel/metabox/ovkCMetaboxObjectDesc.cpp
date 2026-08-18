#include "ovkCMetaboxObjectDesc.h"

namespace OpenViBE {
namespace Metabox {

CMetaboxObjectDesc::CMetaboxObjectDesc(const CString& rMetaboxDescriptor, Kernel::IScenario& metaboxScenario)
    : m_metaboxDesc(rMetaboxDescriptor)
      , m_name(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_Name))
      , m_authorName(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_Author))
      , m_authorCompanyName(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_Company))
      , m_shortDesc(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_ShortDescription))
      , m_detailedDesc(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_DetailedDescription))
      , m_category(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_Category))
      , m_version(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_Version))
      , m_stockItemName("")
      , m_metaboxID(metaboxScenario.getAttributeValue(OVP_AttributeId_Metabox_ID))
{
    for (size_t scenarioInputIdx = 0; scenarioInputIdx < metaboxScenario.getInputCount(); ++scenarioInputIdx)
    {
        CString name;
        CIdentifier typeID;
        CIdentifier id;

        metaboxScenario.getInputType(scenarioInputIdx, typeID);
        metaboxScenario.getInputName(scenarioInputIdx, name);
        metaboxScenario.getInterfacorIdentifier(Kernel::EBoxInterfacorType::Input, scenarioInputIdx, id);

        m_inputs.push_back(io_stream_t(name, typeID, id));
    }

    for (size_t scenarioOutputIdx = 0; scenarioOutputIdx < metaboxScenario.getOutputCount(); ++scenarioOutputIdx)
    {
        CString name;
        CIdentifier typeID;
        CIdentifier id;

        metaboxScenario.getOutputType(scenarioOutputIdx, typeID);
        metaboxScenario.getOutputName(scenarioOutputIdx, name);
        metaboxScenario.getInterfacorIdentifier(Kernel::EBoxInterfacorType::Output, scenarioOutputIdx, id);

        m_outputs.push_back(io_stream_t(name, typeID, id));
    }

    for (size_t scenarioSettingIdx = 0; scenarioSettingIdx < metaboxScenario.getSettingCount(); ++scenarioSettingIdx)
    {
        CString name;
        CIdentifier typeID;
        CString defaultValue;
        CIdentifier id;

        metaboxScenario.getSettingName(scenarioSettingIdx, name);
        metaboxScenario.getSettingType(scenarioSettingIdx, typeID);
        metaboxScenario.getSettingDefaultValue(scenarioSettingIdx, defaultValue);
        metaboxScenario.getInterfacorIdentifier(Kernel::EBoxInterfacorType::Setting, scenarioSettingIdx, id);

        m_settings.push_back(setting_t(name, typeID, defaultValue, id));
    }
}

bool CMetaboxObjectDesc::getBoxPrototype(Kernel::IBoxProto& prototype) const
{
    for (const auto& input : m_inputs) { prototype.addInput(input.m_name, input.m_typeID, input.m_id); }
    for (const auto& output : m_outputs) { prototype.addOutput(output.m_name, output.m_typeID, output.m_id); }
    for (const auto& setting : m_settings) { prototype.addSetting(setting.m_name, setting.m_typeID, setting.m_defaultValue, false, setting.m_id); }
    return true;
}

}  // namespace Metabox
}  // namespace OpenViBE
