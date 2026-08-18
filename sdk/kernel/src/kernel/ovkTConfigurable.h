#pragma once

#include "ovkTKernelObject.h"

#include "ovkCParameter.h"

#include <map>

namespace OpenViBE {
namespace Kernel {
class IParameter;

template <class TBase>
class TBaseConfigurable : public TBase
{
public:

    explicit TBaseConfigurable(const IKernelContext& ctx) : TBase(ctx) { }

    ~TBaseConfigurable() override
    {
        auto itParameter = m_parameters.begin();
        while (itParameter != m_parameters.end())
        {
            // @FIXME is this really as intended, test the first, delete the second?
            if (itParameter->second.first)
            {
                delete itParameter->second.second;
                itParameter->second.second = nullptr;
            }
            ++itParameter;
        }
    }

    CIdentifier getNextParameterIdentifier(const CIdentifier& previousID) const override
    {
        return getNextIdentifier<std::pair<bool, IParameter*>>(m_parameters, previousID);
    }

    IParameter* getParameter(const CIdentifier& parameterID) override
    {
        const auto it = m_parameters.find(parameterID);
        if (it == m_parameters.end()) { return nullptr; }
        return it->second.second;
    }

    bool setParameter(const CIdentifier& parameterID, IParameter& parameter) override
    {
        this->removeParameter(parameterID);

        m_parameters[parameterID] = std::pair<bool, IParameter*>(false, &parameter);

        return true;
    }

    IParameter* createParameter(const CIdentifier& parameterID, const EParameterType parameterType, const CIdentifier& subTypeID) override
    {
        const auto it = m_parameters.find(parameterID);
        if (it != m_parameters.end()) { return nullptr; }

        IParameter* parameter = nullptr;
        switch (parameterType)
        {
            case ParameterType_UInteger: parameter = new CUIntegerParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_Integer: parameter = new CIntegerParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_Enumeration: parameter = new CEnumerationParameter(this->getKernelContext(), parameterType, subTypeID);
                break;
            case ParameterType_Boolean: parameter = new CBooleanParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_Float: parameter = new CFloatParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_String: parameter = new CStringParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_Identifier: parameter = new CIdentifierParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_Matrix: parameter = new CMatrixParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_StimulationSet: parameter = new CStimulationSetParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_MemoryBuffer: parameter = new CMemoryBufferParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_Object: parameter = new CObjectParameter(this->getKernelContext(), parameterType);
                break;
            case ParameterType_None:
            case ParameterType_Pointer: parameter = new CPointerParameter(this->getKernelContext(), parameterType);
                break;
        }

        if (parameter != nullptr) { m_parameters[parameterID] = std::pair<bool, IParameter*>(true, parameter); }

        return parameter;
    }

    bool removeParameter(const CIdentifier& rParameterIdentifier) override
    {
        auto itParameter = m_parameters.find(rParameterIdentifier);
        if (itParameter == m_parameters.end()) { return false; }

        if (itParameter->second.first) { delete itParameter->second.second; }
        m_parameters.erase(itParameter);

        return true;
    }

    _IsDerivedFromClass_Final_(TBase, OVK_ClassId_Kernel_ConfigurableT)

private:

    std::map<CIdentifier, std::pair<bool, IParameter*>> m_parameters;
};
}  // namespace Kernel
}  // namespace OpenViBE
