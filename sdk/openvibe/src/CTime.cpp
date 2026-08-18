#include "CTime.hpp"

#include <sstream>
#include <iomanip>

namespace OpenViBE {

//--------------------------------------------------------------------------------
CTime& CTime::operator=(const CTime& time)
{
    m_time = time.m_time;
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
CTime& CTime::operator=(const uint64_t time)
{
    m_time = time;
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
CTime& CTime::operator+=(const CTime& time)
{
    m_time += time.m_time;
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
CTime& CTime::operator-=(const CTime& time)
{
    m_time -= time.m_time;
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
std::string CTime::str(const bool inSecond, const bool inHexa) const
{
    std::stringstream res;
    // Precision for 3 after dot and fixed to force 0 display to have always 3 after precision
    if (inSecond) { res << std::setprecision(3) << std::fixed << toSeconds() << " sec"; }
    else { res << m_time; }

    // Force to have 16 number and 0 for empty number (space character by default)
    if (inHexa) { res << " (0x" << std::setfill('0') << std::setw(16) << std::hex << m_time << ")"; }
    return res.str();
}
//--------------------------------------------------------------------------------

}  // namespace OpenViBE
