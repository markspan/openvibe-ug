#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CStreamStructureDecoder final : public CEBMLBaseDecoder
{
public:
    CStreamStructureDecoder();
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoder, OVP_ClassId_Algorithm_StreamStructureDecoder)

    // ebml callbacks
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;
};

class CStreamStructureDecoderDesc final : public CEBMLBaseDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stream Structure Decoder"; }
    CString getAuthorName() const override { return "Jozef Legeny"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_StreamStructureDecoder; }
    IPluginObject* create() override { return new CStreamStructureDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CEBMLBaseDecoderDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoderDesc, OVP_ClassId_Algorithm_StreamStructureDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
