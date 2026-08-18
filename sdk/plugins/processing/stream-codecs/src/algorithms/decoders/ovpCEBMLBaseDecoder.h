#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ebml/IReader.h>
#include <ebml/IReaderHelper.h>
#include <ebml/TReaderCallbackProxy.h>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CEBMLBaseDecoder : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    CEBMLBaseDecoder();
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_EBMLBaseDecoder)

    // ebml callbacks
    virtual bool isMasterChild(const EBML::CIdentifier& identifier);
    virtual void openChild(const EBML::CIdentifier& identifier);
    virtual void processChildData(const void* /*buffer*/, const size_t /*size*/) { }
    virtual void closeChild() { }

protected:
    EBML::IReaderHelper* m_readerHelper = nullptr;
    EBML::IReader* m_reader             = nullptr;
    EBML::TReaderCallbackProxy1<CEBMLBaseDecoder> m_callbackProxy;

    Kernel::TParameterHandler<CMemoryBuffer*> ip_bufferToDecode;
};

class CEBMLBaseDecoderDesc : public IAlgorithmDesc
{
public:
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_EBMLDecoder_InputParameterId_MemoryBufferToDecode, "Memory buffer to decode",
                                    Kernel::ParameterType_MemoryBuffer);

        prototype.addOutputTrigger(OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedHeader, "Received header");
        prototype.addOutputTrigger(OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedBuffer, "Received buffer");
        prototype.addOutputTrigger(OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedEnd, "Received end");

        return true;
    }

    _IsDerivedFromClass_(IAlgorithmDesc, OVP_ClassId_Algorithm_EBMLBaseDecoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
