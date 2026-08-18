#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CAcquisitionEncoder final : public CEBMLBaseEncoder
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processHeader() override;
    bool processBuffer() override;

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseEncoder, OVP_ClassId_Algorithm_AcquisitionEncoder)

protected:
    Kernel::TParameterHandler<uint64_t> ip_bufferDuration;
    Kernel::TParameterHandler<CMemoryBuffer*> ip_experimentInfoStream;
    Kernel::TParameterHandler<CMemoryBuffer*> ip_signalStream;
    Kernel::TParameterHandler<CMemoryBuffer*> ip_stimulationStream;
    Kernel::TParameterHandler<CMemoryBuffer*> ip_channelLocalisationStream;
    Kernel::TParameterHandler<CMemoryBuffer*> ip_channelUnitsStream;
};

class CAcquisitionEncoderDesc final : public CEBMLBaseEncoderDesc
{
public:
    void release() override { }

    CString getName() const override { return "Acquisition stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_AcquisitionEncoder; }
    IPluginObject* create() override { return new CAcquisitionEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CEBMLBaseEncoderDesc::getAlgorithmPrototype(prototype);

        prototype.addInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_BufferDuration, "Buffer duration", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ExperimentInfoStream, "Experiment information stream",
                                    Kernel::ParameterType_MemoryBuffer);
        prototype.addInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_SignalStream, "Signal stream", Kernel::ParameterType_MemoryBuffer);
        prototype.addInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_StimulationStream, "Stimulation stream",
                                    Kernel::ParameterType_MemoryBuffer);
        prototype.addInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelLocalisationStream, "Channel localisation stream",
                                    Kernel::ParameterType_MemoryBuffer);
        prototype.addInputParameter(OVP_Algorithm_AcquisitionEncoder_InputParameterId_ChannelUnitsStream, "Channel units stream",
                                    Kernel::ParameterType_MemoryBuffer);

        return true;
    }

    _IsDerivedFromClass_Final_(StreamCodecs::CEBMLBaseEncoderDesc, OVP_ClassId_Algorithm_AcquisitionEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
