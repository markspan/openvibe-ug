#include "CIdentifier.hpp"

#include <cstdio>
#include <random>
#include <sstream>
#include <iomanip>

namespace OpenViBE {

//--------------------------------------------------------------------------------
CIdentifier& CIdentifier::operator=(const CIdentifier& id)
{
    m_id = id.m_id;
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
CIdentifier& CIdentifier::operator++()
{
    if (m_id != std::numeric_limits<uint64_t>::max())
    {
        m_id++;
        if (m_id == std::numeric_limits<uint64_t>::max()) { m_id = 0; }
    }
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
CIdentifier& CIdentifier::operator--()
{
    if (m_id != std::numeric_limits<uint64_t>::max())
    {
        m_id--;
        if (m_id == std::numeric_limits<uint64_t>::max()) { m_id = std::numeric_limits<uint64_t>::max() - 1; }
    }
    return *this;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
std::string CIdentifier::str(const bool hexa) const
{
    const uint32_t id1 = uint32_t(m_id >> 32);
    const uint32_t id2 = uint32_t(m_id);
    if (hexa)
    {
        std::stringstream ss;
        ss.fill('0');
        ss << "(0x" << std::setw(8) << std::hex << id1 << ", 0x" << std::setw(8) << std::hex << id2 << ")";
        return ss.str();
    }
    return std::to_string(m_id);
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CIdentifier::fromString(const std::string& str)
{
    const char* buffer = str.c_str();
    uint32_t id1;
    uint32_t id2;
    if (sscanf(buffer, "(0x%x, 0x%x)", &id1, &id2) != 2) { return false; }
    m_id = (uint64_t(id1) << 32) + id2;
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
CIdentifier CIdentifier::random()
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<size_t> uni(0, std::numeric_limits<size_t>::max() - 1); // This exclude CIdentifier::undefined() value no const on unix system
    return CIdentifier(uni(rng));
}
//--------------------------------------------------------------------------------

}  // namespace OpenViBE
