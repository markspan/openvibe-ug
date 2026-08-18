#pragma once

#include "../../ovk_tools.h"

#include <openvibe/ov_all.h>

#include <map>

namespace OpenViBE {
namespace Kernel {
template <class T>
class TAttributable : public T
{
public:

    explicit TAttributable(const IKernelContext& ctx) : T(ctx) { }

    bool addAttribute(const CIdentifier& attributeID, const CString& sAttributeValue) override
    {
        const auto itAttribute = m_attributes.find(attributeID);
        if (itAttribute != m_attributes.end()) { return false; }
        m_attributes[attributeID] = sAttributeValue;
        return true;
    }

    bool removeAttribute(const CIdentifier& attributeID) override
    {
        const auto itAttribute = m_attributes.find(attributeID);
        if (itAttribute == m_attributes.end()) { return false; }
        m_attributes.erase(itAttribute);
        return true;
    }

    bool removeAllAttributes() override
    {
        m_attributes.clear();
        return true;
    }

    CString getAttributeValue(const CIdentifier& attributeID) const override
    {
        const auto itAttribute = m_attributes.find(attributeID);
        if (itAttribute == m_attributes.end()) { return ""; }
        return itAttribute->second;
    }

    bool setAttributeValue(const CIdentifier& attributeID, const CString& value) override
    {
        auto itAttribute = m_attributes.find(attributeID);
        if (itAttribute == m_attributes.end())
        {
            m_attributes[attributeID] = value;
            return true;
        }
        itAttribute->second = value;
        return true;
    }

    bool hasAttribute(const CIdentifier& attributeID) const override
    {
        const auto itAttribute = m_attributes.find(attributeID);
        if (itAttribute == m_attributes.end()) { return false; }
        return true;
    }

    bool hasAttributes() const override { return !m_attributes.empty(); }

    CIdentifier getNextAttributeIdentifier(const CIdentifier& previousID) const override { return getNextIdentifier<CString>(m_attributes, previousID); }

    _IsDerivedFromClass_(T, OVK_ClassId_Kernel_Scenario_AttributableT)

protected:

    std::map<CIdentifier, CString> m_attributes;
};
}  // namespace Kernel
}  // namespace OpenViBE
