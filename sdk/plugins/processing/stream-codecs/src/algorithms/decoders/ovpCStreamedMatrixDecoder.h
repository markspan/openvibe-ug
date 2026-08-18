#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseDecoder.h"
#include <stack>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CStreamedMatrixDecoder : public CEBMLBaseDecoder
{
public:
    CStreamedMatrixDecoder() { }
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoder, OVP_ClassId_Algorithm_StreamedMatrixDecoder)

    // ebml callbacks
    bool isMasterChild(const EBML::CIdentifier& id) override;
    void openChild(const EBML::CIdentifier& id) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;

protected:
    Kernel::TParameterHandler<CMatrix*> op_pMatrix;

private:
    enum class EParsingStatus { Nothing, Header, Buffer, Dimension };

    std::stack<EBML::CIdentifier> m_nodes;

    EParsingStatus m_status    = EParsingStatus::Nothing;
    size_t m_dimensionIdx      = 0;
    size_t m_dimensionEntryIdx = 0;
    // size_t mdimensionEntryIdxUnit = 0;
    size_t m_size = 0;
};

class CStreamedMatrixDecoderDesc : public CEBMLBaseDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Streamed matrix stream decoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_StreamedMatrixDecoder; }
    IPluginObject* create() override { return new CStreamedMatrixDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CEBMLBaseDecoderDesc::getAlgorithmPrototype(prototype);
        prototype.addOutputParameter(OVP_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix, "Matrix", Kernel::ParameterType_Matrix);
        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoderDesc, OVP_ClassId_Algorithm_StreamedMatrixDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
