#pragma once

#include "../ovkTKernelObject.h"

#include <map>
#include <string>
#include <mutex>

namespace OpenViBE {
namespace Kernel {
class IConfigurationKeywordExpandCallback;

typedef struct
{
    CString name;
    CString value;
} config_token_t;

class CConfigurationManager final : public TKernelObject<IConfigurationManager>
{
public:

    CConfigurationManager(const IKernelContext& ctx, IConfigurationManager* parentConfigManager = nullptr);
    void clear() override;
    bool addConfigurationFromFile(const CString& rFileNameWildCard) override;
    CIdentifier createConfigurationToken(const CString& name, const CString& value) override;
    bool releaseConfigurationToken(const CIdentifier& identifier) override;
    CIdentifier getNextConfigurationTokenIdentifier(const CIdentifier& prevConfigTokenID) const override;
    CString getConfigurationTokenName(const CIdentifier& identifier) const override;
    CString getConfigurationTokenValue(const CIdentifier& identifier) const override;
    bool setConfigurationTokenName(const CIdentifier& identifier, const CString& name) override;
    bool setConfigurationTokenValue(const CIdentifier& identifier, const CString& value) override;
    bool addOrReplaceConfigurationToken(const CString& name, const CString& value) override;
    CIdentifier lookUpConfigurationTokenIdentifier(const CString& name, bool recursive) const override;
    CString lookUpConfigurationTokenValue(const CString& name) const override;
    bool registerKeywordParser(const CString& keyword, const IConfigurationKeywordExpandCallback& callback) override;
    bool unregisterKeywordParser(const CString& keyword) override;
    bool unregisterKeywordParser(const IConfigurationKeywordExpandCallback& callback) override;
    CString expand(const CString& expression) const override;

    _IsDerivedFromClass_Final_(TKernelObject<IConfigurationManager>, OVK_ClassId_Kernel_Config_ConfigManager)
    CString expandOnlyKeyword(const CString& rKeyword, const CString& expression, bool preserveBackslashes) const override;
    double expandAsFloat(const CString& expression, double fallbackValue) const override;
    int64_t expandAsInteger(const CString& expression, int64_t fallbackValue) const override;
    uint64_t expandAsUInteger(const CString& expression, uint64_t fallbackValue) const override;
    bool expandAsBoolean(const CString& expression, bool fallbackValue) const override;
    uint64_t expandAsEnumerationEntryValue(const CString& expression, const CIdentifier& enumTypeID, uint64_t fallbackValue) const override;

protected:

    CIdentifier getUnusedIdentifier() const;

    bool internalExpand(const std::string& sValue, std::string& result) const;
    bool internalExpandOnlyKeyword(const std::string& sKeyword, const std::string& sValue, std::string& sResult, bool preserveBackslashes) const;
    bool internalGetConfigurationTokenValueFromName(const std::string& name, std::string& value) const;

    IConfigurationManager* m_parentConfigManager = nullptr;
    mutable size_t m_idx;
    mutable size_t m_startTime;

    static size_t getRandom();
    size_t getIndex() const;
    static CString getTime();
    static CString getDate();
    size_t getRealTime() const;
    static size_t getProcessId();

    std::map<CIdentifier, config_token_t> m_ConfigTokens;
    std::map<CString, const IConfigurationKeywordExpandCallback*> m_keywordOverrides;

    mutable std::recursive_mutex m_mutex;
};
}  // namespace Kernel
}  // namespace OpenViBE
