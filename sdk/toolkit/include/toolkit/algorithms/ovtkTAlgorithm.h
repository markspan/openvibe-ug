#pragma once

#include "../ovtk_base.h"

namespace OpenViBE {
namespace Toolkit {
template <class TAlgorithmParentClass>
class TAlgorithm : public TAlgorithmParentClass
{
public:

    TAlgorithm() { }

    bool initialize(Kernel::IAlgorithmContext& algorithmCtx) override
    {
        CScopedAlgorithm scopedAlgorithm(m_algorithmCtx, &algorithmCtx);
        return initialize();
    }

    bool uninitialize(Kernel::IAlgorithmContext& algorithmCtx) override
    {
        CScopedAlgorithm scopedAlgorithm(m_algorithmCtx, &algorithmCtx);
        return uninitialize();
    }

    bool process(Kernel::IAlgorithmContext& algorithmCtx) override
    {
        CScopedAlgorithm scopedAlgorithm(m_algorithmCtx, &algorithmCtx);
        return process();
    }

    // ====================================================================================================================================

    virtual bool initialize() { return true; }
    virtual bool uninitialize() { return true; }
    virtual bool process() = 0;

    // ====================================================================================================================================

    virtual Kernel::IAlgorithmContext& getAlgorithmContext()
    {
        return *m_algorithmCtx; // should never be null
    }

protected:

    virtual Kernel::IConfigurationManager& getConfigurationManager() { return m_algorithmCtx->getConfigurationManager(); } // should never be null
    virtual Kernel::IAlgorithmManager& getAlgorithmManager() { return m_algorithmCtx->getAlgorithmManager(); } // should never be null
    virtual Kernel::ILogManager& getLogManager() { return m_algorithmCtx->getLogManager(); } // should never be null
    virtual Kernel::CErrorManager& getErrorManager() { return m_algorithmCtx->getErrorManager(); } // should never be null
    virtual Kernel::ITypeManager& getTypeManager() { return m_algorithmCtx->getTypeManager(); } // should never be null

    virtual CIdentifier getNextInputParameterIdentifier(const CIdentifier& rPreviousInputParameterIdentifier) const
    {
        return m_algorithmCtx->getNextInputParameterIdentifier(rPreviousInputParameterIdentifier);
    }

    virtual Kernel::IParameter* getInputParameter(const CIdentifier& InputParameterID) { return m_algorithmCtx->getInputParameter(InputParameterID); }

    virtual CIdentifier getNextOutputParameterIdentifier(const CIdentifier& rPreviousOutputParameterIdentifier) const
    {
        return m_algorithmCtx->getNextOutputParameterIdentifier(rPreviousOutputParameterIdentifier);
    }

    virtual Kernel::IParameter* getOutputParameter(const CIdentifier& outputParameterID) { return m_algorithmCtx->getOutputParameter(outputParameterID); }

    virtual bool isInputTriggerActive(const CIdentifier& inputTriggerID) const { return m_algorithmCtx->isInputTriggerActive(inputTriggerID); }

    virtual bool activateOutputTrigger(const CIdentifier& outputTriggerID, const bool bTriggerState)
    {
        return m_algorithmCtx->activateOutputTrigger(outputTriggerID, bTriggerState);
    }

    // ====================================================================================================================================

    _IsDerivedFromClass_(TAlgorithmParentClass, OVTK_ClassId_)

private:

    class CScopedAlgorithm final
    {
    public:

        CScopedAlgorithm(Kernel::IAlgorithmContext*& algorithmCtxRef, Kernel::IAlgorithmContext* algorithmCtx)
            : m_algorithmCtx(algorithmCtxRef) { m_algorithmCtx = algorithmCtx; }

        ~CScopedAlgorithm() { m_algorithmCtx = nullptr; }

    protected:

        Kernel::IAlgorithmContext*& m_algorithmCtx;
    };

    Kernel::IAlgorithmContext* m_algorithmCtx = nullptr;
};
}  // namespace Toolkit
}  // namespace OpenViBE
