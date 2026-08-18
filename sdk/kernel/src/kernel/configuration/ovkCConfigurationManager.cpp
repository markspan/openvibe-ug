#include "ovkCConfigurationManager.h"

#include <openvibe/kernel/configuration/ovIConfigurationKeywordExpandCallback.h>

#include <fs/IEntryEnumerator.h>
#include <fs/Files.h>

#include <system/ovCTime.h>
#include <system/ovCMath.h>

#include <stack>
#include <string>
#include <fstream>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <ctime>
#include <climits>
#include <cstdlib>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <unistd.h> // for getpid
#elif defined TARGET_OS_Windows
#include <windows.h> // for GetCurrentProcessId
#else
#endif

namespace OpenViBE {
namespace Kernel {
// because std::tolower has multiple signatures,
// it can not be easily used in std::transform
// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
template <class TCharT>
static TCharT ToLower(TCharT c) { return std::tolower(c); }

class CConfigurationManagerEntryEnumeratorCallBack final : public FS::IEntryEnumeratorCallBack
{
public:

    CConfigurationManagerEntryEnumeratorCallBack(ILogManager& logManager, CErrorManager& errorManager, IConfigurationManager& configManger)
        : m_logManager(logManager), m_errorManager(errorManager), m_configManager(configManger) { }

    static std::string reduce(const std::string& value)
    {
        if (value.length() == 0) { return ""; }

        size_t i = 0;
        size_t j = value.length() - 1;

        while (i < value.length() && (value[i] == '\t' || value[i] == ' ')) { i++; }
        while (j >= i && (value[j] == '\t' || value[j] == ' ')) { j--; }

        return value.substr(i, j - i + 1);
    }

    bool callback(FS::IEntryEnumerator::IEntry& rEntry, FS::IEntryEnumerator::IAttributes& /*attributes*/) override
    {
        std::ifstream file;
        FS::Files::openIFStream(file, rEntry.getName());

        OV_ERROR_UNLESS(file.good(), "Could not open file " << rEntry.getName(), Kernel::ErrorType::ResourceNotFound, false, m_errorManager, m_logManager);
        m_logManager << LogLevel_Trace << "Processing configuration file " << rEntry.getName() << "\n";

        do
        {
            std::string line;
            std::string linePart;
            size_t eq;

            while (!file.eof() && (line.length() == 0 || line[line.length() - 1] == '\\'))
            {
                while (line.length() != 0 && line[line.length() - 1] == '\\')
                {
                    line.resize(line.length() - 1); // removes ending backslashes
                }

                std::getline(file, linePart, '\n');
                line += reduce(linePart);
            }

            // process everything except empty line or comment
            if (!line.empty() && line[0] != '\0' && line[0] != '#')
            {
                OV_ERROR_UNLESS((eq=line.find('=')) != std::string::npos,
                                "Invalid syntax in configuration file " << CString(rEntry.getName()) << " : " << line,
                                ErrorType::BadFileParsing, false, m_errorManager, m_logManager);

                std::string name(reduce(line.substr(0, eq)));
                std::string value(reduce(line.substr(eq + 1, line.length() - eq)));
                if (name == "Include")
                {
                    CString wildCard = m_configManager.expand(value.c_str());
                    m_logManager << LogLevel_Trace << "Including configuration file " << wildCard << "...\n";
                    m_configManager.addConfigurationFromFile(wildCard);
                    m_logManager << LogLevel_Trace << "Including configuration file " << wildCard << " done...\n";
                }
                else
                {
                    CIdentifier tokenID = m_configManager.lookUpConfigurationTokenIdentifier(name.c_str());
                    if (tokenID == CIdentifier::undefined())
                    {
                        m_logManager << LogLevel_Trace << "Adding configuration token " << name << " : " << value << "\n";
                        m_configManager.createConfigurationToken(name.c_str(), value.c_str());
                    }
                    else
                    {
                        m_logManager << LogLevel_Trace << "Changing configuration token " << name << " to " << value << "\n";

                        // warning if base token are overwritten here
                        OV_WARNING_UNLESS(name != "Path_UserData" && name != "Path_Log" && name != "Path_Tmp"
                                          && name != "Path_Lib" && name != "Path_Bin" && name != "OperatingSystem",
                                          "Overwriting critical token " + name, m_logManager);

                        m_configManager.setConfigurationTokenValue(tokenID, value.c_str());
                    }
                }
            }
        } while (!file.eof());

        m_logManager << LogLevel_Trace << "Processing configuration file " << CString(rEntry.getName()) << " finished\n";

        return true;
    }

protected:

