#include <cassert>

#include "ovkCAlgorithmProxy.h"
#include "ovkCAlgorithmContext.h"
#include "ovkCAlgorithmProto.h"

#include "../../ovk_tools.h"

#include <openvibe/ovExceptionHandler.h>

namespace OpenViBE {
namespace Kernel {

CAlgorithmProxy::CAlgorithmProxy(const IKernelContext& ctx, Plugins::IAlgorithm& rAlgorithm, const Plugins::IAlgorithmDesc& algorithmDesc)
    : TKernelObject<IAlgorithmProxy>(ctx), m_algorithmDesc(algorithmDesc), m_algorithm(rAlgorithm)
{
    m_iConfigurable = dynamic_cast<IConfigurable*>(getKernelContext().getKernelObjectFactory().createObject(OV_ClassId_Kernel_Configurable));
    m_oConfigurable = dynamic_cast<IConfigurable*>(getKernelContext().getKernelObjectFactory().createObject(OV_ClassId_Kernel_Configurable));

    // FIXME
    CAlgorithmProto algorithmProto(ctx, *this);
    algorithmDesc.getAlgorithmPrototype(algorithmProto);
}

CAlgorithmProxy::~CAlgorithmProxy()
{
    getKernelContext().getKernelObjectFactory().releaseObject(m_oConfigurable);
    getKernelContext().getKernelObjectFactory().releaseObject(m_iConfigurable);
}

bool CAlgorithmProxy::addInputParameter(const CIdentifier& parameterID, const CString& name, const EParameterType parameterType,
                                        const CIdentifier& subTypeID)
{
    OV_ERROR_UNLESS_KRF(m_iConfigurable->getParameter(parameterID) == nullptr,
                        "For algorithm " << m_algorithmDesc.getName() << " : Input parameter id " << parameterID.str() << " already exists",
                        ErrorType::BadResourceCreation);

    m_iConfigurable->createParameter(parameterID, parameterType, subTypeID);
    m_iParameterNames[parameterID] = name;
    return true;
}

CIdentifier CAlgorithmProxy::getNextInputParameterIdentifier(const CIdentifier& parameterID) const
{
    return m_iConfigurable->getNextParameterIdentifier(parameterID);
}

IParameter* CAlgorithmProxy::getInputParameter(const CIdentifier& parameterID)
{
    IParameter* parameter = m_iConfigurable->getParameter(parameterID);

    OV_ERROR_UNLESS_KRN(
        parameter, "For algorithm " << m_algorithmDesc.getName() << " : Requested null input parameter id " << parameterID.str(),
        ErrorType::ResourceNotFound);

    return parameter;
}

EParameterType CAlgorithmProxy::getInputParameterType(const CIdentifier& parameterID) const
{
    IParameter* parameter = m_iConfigurable->getParameter(parameterID);
    if (!parameter) { return ParameterType_None; }
    return parameter->getType();
}

CString CAlgorithmProxy::getInputParameterName(const CIdentifier& parameterID) const
{
    const auto itName = m_iParameterNames.find(parameterID);
    if (itName == m_iParameterNames.end()) { return ""; }
    return itName->second;
}

bool CAlgorithmProxy::removeInputParameter(const CIdentifier& parameterID)
{
    if (!m_iConfigurable->removeParameter(parameterID)) { return false; }
    m_iParameterNames.erase(m_iParameterNames.find(parameterID));
    return true;
}

bool CAlgorithmProxy::addOutputParameter(const CIdentifier& parameterID, const CString& name, const EParameterType parameterType,
                                         const CIdentifier& subTypeID)
{
    OV_ERROR_UNLESS_KRF(m_oConfigurable->getParameter(parameterID) == nullptr,
                        "For algorithm " << m_algorithmDesc.getName() << " : Output parameter id " << parameterID.str() <<
                        " already exists",
                        ErrorType::BadResourceCreation);

    m_oConfigurable->createParameter(parameterID, parameterType, subTypeID);
    m_oParameterNames[parameterID] = name;
    return true;
}

CIdentifier CAlgorithmProxy::getNextOutputParameterIdentifier(const CIdentifier& parameterID) const
{
    return m_oConfigurable->getNextParameterIdentifier(parameterID);
}

IParameter* CAlgorithmProxy::getOutputParameter(const CIdentifier& parameterID)
{
    IParameter* parameter = m_oConfigurable->getParameter(parameterID);

    OV_ERROR_UNLESS_KRN(
        parameter, "For algorithm " << m_algorithmDesc.getName() << " : Requested null output parameter id " << parameterID.str(),
        ErrorType::ResourceNotFound);

    return parameter;
}

EParameterType CAlgorithmProxy::getOutputParameterType(const CIdentifier& parameterID) const
{
    IParameter* parameter = m_oConfigurable->getParameter(parameterID);
    if (!parameter) { return ParameterType_None; }
    return parameter->getType();
}

CString CAlgorithmProxy::getOutputParameterName(const CIdentifier& parameterID) const
{
    const auto itName = m_oParameterNames.find(parameterID);
    if (itName == m_oParameterNames.end()) { return ""; }
    return itName->second;
}

bool CAlgorithmProxy::removeOutputParameter(const CIdentifier& parameterID)
{
    if (!m_oConfigurable->removeParameter(parameterID)) { return false; }
    m_oParameterNames.erase(m_oParameterNames.find(parameterID));
    return true;
}

bool CAlgorithmProxy::addInputTrigger(const CIdentifier& triggerID, const CString& name)
{
    if (m_iTriggers.find(triggerID) != m_iTriggers.end()) { return false; }
    m_iTriggers[triggerID].first  = name;
    m_iTriggers[triggerID].second = false;
    return true;
}

CIdentifier CAlgorithmProxy::getNextInputTriggerIdentifier(const CIdentifier& triggerID) const
{
    return getNextIdentifier<std::pair<CString, bool>>(m_iTriggers, triggerID);
}

CString CAlgorithmProxy::getInputTriggerName(const CIdentifier& triggerID) const
{
    const auto itTrigger = m_iTriggers.find(triggerID);
    if (itTrigger == m_iTriggers.end()) { return ""; }
    return itTrigger->second.first;
}

bool CAlgorithmProxy::isInputTriggerActive(const CIdentifier& triggerID) const
{
    const auto itTrigger = m_iTriggers.find(triggerID);
    if (itTrigger == m_iTriggers.end()) { return false; }
    return itTrigger->second.second;
}

bool CAlgorithmProxy::activateInputTrigger(const CIdentifier& triggerID, const bool /*triggerState*/)
{
    const auto itTrigger = m_iTriggers.find(triggerID);
    if (itTrigger == m_iTriggers.end()) { return false; }
    itTrigger->second.second = true;
    return true;
}

bool CAlgorithmProxy::removeInputTrigger(const CIdentifier& triggerID)
{
    const auto itTrigger = m_iTriggers.find(triggerID);
    if (itTrigger == m_iTriggers.end()) { return false; }
    m_iTriggers.erase(itTrigger);
    return true;
}

bool CAlgorithmProxy::addOutputTrigger(const CIdentifier& triggerID, const CString& name)
{
    if (m_oTriggers.find(triggerID) != m_oTriggers.end()) { return false; }
    m_oTriggers[triggerID].first  = name;
    m_oTriggers[triggerID].second = false;
    return true;
}

CIdentifier CAlgorithmProxy::getNextOutputTriggerIdentifier(const CIdentifier& triggerID) const
{
    return getNextIdentifier<std::pair<CString, bool>>(m_oTriggers, triggerID);
}

CString CAlgorithmProxy::getOutputTriggerName(const CIdentifier& triggerID) const
{
    const auto itTrigger = m_oTriggers.find(triggerID);
    if (itTrigger == m_oTriggers.end()) { return ""; }
    return itTrigger->second.first;
}

bool CAlgorithmProxy::isOutputTriggerActive(const CIdentifier& triggerID) const
{
    const auto itTrigger = m_oTriggers.find(triggerID);
    if (itTrigger == m_oTriggers.end()) { return false; }
    return itTrigger->second.second;
}

bool CAlgorithmProxy::activateOutputTrigger(const CIdentifier& triggerID, const bool /*state*/)
{
    const auto itTrigger = m_oTriggers.find(triggerID);
    if (itTrigger == m_oTriggers.end()) { return false; }
    itTrigger->second.second = true;
    return true;
}

bool CAlgorithmProxy::removeOutputTrigger(const CIdentifier& triggerID)
{
    const auto itTrigger = m_oTriggers.find(triggerID);
    if (itTrigger == m_oTriggers.end()) { return false; }
    m_oTriggers.erase(itTrigger);
    return true;
}


bool CAlgorithmProxy::initialize()
{
    assert(!m_isInitialized);

    return translateException([&]()
                              {
                                  CAlgorithmContext context(getKernelContext(), *this, m_algorithmDesc);
                                  // The dual state initialized or not does not take into account
                                  // a partially initialized state. Thus, we have to trust algorithms to implement
                                  // their initialization routine as a rollback transaction mechanism
                                  m_isInitialized = m_algorithm.initialize(context);
                                  return m_isInitialized;
                              },
                              std::bind(&CAlgorithmProxy::handleException, this, "Algorithm initialization", std::placeholders::_1));
}

bool CAlgorithmProxy::uninitialize()
{
    assert(m_isInitialized);

    return translateException([&]()
                              {
                                  CAlgorithmContext context(getKernelContext(), *this, m_algorithmDesc);
                                  return m_algorithm.uninitialize(context);
                              },
                              std::bind(&CAlgorithmProxy::handleException, this, "Algorithm uninitialization", std::placeholders::_1));
}

bool CAlgorithmProxy::process()
{
    assert(m_isInitialized);

    return translateException([&]()
                              {
                                  CAlgorithmContext context(getKernelContext(), *this, m_algorithmDesc);
                                  this->setAllOutputTriggers(false);
                                  const bool result = m_algorithm.process(context);
                                  this->setAllInputTriggers(false);
                                  return result;
                              },
                              std::bind(&CAlgorithmProxy::handleException, this, "Algorithm processing", std::placeholders::_1));
}

bool CAlgorithmProxy::process(const CIdentifier& triggerID)
{
    assert(m_isInitialized);
    if (!this->activateInputTrigger(triggerID, true)) { return false; }
    return this->process();
}

void CAlgorithmProxy::setAllInputTriggers(const bool status) { for (auto& trigger : m_iTriggers) { trigger.second.second = status; } }

void CAlgorithmProxy::setAllOutputTriggers(const bool status) { for (auto& trigger : m_oTriggers) { trigger.second.second = status; } }

bool CAlgorithmProxy::isAlgorithmDerivedFrom(const CIdentifier& classID) { return m_algorithm.isDerivedFromClass(classID); }

void CAlgorithmProxy::handleException(const char* errorHint, const std::exception& exception)
{
    this->getLogManager() << LogLevel_Error << "Exception caught in algorithm\n";
    this->getLogManager() << LogLevel_Error << "  [name: " << this->getAlgorithmDesc().getName() << "]\n";
    this->getLogManager() << LogLevel_Error << "  [class identifier: " << this->getAlgorithmDesc().getCreatedClass() << "]\n";
    this->getLogManager() << LogLevel_Error << "  [hint: " << (errorHint ? errorHint : "no hint") << "]\n";
    this->getLogManager() << LogLevel_Error << "  [cause: " << exception.what() << "]\n";

    OV_ERROR_KRV("Caught exception: " << exception.what(), Kernel::ErrorType::ExceptionCaught);
}

}  // namespace Kernel
}  // namespace OpenViBE
