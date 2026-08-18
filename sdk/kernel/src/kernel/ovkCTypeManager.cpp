#include "ovkCTypeManager.h"

#include "lepton/Lepton.h"

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <algorithm>

#define OV_TRACE_K(message) this->getLogManager() << OpenViBE::Kernel::LogLevel_Trace << message << "\n";
#define OV_DEBUG_K(message) this->getLogManager() << OpenViBE::Kernel::LogLevel_Debug << message << "\n";
#define OV_DEBUG_UNLESS_K(expression, message) if (!(expression)) { OV_DEBUG_K(message); }

namespace OpenViBE {
namespace Kernel {
namespace {
struct SAInfB
{
    bool operator()(const std::pair<CIdentifier, CString> a, const std::pair<CIdentifier, CString>& b) const { return a.second < b.second; }
};
}  // namespace

// because std::tolower has multiple signatures,
// it can not be easily used in std::transform
// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
template <class TCharT>
static TCharT ToLower(TCharT c) { return std::tolower(c); }

CTypeManager::CTypeManager(const IKernelContext& ctx)
    : TKernelObject<ITypeManager>(ctx)
{
    m_names[CIdentifier::undefined()] = "undefined";
    this->registerEnumerationType(OV_TypeId_BoxAlgorithmFlag, "BoxFlags");
}

CIdentifier CTypeManager::getNextTypeIdentifier(const CIdentifier& previousID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    return getNextIdentifier<CString>(m_names, previousID);
}

std::vector<std::pair<CIdentifier, CString>> CTypeManager::getSortedTypes() const
{
    std::vector<std::pair<CIdentifier, CString>> sorted;

    for (const auto& element : m_names) { sorted.push_back(std::pair<CIdentifier, CString>(element.first, element.second)); }
    std::sort(sorted.begin(), sorted.end(), SAInfB());

    return sorted;
}

bool CTypeManager::registerType(const CIdentifier& typeID, const CString& name)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(!isRegistered(typeID), "Trying to register type " << typeID.str() << " that already exists.",
                        ErrorType::BadArgument);

    OV_DEBUG_UNLESS_K(m_takenNames.find(name) == m_takenNames.end(),
                      "Trying to register type " << typeID << " with a name that already exists ( " << name << ")");

    m_names[typeID] = name;
    OV_TRACE_K("Registered type id " << typeID << " - " << name);
    return true;
}

bool CTypeManager::registerStreamType(const CIdentifier& typeID, const CString& name, const CIdentifier& parentTypeID)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(!isRegistered(typeID), "Trying to register stream type " << typeID.str() << " that already exists.",
                        ErrorType::BadArgument);

    OV_DEBUG_UNLESS_K(m_takenNames.find(name) == m_takenNames.end(),
                      "Trying to register stream type " << typeID << " with a name that already exists ( " << name << ")");

    OV_ERROR_UNLESS_KRF(parentTypeID == CIdentifier::undefined() || isStream(parentTypeID),
                        "Trying to register an invalid stream type [" << name << "] " << typeID.str() << ", parent : " << parentTypeID.str() << ".",
                        ErrorType::BadArgument);

    m_names[typeID] = name;
    m_takenNames.insert(name);
    m_streams[typeID] = parentTypeID;
    OV_TRACE_K("Registered stream type id " << typeID << "::" << parentTypeID << " - " << name);
    return true;
}

bool CTypeManager::registerEnumerationType(const CIdentifier& typeID, const CString& name)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    if (isRegistered(typeID))
    {
        if (m_names[typeID] != name)
        {
            OV_ERROR_KRF(
                "Trying to register enum type " << typeID.str() << " that already exists with different value (" << m_names[typeID] << " != " << name <<
                ")",
                ErrorType::BadArgument);
        }
        OV_DEBUG_K("Trying to register enum type " << typeID.str() << " that already exists.");
    }

    OV_DEBUG_UNLESS_K(m_takenNames.find(name) == m_takenNames.end(),
                      "Trying to register enum type " << typeID << " with a name that already exists ( " << name << ")");

    m_names[typeID] = name;
    m_takenNames.insert(name);
    m_enumerations[typeID];
    OV_TRACE_K("Registered enumeration type id " << typeID << " - " << name);
    return true;
}

