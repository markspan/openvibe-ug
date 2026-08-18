#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <fstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CAlgorithmBrainampFileReader final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_BrainampFileReader)

protected:
    enum class EStatus { Nothing, CommonInfos, BinrayInfos, ChannelInfos, MarkerInfos, Comment };

    enum class EBinaryFormat { Integer16, UnsignedInteger16, Float32 };

    enum class EEndianness { LittleEndian, BigEndian };

    typedef struct SStimulaton
    {
        uint64_t id;
        uint64_t startIdx;
        uint64_t duration;
        std::string name;
    } stimulation_t;

    Kernel::TParameterHandler<CString*> ip_filename;
    Kernel::TParameterHandler<double> ip_epochDuration;
    Kernel::TParameterHandler<uint64_t> ip_seekTime;
    Kernel::TParameterHandler<bool> ip_convertStimuli;

    Kernel::TParameterHandler<uint64_t> op_startTime;
    Kernel::TParameterHandler<uint64_t> op_endTime;
    Kernel::TParameterHandler<uint64_t> op_sampling;
    Kernel::TParameterHandler<CMatrix*> op_signalMatrix;
    Kernel::TParameterHandler<CStimulationSet*> op_stimulations;

    CString m_filename;

    EBinaryFormat m_binaryFormat    = EBinaryFormat::Integer16;
    EEndianness m_endianness        = EEndianness::LittleEndian;
    uint32_t m_nChannel             = 0;
    uint64_t m_startSampleIdx       = 0;
    uint64_t m_endSampleIdx         = 0;
    uint64_t m_sampleCountPerBuffer = 0;

    uint8_t* m_buffer = nullptr;
    std::vector<double> m_channelScales;
    std::vector<stimulation_t> m_stimulations;

    std::ifstream m_headerFile;
    std::ifstream m_dataFile;
    std::ifstream m_markerFile;
};

class CAlgorithmBrainampFileReaderDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Brainamp file reader"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Reads input having the BrainAmp file format"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/Brainamp"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_BrainampFileReader; }
    IPluginObject* create() override { return new CAlgorithmBrainampFileReader; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_Filename, "Filename", Kernel::ParameterType_String);
        prototype.addInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_EpochDuration, "Epoch duration", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_SeekTime, "Seek time", Kernel::ParameterType_Integer);
        prototype.addInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_ConvertStimuli, "Convert stimuli", Kernel::ParameterType_Boolean);
        prototype.addOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentStartTime, "Current start time", Kernel::ParameterType_Integer);
        prototype.addOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentEndTime, "Current end time", Kernel::ParameterType_Integer);
        prototype.addOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Sampling, "Sampling rate", Kernel::ParameterType_Integer);
        prototype.addOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_SignalMatrix, "Signal samples", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Stimulations, "Stimulations", Kernel::ParameterType_StimulationSet);
        prototype.addInputTrigger(OVP_Algorithm_BrainampFileReader_InputTriggerId_Open, "Open");
        prototype.addInputTrigger(OVP_Algorithm_BrainampFileReader_InputTriggerId_Seek, "Seek");
        prototype.addInputTrigger(OVP_Algorithm_BrainampFileReader_InputTriggerId_Next, "Next");
        prototype.addInputTrigger(OVP_Algorithm_BrainampFileReader_InputTriggerId_Close, "Close");
        prototype.addOutputTrigger(OVP_Algorithm_BrainampFileReader_OutputTriggerId_Error, "Error");
        prototype.addOutputTrigger(OVP_Algorithm_BrainampFileReader_OutputTriggerId_DataProduced, "Data produced");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_BrainampFileReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
