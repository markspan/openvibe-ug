#pragma once

#include "../../ovp_defines.h"
#include "ovpCEBMLBaseEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CMasterAcquisitionEncoder final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_MasterAcquisitionEncoder)

protected:
    Kernel::IAlgorithmProxy* m_acquisitionStreamEncoder         = nullptr;
    Kernel::IAlgorithmProxy* m_experimentInfoStreamEncoder      = nullptr;
    Kernel::IAlgorithmProxy* m_signalStreamEncoder              = nullptr;
    Kernel::IAlgorithmProxy* m_stimulationStreamEncoder         = nullptr;
    Kernel::IAlgorithmProxy* m_channelLocalisationStreamEncoder = nullptr;
    Kernel::IAlgorithmProxy* m_channelUnitsStreamEncoder        = nullptr;
};

class CMasterAcquisitionEncoderDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Master acquisition stream encoder"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Stream codecs/Encoders"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_MasterAcquisitionEncoder; }
    IPluginObject* create() override { return new CMasterAcquisitionEncoder(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputTrigger(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader, "Encode header");
        prototype.addInputTrigger(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer, "Encode buffer");
        prototype.addInputTrigger(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeEnd, "Encode end");

        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectID, "Subject identifier", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectAge, "Subject age", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectGender, "Subject gender", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalMatrix, "Signal matrix", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalSampling, "Signal sampling rate",
                                    Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_StimulationSet, "Stimulation set",
                                    Kernel::ParameterType_StimulationSet);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_BufferDuration, "Buffer duration", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelLocalisation, "Channel localisation",
                                    Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelUnits, "Channel units", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelLocalisationData, "Encode channel localisation data",
                                    Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelUnitData, "Encode channel unit data",
                                    Kernel::ParameterType_Boolean);

        prototype.addOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer, "Encoded memory buffer",
                                     Kernel::ParameterType_MemoryBuffer);

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_MasterAcquisitionEncoderDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