bool CTypeManager::registerEnumerationEntry(const CIdentifier& typeID, const CString& name, const uint64_t value)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto itEnumeration = m_enumerations.find(typeID);

    OV_ERROR_UNLESS_KRF(itEnumeration != m_enumerations.end(), "Enumeration type [" << typeID.str() << "] does not exist." << name,
                        ErrorType::BadArgument);

    const auto itElem = itEnumeration->second.find(value);
    if (itElem != itEnumeration->second.end())
    {
        if (std::string(itElem->second) != std::string(name))
        {
            OV_WARNING_K(
                "Enumeration type [" + typeID.str() + "] already has element [" + std::to_string(value) + "]. Value will be overriden : "
                + itElem->second.toASCIIString() + " => " + name.toASCIIString());
        }
        else { OV_DEBUG_K("Enumeration type [" << typeID.str() << "] already has element [" << value << "]."); }
    }

    itEnumeration->second[value] = name;
    return true;
}

bool CTypeManager::registerBitMaskType(const CIdentifier& typeID, const CString& name)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    OV_ERROR_UNLESS_KRF(!isRegistered(typeID), "Trying to register bitmask type " << typeID.str() << " that already exists.",
                        ErrorType::BadArgument);

    OV_DEBUG_UNLESS_K(m_takenNames.find(name) == m_takenNames.end(),
                      "Trying to register bitmask type " << typeID << " with a name that already exists ( " << name << ")");

    m_names[typeID] = name;
    m_bitMasks[typeID];
    OV_TRACE_K("Registered bitmask type id " << typeID << " - " << name);
    return true;
}

bool CTypeManager::registerBitMaskEntry(const CIdentifier& typeID, const CString& name, const uint64_t value)
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto itBitMask = m_bitMasks.find(typeID);
    OV_ERROR_UNLESS_KRF(itBitMask != m_bitMasks.end(), "Bitmask type [" << typeID.str() << "] does not exist.", Kernel::ErrorType::BadArgument);

    const auto itElem = itBitMask->second.find(value);
    if (itElem != itBitMask->second.end())
    {
        if (std::string(itElem->second) != std::string(name))
        {
            OV_WARNING_K(
                "Bitmask type [" + typeID.str() + "] already has element [" + std::to_string(value) + "]. Value will be overriden : "
                + itElem->second.toASCIIString() + " => " + name.toASCIIString());
        }
        else { OV_DEBUG_K("Bitmask type [" << typeID.str() << "] already has element [" << value << "]."); }
    }

    for (size_t nBit = 0, i = 0; i < 64; ++i)
    {
        if (value & (1LL << i))
        {
            nBit++;
            OV_ERROR_UNLESS_KRF(nBit <= 1,
                                "Discarded bitmask entry (" << m_names[typeID] << ":" << name << ") because value " << value << " contains more than one bit",
                                ErrorType::Overflow);
        }
    }
    itBitMask->second[value] = name;
    return true;
}

bool CTypeManager::isRegistered(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    return m_names.find(typeID) != m_names.end();
}

bool CTypeManager::isStream(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    return m_streams.find(typeID) != m_streams.end();
}

bool CTypeManager::isDerivedFromStream(const CIdentifier& typeID, const CIdentifier& parentTypeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    auto it             = m_streams.find(typeID);
    const auto itParent = m_streams.find(parentTypeID);
    if (it == m_streams.end() || itParent == m_streams.end()) { return false; }
    while (it != m_streams.end())
    {
        if (it->first == parentTypeID) { return true; }
        it = m_streams.find(it->second);
    }
    return false;
}

