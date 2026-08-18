#pragma once

#include "../ovkTKernelObject.h"

#include "ovkTAttributable.h"

#include <iostream>

namespace OpenViBE {
namespace Kernel {
class CScenario;

class CComment final : public TAttributable<TKernelObject<IComment>>
{
public:

    CComment(const IKernelContext& ctx, CScenario& rOwnerScenario);
    ~CComment() override {}
    CIdentifier getIdentifier() const override { return m_id; }
    CString getText() const override { return m_text; }
    bool setIdentifier(const CIdentifier& id) override;
    bool setText(const CString& sText) override;
    bool initializeFromExistingComment(const IComment& rExisitingComment) override;
    bool acceptVisitor(IObjectVisitor& rObjectVisitor) override;

    _IsDerivedFromClass_Final_(TAttributable<TKernelObject<IComment>>, OVK_ClassId_Kernel_Scenario_Comment)

protected:

    CScenario& m_rOwnerScenario;

    CIdentifier m_id = CIdentifier::undefined();
    CString m_text;
};
}  // namespace Kernel
}  // namespace OpenViBE
