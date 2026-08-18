#pragma once

#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"

#include <openvibe/ovIObjectVisitor.h>
#include <openvibe/kernel/ovIObjectVisitorContext.h>

#include <xml/IReader.h>

#define OVD_AttributeId_SettingOverrideFilename         OpenViBE::CIdentifier(0x8D21FF41, 0xDF6AFE7E)

namespace OpenViBE {
namespace Kernel {
class CBoxSettingModifierVisitor final : public IObjectVisitor, public XML::IReaderCallback
{
public:

    explicit CBoxSettingModifierVisitor(IConfigurationManager* pConfigurationManager = nullptr)
        : IObjectVisitor(), m_ConfigManager(pConfigurationManager) {}

    void openChild(const char* name, const char** sAttributeName, const char** sAttributeValue, const size_t nAttribute) override;
    void processChildData(const char* data) override;
    void closeChild() override;
    bool processBegin(IObjectVisitorContext& visitorCtx, IBox& box) override;
    bool processEnd(IObjectVisitorContext& visitorCtx, IBox& box) override;

    IObjectVisitorContext* m_ObjectVisitorCtx = nullptr;
    IBox* m_Box                               = nullptr;
    size_t m_SettingIdx                       = 0;
    bool m_IsParsingSettingValue              = false;
    bool m_IsParsingSettingOverride           = false;
    IConfigurationManager* m_ConfigManager    = nullptr;

    _IsDerivedFromClass_Final_(IObjectVisitor, CIdentifier::undefined())
};
}  //namespace Kernel
}  //namespace OpenViBE