bool CTypeManager::isEnumeration(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    return m_enumerations.find(typeID) != m_enumerations.end();
}

bool CTypeManager::isBitMask(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    return m_bitMasks.find(typeID) != m_bitMasks.end();
}

CString CTypeManager::getTypeName(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    if (!isRegistered(typeID)) { return ""; }
    return m_names.find(typeID)->second;
}

CIdentifier CTypeManager::getTypeFromName(const CString& name) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    for (const auto& element : m_names)
    {
        if (element.second == name) { return element.first; }
    }
    return CIdentifier::undefined();
}

CIdentifier CTypeManager::getStreamParentType(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    if (!isStream(typeID)) { return CIdentifier::undefined(); }
    return m_streams.find(typeID)->second;
}

size_t CTypeManager::getEnumerationEntryCount(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto itEnumeration = m_enumerations.find(typeID);
    if (itEnumeration == m_enumerations.end()) { return 0; }
    return itEnumeration->second.size();
}

bool CTypeManager::getEnumerationEntry(const CIdentifier& typeID, const uint64_t index, CString& name, uint64_t& value) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_enumerations.find(typeID);
    if (it == m_enumerations.end()) { return false; }

    if (index >= it->second.size()) { return false; }

    auto itEntry = it->second.begin();
    for (size_t i = 0; i < index && itEntry != it->second.end(); i++, ++itEntry) { }

    value = itEntry->first;
    name  = itEntry->second;
    return true;
}

CString CTypeManager::getEnumerationEntryNameFromValue(const CIdentifier& typeID, const uint64_t value) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_enumerations.find(typeID);
    if (it == m_enumerations.end()) { return ""; }
    const auto itEntry = it->second.find(value);
    if (itEntry == it->second.end()) { return ""; }
    return it->second.find(value)->second;
}

uint64_t CTypeManager::getEnumerationEntryValueFromName(const CIdentifier& typeID, const CString& name) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_enumerations.find(typeID);
    if (it == m_enumerations.end()) { return OV_IncorrectStimulation; }

    // first looks at the exact std::string match
    for (const auto& entry : it->second) { if (entry.second == name) { return entry.first; } }

    // then looks at the caseless std::string match
    std::string nameLower = name.toASCIIString();
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ToLower<std::string::value_type>);
    for (const auto& entry : it->second)
    {
        std::string tmp = entry.second.toASCIIString();
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), ToLower<std::string::value_type>);
        if (tmp == nameLower) { return entry.first; }
    }

    // then looks at the std::string being the value itself
    try
    {
        const uint64_t value = std::stoull(name.toASCIIString());

        if ((it->second.find(value) != it->second.end()) ||
            (typeID == OV_TypeId_Stimulation && this->getConfigurationManager().expandAsBoolean("Kernel_AllowUnregisteredNumericalStimulationIdentifiers")))
        {
            return value;
        }
    }
    catch (const std::exception&) { return OV_IncorrectStimulation; }

    return OV_IncorrectStimulation;
}

size_t CTypeManager::getBitMaskEntryCount(const CIdentifier& typeID) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto itBitMask = m_bitMasks.find(typeID);
    if (itBitMask == m_bitMasks.end()) { return 0; }
    return itBitMask->second.size();
}

bool CTypeManager::getBitMaskEntry(const CIdentifier& typeID, const uint64_t index, CString& name, uint64_t& value) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto itBitMask = m_bitMasks.find(typeID);
    if (itBitMask == m_bitMasks.end()) { return false; }

    if (index >= itBitMask->second.size()) { return false; }

    auto itBitMaskEntry = itBitMask->second.begin();
    for (size_t i = 0; i < index && itBitMaskEntry != itBitMask->second.end(); i++, ++itBitMaskEntry) { }

    value = itBitMaskEntry->first;
    name  = itBitMaskEntry->second;
    return true;
}

