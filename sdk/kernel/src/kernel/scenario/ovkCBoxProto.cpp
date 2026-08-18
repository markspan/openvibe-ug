#include "ovkCBoxProto.h"

namespace OpenViBE {
namespace Kernel {

bool CBoxProto::addInput(const CString& name, const CIdentifier& typeID, const CIdentifier& id, const bool notify)
{
    if (!m_box.addInput(name, typeID, id, notify)) { return false; }

    const std::string tmp = std::to_string(m_box.getInputCount());
    const CString buffer(tmp.c_str());
    if (m_box.hasAttribute(OV_AttributeId_Box_InitialInputCount)) { m_box.setAttributeValue(OV_AttributeId_Box_InitialInputCount, buffer); }
    else { m_box.addAttribute(OV_AttributeId_Box_InitialInputCount, buffer); }

    return true;
}

bool CBoxProto::addOutput(const CString& name, const CIdentifier& typeID, const CIdentifier& id, const bool notify)
{
    if (!m_box.addOutput(name, typeID, id, notify)) { return false; }

    const std::string tmp = std::to_string(m_box.getOutputCount());
    const CString buffer(tmp.c_str());
    if (m_box.hasAttribute(OV_AttributeId_Box_InitialOutputCount)) { m_box.setAttributeValue(OV_AttributeId_Box_InitialOutputCount, buffer); }
    else { m_box.addAttribute(OV_AttributeId_Box_InitialOutputCount, buffer); }

    return true;
}

bool CBoxProto::addSetting(const CString& name, const CIdentifier& typeID, const CString& value, const bool modifiable,
                           const CIdentifier& id, const bool notify)
{
    if (!m_box.addSetting(name, typeID, value, size_t(-1), modifiable, id, notify)) { return false; }

    const std::string tmp = std::to_string(m_box.getSettingCount());
    const CString buffer(tmp.c_str());
    if (m_box.hasAttribute(OV_AttributeId_Box_InitialSettingCount)) { m_box.setAttributeValue(OV_AttributeId_Box_InitialSettingCount, buffer); }
    else { m_box.addAttribute(OV_AttributeId_Box_InitialSettingCount, buffer); }

    return true;
}
/*
size_t CBoxProto::addSetting(const CString& name, const CIdentifier& typeID, const CString& sDefaultValue, const bool bModifiable)
{
    addSetting(name, typeID, sDefaultValue);
    size_t  lastSetting = m_box.getSettingCount();
    m_box.setSettingMod(lastSetting, bModifiable);
    return true;
}
/*/

bool CBoxProto::addFlag(const EBoxFlag boxFlag)
{
    switch (boxFlag)
    {
        case BoxFlag_CanAddInput: m_box.addAttribute(OV_AttributeId_Box_FlagCanAddInput, "");
            break;
        case BoxFlag_CanModifyInput: m_box.addAttribute(OV_AttributeId_Box_FlagCanModifyInput, "");
            break;
        case BoxFlag_CanAddOutput: m_box.addAttribute(OV_AttributeId_Box_FlagCanAddOutput, "");
            break;
        case BoxFlag_CanModifyOutput: m_box.addAttribute(OV_AttributeId_Box_FlagCanModifyOutput, "");
            break;
        case BoxFlag_CanAddSetting: m_box.addAttribute(OV_AttributeId_Box_FlagCanAddSetting, "");
            break;
        case BoxFlag_CanModifySetting: m_box.addAttribute(OV_AttributeId_Box_FlagCanModifySetting, "");
            break;
        case BoxFlag_ManualUpdate: m_box.addAttribute(OV_AttributeId_Box_FlagNeedsManualUpdate, "");
            break;
        case BoxFlag_IsDeprecated: break;
    }
    return true;
}

bool CBoxProto::addFlag(const CIdentifier& flagID)
{
    const uint64_t value = getKernelContext().getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_BoxAlgorithmFlag, flagID.toString());
    if (value == CIdentifier::undefined().id()) { return false; }
    m_box.addAttribute(flagID, "");
    return true;
}

}  // namespace Kernel
}  // namespace OpenViBE
