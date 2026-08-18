#if defined TARGET_HAS_ThirdPartyEEGOAPI

#include <algorithm>
#include <locale>
#include <boost/algorithm/string.hpp>

#include "ovasCHeaderEEGO.h"

namespace OpenViBE {
namespace AcquisitionServer {

void CHeaderEEGO::setEEGRange(const uint32_t range)
{
    m_iEEGRange    = range;
    m_bEEGRangeSet = true;
}

void CHeaderEEGO::setEEGMask(const CString& mask)
{
    m_sEEGMask    = mask;
    m_bEEGMaskSet = true;
}

void CHeaderEEGO::setBIPRange(const uint32_t range)
{
    m_iBIPRange    = range;
    m_bBIPRangeSet = true;
}

void CHeaderEEGO::setBIPMask(const CString& mask)
{
    m_sBIPMask    = mask;
    m_bBIPMaskSet = true;
}

/* static */
bool CHeaderEEGO::convertMask(char const* str, uint64_t& out)
{
    bool error = false;

    // init r_outValue anyway
    out = 0;

    std::string input(str);         //easier substring handling etc. Minor performance penalty which should not matter.
    boost::algorithm::trim(input);  // Make sure to handle whitespace correctly

    // check prefixes
    if (boost::algorithm::istarts_with(input, "0b"))
    {
        // binary
        const auto substring = input.substr(2);

        // check for valid string members
        if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return chr == '0' || chr == '1'; })) { error = true; }
        else
        {
            // use the substring for string to number conversion as base 2
            out = strtoull(substring.c_str(), nullptr, 2);
        }
    }
    else if (boost::algorithm::istarts_with(input, "0x"))
    {
        // hex
        const auto substring = input.substr(2);
        std::locale loc;

        if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return std::isxdigit(chr, loc); })) { error = true; }
        else { out = strtoull(substring.c_str(), nullptr, 16); }
    }
    else if (boost::algorithm::istarts_with(input, "0"))
    {
        // octal
        const auto substring = input.substr(1);

        if (!std::all_of(substring.begin(), substring.end(), [&](const char& chr) { return chr >= '0' && chr < '8'; })) { error = true; }
        else { out = strtoull(substring.c_str(), nullptr, 8); }
    }
    else
    {
        // decimal
        //const auto substring = input;
        std::locale loc;

        if (!std::all_of(input.begin(), input.end(), [&](const char& chr) { return std::isdigit(chr, loc); })) { error = true; }
        else { out = strtoull(input.c_str(), nullptr, 10); }
    }

    // if no special handling for the base 2 case is neccessary we can just use the std::stroull implementation and do not mess with that any further.
    return !error;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