    ILogManager& m_logManager;
    CErrorManager& m_errorManager;
    IConfigurationManager& m_configManager;
};

CConfigurationManager::CConfigurationManager(const IKernelContext& ctx, IConfigurationManager* parentConfigManager)
    : TKernelObject<IConfigurationManager>(ctx), m_parentConfigManager(parentConfigManager)
{
    m_idx       = 0;
    m_startTime = System::Time::getTime();
}

void CConfigurationManager::clear()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_ConfigTokens.clear();
}

bool CConfigurationManager::addConfigurationFromFile(const CString& rFileNameWildCard)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    this->getLogManager() << LogLevel_Trace << "Adding configuration file(s) [" << rFileNameWildCard << "]\n";


    CConfigurationManagerEntryEnumeratorCallBack cb(getKernelContext().getLogManager(), getKernelContext().getErrorManager(), *this);
    FS::IEntryEnumerator* entryEnumerator = createEntryEnumerator(cb);
    const bool res                        = entryEnumerator->enumerate(rFileNameWildCard);
    entryEnumerator->release();
    return res;
}

// ----------------------------------------------------------------------------------------------------------------------------
//

CIdentifier CConfigurationManager::createConfigurationToken(const CString& name, const CString& value)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(this->lookUpConfigurationTokenIdentifier(name, false) == CIdentifier::undefined(),
                        "Configuration token name " << name << " already exists", Kernel::ErrorType::BadResourceCreation);

    CIdentifier id           = this->getUnusedIdentifier();
    m_ConfigTokens[id].name  = name;
    m_ConfigTokens[id].value = value;
    return id;
}

bool CConfigurationManager::releaseConfigurationToken(const CIdentifier& identifier)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_ConfigTokens.find(identifier);

    OV_ERROR_UNLESS_KRF(it != m_ConfigTokens.end(), "Configuration token not found " << identifier.str(), Kernel::ErrorType::ResourceNotFound);

    m_ConfigTokens.erase(it);
    return true;
}

CIdentifier CConfigurationManager::getNextConfigurationTokenIdentifier(const CIdentifier& prevConfigTokenID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    std::map<CIdentifier, config_token_t>::const_iterator it;

    if (prevConfigTokenID == CIdentifier::undefined()) { it = m_ConfigTokens.begin(); }
    else
    {
        it = m_ConfigTokens.find(prevConfigTokenID);
        if (it == m_ConfigTokens.end()) { return CIdentifier::undefined(); }
        ++it;
    }

    return it != m_ConfigTokens.end() ? it->first : CIdentifier::undefined();
}

// ----------------------------------------------------------------------------------------------------------------------------

CString CConfigurationManager::getConfigurationTokenName(const CIdentifier& identifier) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_ConfigTokens.find(identifier);
    if (it != m_ConfigTokens.end()) { return it->second.name; }
    return "";
}

CString CConfigurationManager::getConfigurationTokenValue(const CIdentifier& identifier) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_ConfigTokens.find(identifier);
    if (it != m_ConfigTokens.end()) { return it->second.value; }
    return "";
}

// ----------------------------------------------------------------------------------------------------------------------------

bool CConfigurationManager::setConfigurationTokenName(const CIdentifier& identifier, const CString& name)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(this->lookUpConfigurationTokenIdentifier(name, false) == CIdentifier::undefined(),
                        "Configuration token name " << name << " already exists", Kernel::ErrorType::BadResourceCreation);

    auto it = m_ConfigTokens.find(identifier);

    OV_ERROR_UNLESS_KRF(it != m_ConfigTokens.end(), "Configuration token " << identifier.str() << " does not exist",
                        ErrorType::BadResourceCreation);

    it->second.name = name;
    return true;
}

