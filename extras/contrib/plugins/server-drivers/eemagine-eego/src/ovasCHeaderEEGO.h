#pragma once

#if defined TARGET_HAS_ThirdPartyEEGOAPI

#include "../ovasCHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CHeaderEEGO final : public CHeader
{
public:

    CHeaderEEGO() { }

    // EEG, referential channels
    // range
    uint32_t getEEGRange() const { return m_iEEGRange; }

    void setEEGRange(uint32_t range);
    bool isEEGRangeSet() const { return m_bEEGRangeSet; }

    // mask
    CString getEEGMask() const { return m_sEEGMask; }
    uint64_t getEEGMaskInt() const { return strtoull(m_sEEGMask, nullptr, 0); } // Same as method above. Only string parsing has been done
    void setEEGMask(const CString& mask);
    bool isEEGMaskSet() const { return m_bEEGMaskSet; }

    // Bipolar channels
    // range
    uint32_t getBIPRange() const { return m_iBIPRange; }
    uint64_t getBIPMaskInt() const { return strtoull(m_sBIPMask, nullptr, 0); } // Same as method above. Only string parsing has been done
    void setBIPRange(uint32_t range);
    bool isBIPRangeSet() const { return m_bBIPRangeSet; }

    // mask
    CString getBIPMask() const { return m_sBIPMask; }
    void setBIPMask(const CString& mask);
    bool isBIPMaskSet() const { return m_bBIPMaskSet; }

    // Converts a string representing a number to this number as unsigned 64 bit value.
    // Accepts 0x, 0b and 0 notation for hexadecimal, binary and octal notation.
    // Otherwise it is interpreted as decimal.
    // Returns true if the conversion was successfull, false on error.
    // Please note that the error checking goes beyond the parsing  strtoull etc.:
    // The strto* methods stop parsing at the first character which could not be interpreted.
    // Here the string is checked against all invalid chars and an error will be returned.
    static bool convertMask(char const* str, uint64_t& out);

    // data
protected:
    uint32_t m_iEEGRange = 1000;
    uint32_t m_iBIPRange = 1500;
    CString m_sEEGMask   = "0xFFFFFFFFFFFFFFFF";
    CString m_sBIPMask   = "0xFFFFFF";
    bool m_bEEGRangeSet  = false;
    bool m_bBIPRangeSet  = false;
    bool m_bEEGMaskSet   = false;
    bool m_bBIPMaskSet   = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
