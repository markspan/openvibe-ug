#include "ovtkString.h"

#include <cstring>

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>

#include <cstring>

namespace OpenViBE {
namespace Toolkit {
namespace String {

static bool isSeparator(const uint8_t value, const uint8_t* separator, const size_t nSeparator)
{
    for (size_t i = 0; i < nSeparator; ++i) { if (value == separator[i]) { return true; } }
    return false;
}

// because std::tolower has multiple signatures,
// it can not be easily used in std::transform
// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
template <class T>
static T ToLower(T c) { return std::tolower(c); }

size_t split(const CString& str, const ISplitCallback& splitCB, uint8_t separator) { return split(str, splitCB, &separator, 1); }

size_t split(const CString& str, const ISplitCallback& splitCB, uint8_t* separator, const size_t nSeparator)
{
    if (nSeparator == 0 || separator == nullptr) { return 0; }

    size_t n = 0;
    std::string tmp(str.toASCIIString());
    size_t i = 0;
    while (i < tmp.length())
    {
        size_t j = i;
        while (j < tmp.length() && !isSeparator(tmp[j], separator, nSeparator)) { j++; }
        //if(i!=j)
        {
            splitCB.setToken(std::string(tmp, i, j - i).c_str());
            n++;
        }
        i = j + 1;
    }
    if (tmp.length() != 0 && isSeparator(tmp[tmp.length() - 1], separator, nSeparator))
    {
        splitCB.setToken("");
        n++;
    }

    return n;
}


bool isAlmostEqual(const CString& str1, const CString& str2, const bool caseSensitive, const bool removeStartSpaces, const bool removeEndSpaces)
{
    const char* str1Start = str1.toASCIIString();
    const char* str1End   = str1Start + strlen(str1Start) - 1;

    const char* str2Start = str2.toASCIIString();
    const char* str2End   = str2Start + strlen(str2Start) - 1;

    if (removeStartSpaces)
    {
        while (*str1Start == ' ') { str1Start++; }
        while (*str2Start == ' ') { str2Start++; }
    }

    if (removeEndSpaces)
    {
        while (str1Start < str1End && *str1End == ' ') { str1End--; }
        while (str2Start < str2End && *str2End == ' ') { str2End--; }
    }

    std::string tmp1(str1Start, str1End - str1Start + 1);
    std::string tmp2(str2Start, str2End - str2Start + 1);

    if (!caseSensitive)
    {
        std::transform(tmp1.begin(), tmp1.end(), tmp1.begin(), ToLower<std::string::value_type>);
        std::transform(tmp2.begin(), tmp2.end(), tmp2.begin(), ToLower<std::string::value_type>);
    }

    return tmp1 == tmp2;
}

}  // namespace String
}  // namespace Toolkit
}  // namespace OpenViBE
