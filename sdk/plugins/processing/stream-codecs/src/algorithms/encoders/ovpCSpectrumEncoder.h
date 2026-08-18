#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CSpectrumEncoder final : public CStreamedMatrixEncoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processHeader() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoder, OVP_ClassId_Algorithm_SpectrumEncoder)

protected:
    Kernel::TParameterHandler<CMatrix*> ip_frequencyAbscissa;
    Kernel::TParameterHandler<uint64_t> ip_sampling;
};

class CSpectrumEncoderDesc final : public CStreamedMatrixEncoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spectrum stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Encodes a Spectrum stream."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_SpectrumEncoder; }
    IPluginObject* create() override { return new CSpectrumEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixEncoderDesc::getAlgorithmPrototype(prototype);

        prototype.addInputParameter(OVP_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa, "Frequency abscissa", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_SpectrumEncoder_InputParameterId_Sampling, "Sampling rate", Kernel::ParameterType_UInteger);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoderDesc, OVP_ClassId_Algorithm_SpectrumEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
