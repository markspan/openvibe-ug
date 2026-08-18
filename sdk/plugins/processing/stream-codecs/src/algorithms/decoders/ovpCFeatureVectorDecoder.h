#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CFeatureVectorDecoder final : public CStreamedMatrixDecoder
{
public:
    void release() override { delete this; }
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixDecoder, OVP_ClassId_Algorithm_FeatureVectorDecoder)

private:
    EBML::CIdentifier m_oTop;
};

class CFeatureVectorDecoderDesc final : public CStreamedMatrixDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Feature vector stream decoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_FeatureVectorDecoder; }
    IPluginObject* create() override { return new CFeatureVectorDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixDecoderDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixDecoderDesc, OVP_ClassId_Algorithm_FeatureVectorDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
