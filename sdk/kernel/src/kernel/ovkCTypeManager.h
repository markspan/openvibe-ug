#pragma once

#include "ovkTKernelObject.h"

#include <map>
#include <set>
#include <vector>
#include <mutex>

namespace OpenViBE {
namespace Kernel {
class CTypeManager final : public TKernelObject<ITypeManager>
{
public:

    explicit CTypeManager(const IKernelContext& ctx);
    CIdentifier getNextTypeIdentifier(const CIdentifier& previousID) const override;
    std::vector<std::pair<CIdentifier, CString>> getSortedTypes() const override;
    bool registerType(const CIdentifier& typeID, const CString& name) override;
    bool registerStreamType(const CIdentifier& typeID, const CString& name, const CIdentifier& parentTypeID) override;
    bool registerEnumerationType(const CIdentifier& typeID, const CString& name) override;
    bool registerEnumerationEntry(const CIdentifier& typeID, const CString& name, uint64_t value) override;
    bool registerBitMaskType(const CIdentifier& typeID, const CString& name) override;
    bool registerBitMaskEntry(const CIdentifier& typeID, const CString& name, uint64_t value) override;
    bool isRegistered(const CIdentifier& typeID) const override;
    bool isStream(const CIdentifier& typeID) const override;
    bool isDerivedFromStream(const CIdentifier& typeID, const CIdentifier& parentTypeID) const override;
    bool isEnumeration(const CIdentifier& typeID) const override;
    bool isBitMask(const CIdentifier& typeID) const override;
    CString getTypeName(const CIdentifier& typeID) const override;
    CIdentifier getTypeFromName(const CString& name) const override;
    CIdentifier getStreamParentType(const CIdentifier& typeID) const override;
    size_t getEnumerationEntryCount(const CIdentifier& typeID) const override;
    bool getEnumerationEntry(const CIdentifier& typeID, uint64_t index, CString& name, uint64_t& value) const override;
    CString getEnumerationEntryNameFromValue(const CIdentifier& typeID, uint64_t value) const override;
    uint64_t getEnumerationEntryValueFromName(const CIdentifier& typeID, const CString& name) const override;
    size_t getBitMaskEntryCount(const CIdentifier& typeID) const override;
    bool getBitMaskEntry(const CIdentifier& typeID, uint64_t index, CString& name, uint64_t& value) const override;
    CString getBitMaskEntryNameFromValue(const CIdentifier& typeID, uint64_t value) const override;
    uint64_t getBitMaskEntryValueFromName(const CIdentifier& typeID, const CString& name) const override;
    CString getBitMaskEntryCompositionNameFromValue(const CIdentifier& typeID, uint64_t value) const override;
    uint64_t getBitMaskEntryCompositionValueFromName(const CIdentifier& typeID, const CString& name) const override;
    bool evaluateSettingValue(CString value, double& result) const override;

    _IsDerivedFromClass_Final_(TKernelObject<ITypeManager>, OVK_ClassId_Kernel_TypeManager)

protected:

    std::map<CIdentifier, CString> m_names;
    std::set<CString> m_takenNames;
    std::map<CIdentifier, std::map<uint64_t, CString>> m_enumerations;
    std::map<CIdentifier, std::map<uint64_t, CString>> m_bitMasks;
    std::map<CIdentifier, CIdentifier> m_streams;

    mutable std::recursive_mutex m_mutex;
};
}  // namespace Kernel
}  // namespace OpenViBE
