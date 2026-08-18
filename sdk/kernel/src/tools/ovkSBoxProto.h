#pragma once

#include <openvibe/kernel/scenario/ovIBoxProto.h>
#include <system/ovCMemory.h>

namespace OpenViBE {
namespace Kernel {

struct SBoxProto final : IBoxProto
{
    SBoxProto(ITypeManager& typeManager) : m_TypeManager(typeManager) { }

    bool addInput(const CString& /*name*/, const CIdentifier& typeID, const CIdentifier& id, const bool /*notify*/) override
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_inputCountHash);
        swap_byte(m_inputCountHash, 0x7936A0F3BD12D936LL);
        m_hash = m_hash.id() ^ v;
        if (id != CIdentifier::undefined())
        {
            v = id.id();
            swap_byte(v, 0x2BD1D158F340014D);
            m_hash = m_hash.id() ^ v;
        }
        return true;
    }

    bool addOutput(const CString& /*name*/, const CIdentifier& typeID, const CIdentifier& id, const bool /*notify*/) override
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_outputCountHash);
        swap_byte(m_outputCountHash, 0xCBB66A5B893AA4E9LL);
        m_hash = m_hash.id() ^ v;
        if (id != CIdentifier::undefined())
        {
            v = id.id();
            swap_byte(v, 0x87CA0F5EFC4FAC68);
            m_hash = m_hash.id() ^ v;
        }
        return true;
    }

    bool addSetting(const CString& /*name*/, const CIdentifier& typeID, const CString& /*defaultValue*/, const bool /*modifiable*/, const CIdentifier& id,
                    const bool /*notify*/) override
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_settingCountHash);
        swap_byte(m_settingCountHash, 0x3C87F3AAE9F8303BLL);
        m_hash = m_hash.id() ^ v;
        if (id != CIdentifier::undefined())
        {
            v = id.id();
            swap_byte(v, 0x17185F7CDA63A9FA);
            m_hash = m_hash.id() ^ v;
        }
        return true;
    }

    bool addInputSupport(const CIdentifier& typeID) override
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_outputCountHash);
        swap_byte(m_outputCountHash, 0xCBB66A5B893AA4E9LL);
        m_hash = m_hash.id() ^ v;
        return true;
    }

    bool addInputAndDerivedSupport(const CIdentifier& typeID)
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_outputCountHash);
        swap_byte(m_outputCountHash, 0xCBB66A5B893AA4E9LL);
        m_hash = m_hash.id() ^ v;
        return true;
    }

    bool addOutputSupport(const CIdentifier& typeID) override
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_outputCountHash);
        swap_byte(m_outputCountHash, 0xCBB66A5B893AA4E9LL);
        m_hash = m_hash.id() ^ v;
        return true;
    }

    bool addOutputAndDerivedSupport(const CIdentifier& typeID)
    {
        uint64_t v = typeID.id();
        swap_byte(v, m_outputCountHash);
        swap_byte(m_outputCountHash, 0xCBB66A5B893AA4E9LL);
        m_hash = m_hash.id() ^ v;
        return true;
    }

    bool addFlag(const EBoxFlag flag) override
    {
        switch (flag)
        {
            case BoxFlag_CanAddInput: m_hash = m_hash.id() ^ CIdentifier(0x07507AC8, 0xEB643ACE).id();
                break;
            case BoxFlag_CanModifyInput: m_hash = m_hash.id() ^ CIdentifier(0x5C985376, 0x8D74CDB8).id();
                break;
            case BoxFlag_CanAddOutput: m_hash = m_hash.id() ^ CIdentifier(0x58DEA69B, 0x12411365).id();
                break;
            case BoxFlag_CanModifyOutput: m_hash = m_hash.id() ^ CIdentifier(0x6E162C01, 0xAC979F22).id();
                break;
            case BoxFlag_CanAddSetting: m_hash = m_hash.id() ^ CIdentifier(0xFA7A50DC, 0x2140C013).id();
                break;
            case BoxFlag_CanModifySetting: m_hash = m_hash.id() ^ CIdentifier(0x624D7661, 0xD8DDEA0A).id();
                break;
            case BoxFlag_IsDeprecated: m_isDeprecated = true;
                break;
            default:
                return false;
        }
        return true;
    }

    bool addFlag(const CIdentifier& flagId) override
    {
        const uint64_t value = m_TypeManager.getEnumerationEntryValueFromName(OV_TypeId_BoxAlgorithmFlag, flagId.toString());
        if (value == CIdentifier::undefined().id()) { return false; }
        // Flags do not modify internal hash
        //m_hash=m_hash.id() ^ flagId.id();
        return true;
    }

    void swap_byte(uint64_t& v, const uint64_t s)
    {
        uint8_t v2[sizeof(v)];
        uint8_t s2[sizeof(s)];
        System::Memory::hostToLittleEndian(v, v2);
        System::Memory::hostToLittleEndian(s, s2);
        for (size_t i = 0; i < sizeof(s); i += 2)
        {
            const size_t j  = s2[i] % sizeof(v);
            const size_t k  = s2[i + 1] % sizeof(v);
            const uint8_t t = v2[j];
            v2[j]           = v2[k];
            v2[k]           = t;
        }
        System::Memory::littleEndianToHost(v2, &v);
    }

    _IsDerivedFromClass_Final_(IBoxProto, CIdentifier::undefined())

    CIdentifier m_hash          = CIdentifier::undefined();
    bool m_isDeprecated         = false;
    uint64_t m_inputCountHash   = 0x64AC3CB54A35888CLL;
    uint64_t m_outputCountHash  = 0x21E0FAAFE5CAF1E1LL;
    uint64_t m_settingCountHash = 0x6BDFB15B54B09F63LL;
    ITypeManager& m_TypeManager;
};
}  // namespace Kernel
}  // namespace OpenViBE
