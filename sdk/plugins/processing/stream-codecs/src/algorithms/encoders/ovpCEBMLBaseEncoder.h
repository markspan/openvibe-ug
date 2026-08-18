#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ebml/IWriter.h>
#include <ebml/IWriterHelper.h>
#include <ebml/TWriterCallbackProxy.h>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CEBMLBaseEncoder : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    CEBMLBaseEncoder() : m_callbackProxy(*this, &CEBMLBaseEncoder::write) {}
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;
    virtual bool processHeader() { return true; }
    virtual bool processBuffer() { return true; }
    virtual bool processEnd() { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_EBMLBaseEncoder)

    // ebml callbacks
    virtual void write(const void* buffer, size_t size);

protected:
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    EBML::IWriterHelper* m_writerHelper = nullptr;
    EBML::IWriter* m_writer             = nullptr;
    EBML::TWriterCallbackProxy1<CEBMLBaseEncoder> m_callbackProxy;
};

class CEBMLBaseEncoderDesc : public IAlgorithmDesc
{
public:
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer, "Encoded memory buffer",
                                     Kernel::ParameterType_MemoryBuffer);
        prototype.addInputTrigger(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader, "Encode header");
        prototype.addInputTrigger(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer, "Encode buffer");
        prototype.addInputTrigger(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd, "Encode end");

        return true;
    }

    _IsDerivedFromClass_(IAlgorithmDesc, OVP_ClassId_Algorithm_EBMLBaseEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
