#pragma once

#include "ovkTKernelObject.h"

#include <cstring>

namespace OpenViBE {
namespace Kernel {
template <class TBase, class TType>
class TBaseParameter : public TBase
{
public:

    TBaseParameter(const IKernelContext& ctx, const EParameterType type, const CIdentifier& subTypeID = CIdentifier::undefined())
        : TBase(ctx), m_valueRef(nullptr), m_value(0), m_parameterType(type), m_subTypeID(subTypeID) { }

    uint64_t getParameterSize() const override { return sizeof(TType); }
    EParameterType getType() const override { return m_parameterType; }
    CIdentifier getSubTypeIdentifier() const override { return m_subTypeID; }

    bool clearReferenceTarget() override
    {
        m_valueRef     = NULL;
        m_parameterRef = nullptr;
        return true;
    }

    bool getReferenceTarget(IParameter*& pParameterRef) const override
    {
        pParameterRef = m_parameterRef;
        return true;
    }

    bool setReferenceTarget(IParameter* pParameterRef) override
    {
        if (m_valueRef) { m_valueRef = NULL; }
        m_parameterRef = pParameterRef;
        return true;
    }

    bool getReferenceTarget(void* pValue) const override
    {
        memcpy(&pValue, &m_valueRef, sizeof(TType*));
        return true;
    }

    bool setReferenceTarget(const void* pValue) override
    {
        if (m_parameterRef) { m_parameterRef = nullptr; }
        memcpy(&m_valueRef, &pValue, sizeof(TType*));
        return true;
    }

    bool getValue(void* pValue) const override
    {
        if (m_parameterRef) { return m_parameterRef->getValue(pValue); }
        if (m_valueRef) { memcpy(pValue, m_valueRef, sizeof(TType)); }
        else { memcpy(pValue, &m_value, sizeof(TType)); }
        return true;
    }

    bool setValue(const void* pValue) override
    {
        if (m_parameterRef) { return m_parameterRef->setValue(pValue); }
        if (m_valueRef) { memcpy(m_valueRef, pValue, sizeof(TType)); }
        else { memcpy(&m_value, pValue, sizeof(TType)); }
        return true;
    }

    _IsDerivedFromClass_(TBase, OVK_ClassId_Kernel_ParameterT)

protected:

    IParameter* m_parameterRef = nullptr;
    TType* m_valueRef          = nullptr;
    TType m_value;
    EParameterType m_parameterType;
    CIdentifier m_subTypeID = CIdentifier::undefined();
};
}  // namespace Kernel
}  // namespace OpenViBE
