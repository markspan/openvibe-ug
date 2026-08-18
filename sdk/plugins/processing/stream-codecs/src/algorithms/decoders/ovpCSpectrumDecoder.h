#pragma once

#include "../../ovp_defines.h"
#include "ovpCStreamedMatrixDecoder.h"
#include <iomanip>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CSpectrumDecoder final : public CStreamedMatrixDecoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixDecoder, OVP_ClassId_Algorithm_SpectrumDecoder)

    // ebml callbacks
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;

protected:
    Kernel::TParameterHandler<CMatrix*> op_frequencyAbscissa;
    Kernel::TParameterHandler<uint64_t> op_sampling;


private:
    std::stack<EBML::CIdentifier> m_nodes;

    size_t m_frequencyBandIdx = 0;

    // Value of the current lower frequency of the band. Only used to read old spectrum format.
    double m_lowerFreq = 0;
};

class CSpectrumDecoderDesc final : public CStreamedMatrixDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spectrum stream decoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Decodes the Spectrum type streams."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_SpectrumDecoder; }
    IPluginObject* create() override { return new CSpectrumDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CStreamedMatrixDecoderDesc::getAlgorithmPrototype(prototype);

        prototype.addOutputParameter(OVP_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa, "Frequency abscissa", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_SpectrumDecoder_OutputParameterId_Sampling, "Sampling rate", Kernel::ParameterType_UInteger);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CStreamedMatrixDecoderDesc, OVP_ClassId_Algorithm_SpectrumDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
