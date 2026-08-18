#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CChannelLocalisationEncoder final : public CStreamedMatrixEncoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processHeader() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoder, OVP_ClassId_Algorithm_ChannelLocalisationEncoder)

protected:
    Kernel::TParameterHandler<bool> ip_bDynamic;
};


class CChannelLocalisationEncoderDesc final : public CStreamedMatrixEncoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Channel localisation stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ChannelLocalisationEncoder; }
    IPluginObject* create() override { return new CChannelLocalisationEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixEncoderDesc::getAlgorithmPrototype(prototype);

        prototype.addInputParameter(OVP_Algorithm_ChannelLocalisationEncoder_InputParameterId_Dynamic, "Dynamic", Kernel::ParameterType_Boolean);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoderDesc, OVP_ClassId_Algorithm_ChannelLocalisationEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
