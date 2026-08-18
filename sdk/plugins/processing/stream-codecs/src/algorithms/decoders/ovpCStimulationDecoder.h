#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseDecoder.h"
#include <stack>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CStimulationDecoder final : public CEBMLBaseDecoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoder, OVP_ClassId_Algorithm_StimulationDecoder)

    // ebml callbacks
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;

protected:
    Kernel::TParameterHandler<CStimulationSet*> op_stimulationSet;

private:
    std::stack<EBML::CIdentifier> m_nodes;

    size_t m_stimulationIdx = 0;
    CStimulationSet m_stimSet;
};

class CStimulationDecoderDesc final : public CEBMLBaseDecoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stimulation stream decoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Decoders"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_StimulationDecoder; }
    IPluginObject* create() override { return new CStimulationDecoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CEBMLBaseDecoderDesc::getAlgorithmPrototype(prototype);
        prototype.addOutputParameter(OVP_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet, "Stimulation set",
                                     Kernel::ParameterType_StimulationSet);
        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseDecoderDesc, OVP_ClassId_Algorithm_StimulationDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
