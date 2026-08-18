#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CFeatureVectorEncoder final : public CStreamedMatrixEncoder
{
public:
    void release() override { delete this; }
    bool processHeader() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoder, OVP_ClassId_Algorithm_FeatureVectorEncoder)
};

class CFeatureVectorEncoderDesc final : public CStreamedMatrixEncoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Feature vector stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_FeatureVectorEncoder; }
    IPluginObject* create() override { return new CFeatureVectorEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixEncoderDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixEncoderDesc, OVP_ClassId_Algorithm_FeatureVectorEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
