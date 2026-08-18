#include "ovCString.h"

#include <string>

namespace OpenViBE {

struct SStringImpl
{
    std::string m_Value;
};

CString::CString() { m_impl = new SStringImpl(); }

CString::CString(const CString& str)
{
    m_impl          = new SStringImpl();
    m_impl->m_Value = str.m_impl->m_Value;
}

CString::CString(const char* str)
{
    m_impl = new SStringImpl();
    if (str) { m_impl->m_Value = str; }
}

CString::~CString() { delete m_impl; }
CString::operator const char*() const { return m_impl->m_Value.c_str(); }

CString& CString::operator=(const CString& str)
{
    m_impl->m_Value = str.m_impl->m_Value;
    return *this;
}

CString& CString::operator+=(const CString& str)
{
    m_impl->m_Value += str.m_impl->m_Value;
    return *this;
}

char& CString::operator[](const size_t idx) const { return m_impl->m_Value[idx]; }

CString operator+(const CString& str1, const CString& str2)
{
    std::string res;
    res = str1.m_impl->m_Value + str2.m_impl->m_Value;
    return res.c_str();
}

bool operator==(const CString& str1, const CString& str2) { return (str1.m_impl->m_Value) == (str2.m_impl->m_Value); }
bool operator!=(const CString& str1, const CString& str2) { return (str1.m_impl->m_Value) != (str2.m_impl->m_Value); }
bool operator<(const CString& str1, const CString& str2) { return (str1.m_impl->m_Value) < (str2.m_impl->m_Value); }

bool CString::set(const CString& str) const
{
    m_impl->m_Value = str.m_impl->m_Value;
    return true;
}

bool CString::set(const char* str) const
{
    if (str) { m_impl->m_Value = str; }
    else { m_impl->m_Value = ""; }
    return true;
}

const char* CString::toASCIIString() const { return m_impl->m_Value.c_str(); }

size_t CString::length() const { return m_impl->m_Value.length(); }

}  // namespace OpenViBE