bool CConfigurationManager::setConfigurationTokenValue(const CIdentifier& identifier, const CString& value)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto itConfigurationToken = m_ConfigTokens.find(identifier);

    OV_ERROR_UNLESS_KRF(itConfigurationToken != m_ConfigTokens.end(), "Configuration token " << identifier.str() << " does not exist",
                        ErrorType::BadResourceCreation);

    itConfigurationToken->second.value = value;
    return true;
}

bool CConfigurationManager::addOrReplaceConfigurationToken(const CString& name, const CString& value)
{
    const CIdentifier oldID = this->lookUpConfigurationTokenIdentifier(name, false);
    if (oldID == CIdentifier::undefined()) { return CIdentifier::undefined() != this->createConfigurationToken(name, value); }
    return this->setConfigurationTokenValue(oldID, value);
}

// ----------------------------------------------------------------------------------------------------------------------------

CIdentifier CConfigurationManager::lookUpConfigurationTokenIdentifier(const CString& name, const bool recursive) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto it = m_ConfigTokens.begin();
    while (it != m_ConfigTokens.end())
    {
        if (it->second.name == name) { return it->first; }
        ++it;
    }
    if (recursive && m_parentConfigManager) { return m_parentConfigManager->lookUpConfigurationTokenIdentifier(name, recursive); }
    return CIdentifier::undefined();
}

CString CConfigurationManager::lookUpConfigurationTokenValue(const CString& name) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto it = m_ConfigTokens.begin();
    while (it != m_ConfigTokens.end())
    {
        if (it->second.name == name) { return it->second.value; }
        ++it;
    }
    if (m_parentConfigManager) { return m_parentConfigManager->lookUpConfigurationTokenValue(name); }
    return "";
}

// ----------------------------------------------------------------------------------------------------------------------------

bool CConfigurationManager::registerKeywordParser(const CString& keyword, const IConfigurationKeywordExpandCallback& callback)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(keyword != CString("") && keyword != CString("core") && keyword != CString("environment"),
                        "Trying to overwrite internal keyword " << keyword,
                        ErrorType::BadResourceCreation);

    m_keywordOverrides[keyword] = &callback;

    return true;
}

bool CConfigurationManager::unregisterKeywordParser(const CString& keyword)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(m_keywordOverrides.count(keyword), "Override for keyword [" << keyword << "] was not found", Kernel::ErrorType::ResourceNotFound);

    m_keywordOverrides.erase(keyword);

    return true;
}

bool CConfigurationManager::unregisterKeywordParser(const IConfigurationKeywordExpandCallback& callback)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto it = m_keywordOverrides.begin();

    bool res = false;
    while (it != m_keywordOverrides.end())
    {
        if (it->second == &callback)
        {
            m_keywordOverrides.erase(it);
            res = true;
            break;
        }
        ++it;
    }

    OV_ERROR_UNLESS_KRF(res, "Override for the callback was not found", Kernel::ErrorType::ResourceNotFound);

    return res;
}

// ----------------------------------------------------------------------------------------------------------------------------

CString CConfigurationManager::expand(const CString& expression) const
{
    const std::string value(expression.toASCIIString());
    std::string res;
    if (this->internalExpand(value, res)) { return res.c_str(); }
    return value.c_str();
}

// ----------------------------------------------------------------------------------------------------------------------------

CIdentifier CConfigurationManager::getUnusedIdentifier() const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    uint64_t id = (uint64_t(rand()) << 32) + uint64_t(rand());
    CIdentifier res;
    std::map<CIdentifier, config_token_t>::const_iterator i;
    do
    {
        id++;
        res = CIdentifier(id);
        i   = m_ConfigTokens.find(res);
    } while (i != m_ConfigTokens.end() || res == CIdentifier::undefined());
    return res;
}

// ----------------------------------------------------------------------------------------------------------------------------

enum class ENodeType { Value, NamePrefix, NamePostfix, };