CString CTypeManager::getBitMaskEntryNameFromValue(const CIdentifier& typeID, const uint64_t value) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto itBitMask = m_bitMasks.find(typeID);
    if (itBitMask == m_bitMasks.end()) { return ""; }
    const auto itBitMaskEntry = itBitMask->second.find(value);
    if (itBitMaskEntry == itBitMask->second.end()) { return ""; }
    return itBitMask->second.find(value)->second;
}

uint64_t CTypeManager::getBitMaskEntryValueFromName(const CIdentifier& typeID, const CString& name) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto itBitMask = m_bitMasks.find(typeID);
    if (itBitMask == m_bitMasks.end()) { return 0xffffffffffffffffLL; }

    // first looks at the exact std::string match
    for (const auto& mask : itBitMask->second) { if (mask.second == name) { return mask.first; } }

    // then looks at the caseless std::string match
    std::string entryNameLower = name.toASCIIString();
    std::transform(entryNameLower.begin(), entryNameLower.end(), entryNameLower.begin(), ToLower<std::string::value_type>);
    for (const auto& mask : itBitMask->second)
    {
        std::string itEntryNameLower = mask.second.toASCIIString();
        std::transform(itEntryNameLower.begin(), itEntryNameLower.end(), itEntryNameLower.begin(), ToLower<std::string::value_type>);
        if (itEntryNameLower == entryNameLower) { return mask.first; }
    }

    // then looks at the std::string being the value itself
    try
    {
        const uint64_t value = std::stoll(name.toASCIIString());
        if (itBitMask->second.find(value) != itBitMask->second.end()) { return value; }
    }
    catch (const std::exception&) { return 0xffffffffffffffffLL; }

    return 0xffffffffffffffffLL;
}

CString CTypeManager::getBitMaskEntryCompositionNameFromValue(const CIdentifier& typeID, const uint64_t value) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto itBitMask = m_bitMasks.find(typeID);
    if (itBitMask == m_bitMasks.end()) { return ""; }

    std::string res;
    for (size_t i = 0; i < 64; ++i)
    {
        if (value & (1LL << i))
        {
            const auto itBitMaskEntry = itBitMask->second.find(value & (1LL << i));
            if (itBitMaskEntry == itBitMask->second.end()) { return ""; }
            if (res.empty()) { res = itBitMaskEntry->second.toASCIIString(); }
            else
            {
                res += std::string(1, OV_Value_EnumeratedStringSeparator);
                res += itBitMaskEntry->second.toASCIIString();
            }
        }
    }
    return CString(res.c_str());
}

uint64_t CTypeManager::getBitMaskEntryCompositionValueFromName(const CIdentifier& typeID, const CString& name) const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    const auto it = m_bitMasks.find(typeID);
    if (it == m_bitMasks.end()) { return 0; }

    uint64_t res                           = 0;
    const std::string entryCompositionName = name.toASCIIString();
    size_t i                               = 0;
    size_t j                               = 0;
    do
    {
        i = entryCompositionName.find(OV_Value_EnumeratedStringSeparator, i);
        if (i == std::string::npos) { i = entryCompositionName.length(); }

        if (i != j)
        {
            std::string entryName;
            entryName.assign(entryCompositionName, j, i - j);

            bool found = false;
            for (const auto& mask : it->second)
            {
                if (mask.second == CString(entryName.c_str()))
                {
                    res |= mask.first;
                    found = true;
                }
            }

            if (!found) { return 0; }
        }

        i++;
        j = i;
    } while (i < entryCompositionName.length());

    return res;
}

bool CTypeManager::evaluateSettingValue(const CString value, double& result) const
{
    // parse and expression with no variables or functions
    try { result = Lepton::Parser::parse(value.toASCIIString()).evaluate(); }
    catch (...) { return false; }
    return true;
}

}  // namespace Kernel
}  // namespace OpenViBE
