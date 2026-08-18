#pragma once

#include "../ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class CAlgorithmProxy;

class CAlgorithmProto final : public TKernelObject<IAlgorithmProto>
{
public:

    CAlgorithmProto(const IKernelContext& ctx, CAlgorithmProxy& rAlgorithmProxy);
    bool addInputParameter(const CIdentifier& inputParameterID, const CString& sInputName, EParameterType eParameterType,
                           const CIdentifier& subTypeID) override;
    bool addOutputParameter(const CIdentifier& outputParameterID, const CString& sOutputName, EParameterType eParameterType,
                            const CIdentifier& subTypeID) override;
    bool addInputTrigger(const CIdentifier& inputTriggerID, const CString& rInputTriggerName) override;
    bool addOutputTrigger(const CIdentifier& outputTriggerID, const CString& rOutputTriggerName) override;

    _IsDerivedFromClass_Final_(TKernelObject<IAlgorithmProto>, OVK_ClassId_Kernel_Algorithm_AlgorithmProto)

protected:

    CAlgorithmProxy& m_algorithmProxy;
};
}  // namespace Kernel
}  // namespace OpenViBE