bool CConfigurationManager::internalExpand(const std::string& sValue, std::string& result) const
{
    std::stack<std::pair<ENodeType, std::string>> children;
    children.push(std::make_pair(ENodeType::Value, std::string()));

    std::string prefix;
    std::string postfix;
    std::string lowerPrefix;
    std::string lowerPostfix;
    std::string value;
    std::string expandedValue;

    for (size_t i = 0; i < sValue.length(); ++i)
    {
        bool shouldExpand;

        switch (sValue[i])
        {
            case '$':
                children.push(std::make_pair(ENodeType::NamePrefix, std::string()));
                break;

            case '{':
                OV_ERROR_UNLESS_KRF(children.top().first == ENodeType::NamePrefix,
                                    "Could not expand token with syntax error while expanding " << sValue, Kernel::ErrorType::BadFileParsing);
                children.push(std::make_pair(ENodeType::NamePostfix, std::string()));
                break;

            case '}':
                OV_ERROR_UNLESS_KRF(children.top().first == ENodeType::NamePostfix,
                                    "Could not expand token with syntax error while expanding " << sValue, Kernel::ErrorType::BadFileParsing);
                postfix = children.top().second;
                lowerPostfix.resize(postfix.size());
                std::transform(postfix.begin(), postfix.end(), lowerPostfix.begin(), ToLower<std::string::value_type>);
                children.pop();

                prefix = children.top().second;
                lowerPrefix.resize(prefix.size());
                std::transform(prefix.begin(), prefix.end(), lowerPrefix.begin(), ToLower<std::string::value_type>);
                children.pop();

                shouldExpand = true;

                if (lowerPrefix.empty())
                {
                    // value = this->getConfigurationTokenValue(this->lookUpConfigurationTokenIdentifier(postfix.c_str()));
                    // this->internalGetConfigurationTokenValueFromName(postfix, value);
                    value = this->lookUpConfigurationTokenValue(postfix.c_str()).toASCIIString();
                }
                else if (lowerPrefix == "environment" || lowerPrefix == "env")
                {
                    char* envValue = getenv(postfix.c_str());
                    value          = (envValue ? envValue : "");
                    shouldExpand   = false;
                }
                else if (lowerPrefix == "core")
                {
                    if (lowerPostfix == "random") { value = std::to_string(this->getRandom()); }
                    else if (lowerPostfix == "index") { value = std::to_string(this->getIndex()); }
                    else if (lowerPostfix == "time") { value = this->getTime(); }
                    else if (lowerPostfix == "date") { value = this->getDate(); }
                    else if (lowerPostfix == "real-time") { value = std::to_string(this->getRealTime()); }
                    else if (lowerPostfix == "process-id") { value = std::to_string(this->getProcessId()); }
                    else
                    {
                        OV_ERROR_KRF("Could not expand token with " << prefix << " prefix and " << postfix << " postfix while expanding " << sValue,
                                     ErrorType::BadFileParsing);
                    }
                }
                else
                {
                    if (m_keywordOverrides.count(lowerPrefix.c_str()))
                    {
                        CString overridenValue("");

                        OV_ERROR_UNLESS_KRF((m_keywordOverrides.find(lowerPrefix.c_str())->second)->expand(CString(postfix.c_str()), overridenValue),
                                            "Could not expand $" << lowerPrefix << "{" << lowerPostfix << "}",
                                            ErrorType::BadFileParsing);

                        value = overridenValue;
                    }
                    else
                    {
                        OV_ERROR_UNLESS_KRF(m_parentConfigManager, "Could not expand token with " << prefix << " prefix while expanding " << sValue,
                                            ErrorType::BadFileParsing);

                        std::string keyword = "$" + lowerPrefix + "{" + lowerPostfix + "}";

                        value = m_parentConfigManager->expand(CString(keyword.c_str()));

                        if (value == sValue)
                        {
                            value = "";
                            OV_ERROR_KRF("Could not expand token with " << prefix << " prefix while expanding " << sValue,
                                         ErrorType::BadFileParsing);
                        }
                    }
                }

                if (shouldExpand)
                {
                    OV_ERROR_UNLESS_KRF(this->internalExpand(value, expandedValue),
                                        "Could not expand " << value << " while expanding " << sValue,
                                        ErrorType::BadFileParsing);

                    children.top().second += expandedValue;
                }
                else { children.top().second += value; }
                break;

            case '\\':
                i++;
                OV_ERROR_UNLESS_KRF(i < sValue.length(),
                                    "Could not expand token with unterminated string while expanding " << sValue,
                                    ErrorType::BadFileParsing);

            default:
                children.top().second += sValue[i];
                break;
        }
    }

    /* This will merge all NamePrefix Node on top of the pile into the first which isn't.
     * In case of handling an UNC path, pile should look like this, eventually with multiple ENodeType::NamePrefix :
     * ENodeType::NamePrefix, value2
     * ENodeType::Value, value1
     * 
     * This will merge all of them into the Node below, like this :
     * ENodeType::Value, value1 + '$' + value2 + ( '$' + value3 ...)
     * Using this method, tokens are not reinterpreted, which reduce risks introduced by introducing parser leniency.
     */
    while (children.top().first == ENodeType::NamePrefix && children.size() > 1)
    {
        const std::string topVal = children.top().second;
        children.pop();
        children.top().second += "$" + topVal;
    }

    OV_ERROR_UNLESS_KRF(children.size() == 1, "Could not expand token with unterminated string while expanding " << sValue,
                        ErrorType::BadFileParsing);

    result = children.top().second;

    return true;
}

