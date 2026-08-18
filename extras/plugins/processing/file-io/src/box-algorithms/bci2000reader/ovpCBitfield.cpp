#include "ovpCBitfield.h"

namespace BCI2000 {

uint32_t* CBitfield::getFields(unsigned char* data)
{
    const size_t nFields = m_fields.size();
    uint32_t* fields     = new uint32_t[nFields];
    getFields(data, fields);
    return fields;
}

void CBitfield::getFields(unsigned char* data, uint32_t* fields)
{
    const size_t nFields = m_fields.size();
    for (uint32_t i = 0; i < nFields; ++i)
    {
        const int space  = (m_fields[i].m_Length - 1 + m_fields[i].m_BitPos) / 8 + 1;
        unsigned char* p = data + m_fields[i].m_BytePos;
        uint32_t field   = (*p) >> m_fields[i].m_BitPos;

        for (uint32_t j = 1; j < uint32_t(space); ++j)
        {
            p++;
            field += uint32_t(*p) << (j * 8 - m_fields[i].m_BitPos);
        }

        field &= 0xFFFFFFFF >> (32 - m_fields[i].m_Length);
        fields[i] = field;
    }
}

bool CBitfield::addField(const int bytePosition, const int bitPosition, const int length, const OpenViBE::CString& name, const int value)
{
    if (length > 32) { return false; } // doc says len is 32 bits max
    if (bitPosition > 7) { return false; }
    m_fields.push_back(CField(bytePosition, bitPosition, length, name, value));
    return true;
}

}  // namespace BCI2000
