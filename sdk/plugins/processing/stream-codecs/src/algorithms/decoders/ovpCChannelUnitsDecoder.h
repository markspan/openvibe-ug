#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixDecoder.h"
#include <stack>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CChannelUnitsDecoder final : public CStreamedMatrixDecoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixDecoder, OVP_ClassId_Algorithm_ChannelUnitsDecoder)

    // ebml callbacks
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;

protected:
    Kernel::TParameterHandler<bool> op_bDynamic;

private:
    std::stack<EBML::CIdentifier> m_nodes;
    // size_t m_unitIdx = 0;
};

class CChannelUnitsDecoderDesc final : public CStreamedMatrixDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Channel units stream decoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ChannelUnitsDecoder; }
    IPluginObject* create() override { return new CChannelUnitsDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixDecoderDesc::getAlgorithmPrototype(prototype);

        prototype.addOutputParameter(OVP_Algorithm_ChannelUnitsDecoder_OutputParameterId_Dynamic, "Dynamic", Kernel::ParameterType_Boolean);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixDecoderDesc, OVP_ClassId_Algorithm_ChannelUnitsDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