bool CConfigurationManager::internalExpandOnlyKeyword(const std::string& sKeyword, const std::string& sValue, std::string& sResult,
                                                      bool preserveBackslashes = false) const
{
    std::stack<std::pair<ENodeType, std::string>> childrens;
    childrens.push(std::make_pair(ENodeType::Value, std::string()));

    std::string prefix;
    std::string postfix;
    std::string lowerPrefix;
    std::string lowerPostfix;
    std::string value;
    std::string expandedValue;

    for (size_t i = 0; i < sValue.length(); ++i)
    {
        bool shouldExpand;

        switch (sValue[i])
        {
            case '$':
                childrens.push(std::make_pair(ENodeType::NamePrefix, std::string()));
                break;

            case '{':
                OV_ERROR_UNLESS_KRF(childrens.top().first == ENodeType::NamePrefix,
                                    "Could not expand token with syntax error while expanding " << sValue,
                                    ErrorType::BadFileParsing);
                childrens.push(std::make_pair(ENodeType::NamePostfix, std::string()));
                break;

            case '}':
                OV_ERROR_UNLESS_KRF(childrens.top().first == ENodeType::NamePostfix,
                                    "Could not expand token with syntax error while expanding " << sValue,
                                    ErrorType::BadFileParsing);

                postfix = childrens.top().second;
                childrens.pop();

                prefix = childrens.top().second;
                childrens.pop();

                shouldExpand = true;

                lowerPrefix  = prefix;
                lowerPostfix = postfix;
                std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ToLower<std::string::value_type>);
                std::transform(lowerPostfix.begin(), lowerPostfix.end(), lowerPostfix.begin(), ToLower<std::string::value_type>);

                if (lowerPrefix == sKeyword)
                {
                    OV_ERROR_UNLESS_KRF(m_keywordOverrides.count(lowerPrefix.c_str()),
                                        "Could not expand token with " << prefix << " prefix while expanding " << sValue,
                                        ErrorType::BadFileParsing);

                    CString overridenValue("");

                    OV_ERROR_UNLESS_KRF((m_keywordOverrides.find(lowerPrefix.c_str())->second)->expand(CString(postfix.c_str()), overridenValue),
                                        "Could not expand $" << lowerPrefix << "{" << lowerPostfix << "}",
                                        ErrorType::BadFileParsing);

                    value = overridenValue;
                }
                else
                {
                    // If the previous token was not something we want to parse we will simply put it back
                    value        = "$" + prefix + "{" + postfix + "}";
                    shouldExpand = false;
                }

                if (shouldExpand)
                {
                    OV_ERROR_UNLESS_KRF(this->internalExpandOnlyKeyword(sKeyword, value, expandedValue),
                                        "Could not expand " << value << " while expanding " << sValue,
                                        ErrorType::BadFileParsing);

                    childrens.top().second += expandedValue;
                }
                else { childrens.top().second += value; }
                break;

            case '\\':
                if (preserveBackslashes) { childrens.top().second += sValue[i]; }
                i++;
                OV_ERROR_UNLESS_KRF(i < sValue.length(),
                                    "Could not expand token with unterminated string while expanding " << sValue,
                                    ErrorType::BadFileParsing);
                childrens.top().second += sValue[i];
                break;

            default:
                childrens.top().second += sValue[i];
                break;
        }
    }

    OV_ERROR_UNLESS_KRF(childrens.size() == 1, "Could not expand token with unterminated string while expanding " << sValue,
                        ErrorType::BadFileParsing);

    sResult = childrens.top().second;

    return true;
}

