#pragma once

#include <openvibe/ov_all.h>

#include <vector>

namespace BCI2000 {
class CBitfield
{
protected:
    class CField
    {
    public:
        int m_BytePos = 0;  // starting byte
        int m_BitPos  = 0;  // starting bit within starting byte
        int m_Length  = 0;  // field length (in bits)
        OpenViBE::CString m_Name;
        int m_InitialValue = 0; // initial value. Not updated when reading files
        CField(const int bytePos, const int bitPos, const int length, const OpenViBE::CString& name, const int initialValue)
            : m_BytePos(bytePos), m_BitPos(bitPos), m_Length(length), m_Name(name), m_InitialValue(initialValue) {}
    };

    std::vector<CField> m_fields;
public:
    // extract values from compressed data; returned array of size this->size() to
    // be deleted[] by the user.
    uint32_t* getFields(unsigned char* data);

    // extract values from compressed data
    // extractedFields must be allocated and of size this->size()
    void getFields(unsigned char* data, uint32_t* fields);

    // add a new field. Returns false when invalid paramaters entered, true otherwise.
    bool addField(const int bytePosition, const int bitPosition, const int length, const OpenViBE::CString& name, const int value);

    // returns the number of m_fields in the structure
    // (not to be confused with the actual memory footprint)
    uint32_t size() const { return m_fields.size(); }

    const OpenViBE::CString& getFieldName(const size_t index) const { return m_fields[index].m_Name; }
    int getInitialValue(const size_t index) const { return m_fields[index].m_InitialValue; }
};
}  // namespace BCI2000
