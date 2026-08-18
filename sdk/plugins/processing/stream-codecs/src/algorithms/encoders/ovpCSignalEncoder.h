#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CSignalEncoder final : public CStreamedMatrixEncoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processHeader() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoder, OVP_ClassId_Algorithm_SignalEncoder)

protected:
    Kernel::TParameterHandler<uint64_t> ip_sampling;
};


class CSignalEncoderDesc final : public CStreamedMatrixEncoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_SignalEncoder; }
    IPluginObject* create() override { return new CSignalEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixEncoderDesc::getAlgorithmPrototype(prototype);
        prototype.addInputParameter(OVP_Algorithm_SignalEncoder_InputParameterId_Sampling, "Sampling rate", Kernel::ParameterType_UInteger);
        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoderDesc, OVP_ClassId_Algorithm_SignalEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
