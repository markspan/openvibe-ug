#pragma once

#include "../ovp_defines.h"
#include "../ovp_gdf_helpers.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <fstream>
#include <string>
#include <vector>

#define GDFReader_ExperimentInfoOutput  0
#define GDFReader_SignalOutput          1
#define GDFReader_StimulationOutput     2

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

/// <summary> The GDF reader plugin main class. </summary>
/// <seealso cref="Toolkit::TBoxAlgorithm{IBoxAlgorithm}" />
class CGDFFileReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    //Helper structures
    class CExperimentInfoHeader
    {
    public:
        uint64_t m_ExperimentID = 0;
        std::string m_ExperimentDate;

        uint64_t m_SubjectID = 0;
        std::string m_SubjectName;
        uint64_t m_SubjectAge = 0;
        uint64_t m_SubjectSex = 0;

        uint64_t m_LaboratoryID = 0;
        std::string m_LaboratoryName;
        uint64_t m_TechnicianID = 0;
        std::string m_TechnicianName;

        bool m_ReadyToSend = false;
    };

    // Used to store information about the signal stream
    class CSignalDescription
    {
    public:
        CSignalDescription()  = default;
        ~CSignalDescription() = default;

        size_t m_Version  = 1;
        size_t m_Sampling = 0;
        size_t m_NChannel = 0;
        size_t m_NSample  = 0;
        std::vector<std::string> m_ChannelNames;
        size_t m_CurrentChannel = 0;

        bool m_ReadyToSend = false;
    };

    CGDFFileReader() { }

    void release() override {}
    bool initialize() override;
    bool uninitialize() override;

    uint64_t getClockFrequency() override { return m_clockFrequency; }
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_GDFFileReader)

protected:
    bool m_errorOccurred = false;  // true if an error has occurred while reading the GDF file

    //The GDF filename and handle
    CString m_filename;
    std::ifstream m_file;
    size_t m_fileSize        = 0;
    uint64_t m_header3Length = 0;

    float m_fileVersion = -1;

    Toolkit::TSignalEncoder<CGDFFileReader>* m_signalEncoder           = nullptr;
    Toolkit::TExperimentInfoEncoder<CGDFFileReader>* m_xpInfoEncoder   = nullptr;
    Toolkit::TStimulationEncoder<CGDFFileReader>* m_stimulationEncoder = nullptr;


    //Stream information
    uint64_t m_samplesPerBuffer = 0;  //user defined

    //input
    uint64_t m_nDataRecords     = 0;
    double m_durationDataRecord = 0;
    uint16_t m_nChannels        = 0;

    uint32_t m_nSamplesPerRecord = 0; // We only handle the files where it is the same for all the channels

    //info about channel's data type in data record
    std::vector<uint32_t> m_channelType;
    std::vector<uint16_t> m_channelDataSize;
    std::vector<double> m_channelScale;
    std::vector<double> m_channelOffset;

    //Size of a data record
    uint64_t m_dataRecordSize = 0;

    //The current data record's data
    uint8_t* m_dataRecordBuffer = nullptr;

    //pointers to each channel's information in the current data record
    uint8_t** m_channelDataInDataRecord = nullptr;

    //Output Stream matrix
    double* m_matrixBuffer      = nullptr;
    uint64_t m_matrixBufferSize = 0;
    bool m_matricesSent         = false;

    //Total number of samples sent up to now (used to compute start/end time)
    uint32_t m_nSentSample = 0;

    //indexes of current data record, channel, and sample
    uint64_t m_currentDataRecord         = 0;
    uint32_t m_currentSampleInDataRecord = 0;

    //Events variables
    uint8_t m_eventTableMode         = 0;       // mode of the event table
    uint32_t m_nEvents               = 0;       // number of events in the event table
    uint32_t* m_eventsPositionBuffer = nullptr; // pointer on the array of event's positions
    uint16_t* m_eventsTypeBuffer     = nullptr; // pointer on the array of event's types

    std::vector<GDF::CGDFEvent> m_events;  //current stimulation block

    uint32_t m_currentEvent = 0;        // current event in event table
    bool m_eventsSent       = false;    // true if all the events have been sent
    bool m_appendEOF        = true;     // true if the file does contains a recognized EOF marker, then we add our own

    uint64_t m_stimulationPerBuffer = 32;    //user defined

    //helper structures
    CExperimentInfoHeader* m_xpInfoHeader = nullptr;
    bool m_xpInfoSent                     = false;

    CSignalDescription m_signalDesc;
    bool m_signalDescSent = false;

    uint64_t m_clockFrequency = 0;

    bool readFileHeader();
    void writeExperimentInfo() const;
    void writeSignalInformation();
    void writeEvents();

    template <class T>
    double gdfTypeToDouble(T val, const uint32_t channel) {
        return double(val) * m_channelScale[channel] + m_channelOffset[channel];
    }

    template <class T>
    void gdfTypeBufferToDoubleBuffer(double* out, T* in, const uint64_t size, const uint32_t channel) {
        for (uint64_t i = 0; i < size; ++i) {
            out[i] = gdfTypeToDouble<T>(in[i], channel);
        }
    }

    void gdfBufferToDoubleBuffer(double* out, void* in, const uint64_t size, const uint32_t channel);
};

// template<> double CGDFFileReader::gdfTypeToDouble<float>(float val, uint32_t channel);
// template<> double CGDFFileReader::gdfTypeToDouble<double>(double val, uint32_t channel);

/**
* Description of the GDF Reader plugin
*/
class CGDFFileReaderDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "GDF file reader"; }
    CString getAuthorName() const override { return "Bruno Renier, Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "GDF file reader"; }
    CString getDetailedDescription() const override { return "Reads .GDF format files"; }
    CString getCategory() const override { return "File reading and writing/GDF"; }
    CString getVersion() const override { return "0.9.1"; }
    CString getStockItemName() const override { return "gtk-open"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_GDFFileReader; }
    IPluginObject* create() override { return new CGDFFileReader(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box outputs
        prototype.addOutput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addOutput("EEG stream", OV_TypeId_Signal);
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);

        // Adds settings
        prototype.addSetting("Filename", OV_TypeId_Filename, "");
        prototype.addSetting("Samples per buffer", OV_TypeId_Integer, "32");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_GDFFileReaderDesc)
};

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
