#include "ovkCAlgorithmProto.h"
#include "ovkCAlgorithmProxy.h"

namespace OpenViBE {
namespace Kernel {

CAlgorithmProto::CAlgorithmProto(const IKernelContext& ctx, CAlgorithmProxy& rAlgorithmProxy)
    : TKernelObject<IAlgorithmProto>(ctx), m_algorithmProxy(rAlgorithmProxy) {}

bool CAlgorithmProto::addInputParameter(const CIdentifier& inputParameterID, const CString& sInputName,
                                        const EParameterType eParameterType, const CIdentifier& subTypeID)
{
    return m_algorithmProxy.addInputParameter(inputParameterID, sInputName, eParameterType, subTypeID);
}

bool CAlgorithmProto::addOutputParameter(const CIdentifier& outputParameterID, const CString& sOutputName,
                                         const EParameterType eParameterType, const CIdentifier& subTypeID)
{
    return m_algorithmProxy.addOutputParameter(outputParameterID, sOutputName, eParameterType, subTypeID);
}

bool CAlgorithmProto::addInputTrigger(const CIdentifier& inputTriggerID, const CString& rInputTriggerName)
{
    return m_algorithmProxy.addInputTrigger(inputTriggerID, rInputTriggerName);
}

bool CAlgorithmProto::addOutputTrigger(const CIdentifier& outputTriggerID, const CString& rOutputTriggerName)
{
    return m_algorithmProxy.addOutputTrigger(outputTriggerID, rOutputTriggerName);
}

}  // namespace Kernel
}  // namespace OpenViBE