bool CConfigurationManager::internalGetConfigurationTokenValueFromName(const std::string& name, std::string& value) const
{
    const CIdentifier tokenID = this->lookUpConfigurationTokenIdentifier(name.c_str(), false);
    if (tokenID == CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(m_parentConfigManager,
                            "Could not expand token [" << name <<
                            "]. This token does not exist. If this is expected behavior, please add \"" << name <<
                            " = \" to your configuration file",
                            ErrorType::ResourceNotFound);

        const std::string str = std::string("${") + name + ("}");
        value                 = m_parentConfigManager->expand(str.c_str());
    }
    else { value = this->getConfigurationTokenValue(tokenID); }
    return true;
}

CString CConfigurationManager::expandOnlyKeyword(const CString& rKeyword, const CString& expression, const bool preserveBackslashes) const
{
    const std::string value(expression.toASCIIString());
    std::string res;
    if (this->internalExpandOnlyKeyword(rKeyword.toASCIIString(), value, res, preserveBackslashes)) { return res.c_str(); }
    return value.c_str();
}

double CConfigurationManager::expandAsFloat(const CString& expression, const double fallbackValue) const
{
    const CString result = this->expand(expression);
    double res;

    try { res = std::stod(result.toASCIIString()); }
    catch (const std::exception&) { res = fallbackValue; }

    return res;
}

int64_t CConfigurationManager::expandAsInteger(const CString& expression, const int64_t fallbackValue) const
{
    const CString result = this->expand(expression);
    int64_t res;

    try { res = std::stoll(result.toASCIIString()); }
    catch (const std::exception&) { res = fallbackValue; }

    return res;
}

uint64_t CConfigurationManager::expandAsUInteger(const CString& expression, const uint64_t fallbackValue) const
{
    const CString result = this->expand(expression);
    uint64_t res;

    try { res = std::stoull(result.toASCIIString()); }
    catch (const std::exception&) { res = fallbackValue; }

    return res;
}

bool CConfigurationManager::expandAsBoolean(const CString& expression, const bool fallbackValue) const
{
    std::string res = this->expand(expression).toASCIIString();
    std::transform(res.begin(), res.end(), res.begin(), ToLower<std::string::value_type>);

    if (res == "true" || res == "on" || res == "1") { return true; }
    if (res == "false" || res == "off" || res == "0") { return false; }

    return fallbackValue;
}

uint64_t CConfigurationManager::expandAsEnumerationEntryValue(const CString& expression, const CIdentifier& enumTypeID, const uint64_t fallbackValue) const
{
    const CString str     = this->expand(expression);
    const uint64_t result = this->getTypeManager().getEnumerationEntryValueFromName(enumTypeID, str);
    if (result != ULLONG_MAX) { return result; }

    return fallbackValue;
}

size_t CConfigurationManager::getRandom() { return size_t(System::Math::randomI()); }

size_t CConfigurationManager::getIndex() const { return m_idx++; }

CString CConfigurationManager::getTime()
{
    char res[1024];
    time_t rawTime;

    time(&rawTime);
    struct tm* timeInfo = localtime(&rawTime);

    sprintf(res, "%02i.%02i.%02i", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    return res;
}

CString CConfigurationManager::getDate()
{
    char res[1024];
    time_t rawTime;

    time(&rawTime);
    struct tm* timeInfo = localtime(&rawTime);

    sprintf(res, "%04i.%02i.%02i", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday);
    return res;
}

size_t CConfigurationManager::getRealTime() const { return System::Time::getTime() - m_startTime; }

size_t CConfigurationManager::getProcessId()
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    return size_t(getpid());
#elif defined TARGET_OS_Windows
    return size_t(GetCurrentProcessId());
#else
    #error TODO
#endif
}

}  // namespace Kernel
}  // namespace OpenViBE
