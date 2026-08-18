#pragma once

#include "../ovkTKernelObject.h"

#include "ovkTAttributable.h"

namespace OpenViBE {
namespace Kernel {
class CScenario;

class CLink final : public TAttributable<TKernelObject<ILink>>
{
public:

    CLink(const IKernelContext& ctx, CScenario& ownerScenario) : TAttributable<TKernelObject<ILink>>(ctx), m_ownerScenario(ownerScenario) {}
    bool initializeFromExistingLink(const ILink& link) override;
    bool setIdentifier(const CIdentifier& identifier) override;
    CIdentifier getIdentifier() const override { return m_id; }
    bool setSource(const CIdentifier& boxId, size_t boxOutputIdx, CIdentifier boxOutputID) override;
    bool setTarget(const CIdentifier& boxId, size_t boxInputIdx, CIdentifier boxInputID) override;

    bool getSource(CIdentifier& boxId, size_t& boxOutputIdx, CIdentifier& boxOutputID) const override;
    CIdentifier getSourceBoxIdentifier() const override { return m_srcBoxID; }
    size_t getSourceBoxOutputIndex() const override { return m_srcOutputIdx; }
    CIdentifier getSourceBoxOutputIdentifier() const override { return m_srcBoxOutputID; }

    bool getTarget(CIdentifier& dstBoxID, size_t& boxInputIndex, CIdentifier& dstBoxInputID) const override;
    CIdentifier getTargetBoxIdentifier() const override { return m_dstBoxID; }
    size_t getTargetBoxInputIndex() const override { return m_dstInputIdx; }
    CIdentifier getTargetBoxInputIdentifier() const override { return m_dstBoxInputID; }

    bool acceptVisitor(IObjectVisitor& visitor) override;

    _IsDerivedFromClass_Final_(TAttributable<TKernelObject<ILink>>, OVK_ClassId_Kernel_Scenario_Link)

protected:

    CScenario& m_ownerScenario;
    CIdentifier m_id             = CIdentifier::undefined();
    CIdentifier m_srcBoxID       = CIdentifier::undefined();
    CIdentifier m_dstBoxID       = CIdentifier::undefined();
    size_t m_srcOutputIdx        = 0;
    CIdentifier m_srcBoxOutputID = CIdentifier::undefined();
    size_t m_dstInputIdx         = 0;
    CIdentifier m_dstBoxInputID  = CIdentifier::undefined();
};
}  // namespace Kernel
}  // namespace OpenViBE
