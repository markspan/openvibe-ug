#include "ovpCAlgorithmBrainampFileReader.h"

#include <system/ovCMemory.h>
#include <sstream>
#include <cstdlib>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

// 
bool CAlgorithmBrainampFileReader::initialize()
{
    ip_filename.initialize(getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_Filename));
    ip_epochDuration.initialize(getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_EpochDuration));
    ip_seekTime.initialize(getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_SeekTime));
    ip_convertStimuli.initialize(getInputParameter(OVP_Algorithm_BrainampFileReader_InputParameterId_ConvertStimuli));

    op_startTime.initialize(getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentStartTime));
    op_endTime.initialize(getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_CurrentEndTime));
    op_sampling.initialize(getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Sampling));
    op_signalMatrix.initialize(getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_SignalMatrix));
    op_stimulations.initialize(getOutputParameter(OVP_Algorithm_BrainampFileReader_OutputParameterId_Stimulations));

    // Default value
    ip_convertStimuli = true;

    m_buffer = nullptr;

    return true;
}

bool CAlgorithmBrainampFileReader::uninitialize()
{
    if (m_dataFile.is_open()) { m_dataFile.close(); }

    op_stimulations.uninitialize();
    op_signalMatrix.uninitialize();
    op_sampling.uninitialize();
    op_endTime.uninitialize();
    op_startTime.uninitialize();

    ip_convertStimuli.uninitialize();
    ip_seekTime.uninitialize();
    ip_epochDuration.uninitialize();
    ip_filename.uninitialize();

    delete [] m_buffer;
    m_buffer = nullptr;

    return true;
}

bool CAlgorithmBrainampFileReader::process()
{
    if (this->isInputTriggerActive(OVP_Algorithm_BrainampFileReader_InputTriggerId_Open))
    {
        std::string markerFilename;
        std::string dataFilename;

        m_binaryFormat         = EBinaryFormat::Integer16;
        m_nChannel             = 0;
        m_startSampleIdx       = 0;
        m_endSampleIdx         = 0;
        m_sampleCountPerBuffer = 0;
        m_channelScales.clear();
        delete [] m_buffer;
        m_buffer     = nullptr;
        m_endianness = EEndianness::LittleEndian;

        m_headerFile.open(ip_filename->toASCIIString(), std::ios::binary);
        if (!m_headerFile.good())
        {
            getLogManager() << Kernel::LogLevel_Error << "Could not open file [" << *ip_filename << "]\n";
            return false;
        }
        getLogManager() << Kernel::LogLevel_Trace << "Opening " << *ip_filename << " succeeded\n";

        op_signalMatrix->setDimensionCount(2);

        size_t channelIdx = 0;
        EStatus status    = EStatus::Nothing;
        do
        {
            std::string what;
            std::getline(m_headerFile, what, '\n');
            getLogManager() << Kernel::LogLevel_Debug << what << "\n";

            // optionally removes ending carriage return for windows / linux compatibility
            if (what.length() != 0) { if (what[what.length() - 1] == '\r') { what.erase(what.length() - 1, 1); } }

            if (what.length() != 0)
            {
                std::string::size_type equalPos;
                if (what[0] == ';') // comments
                { }
                else if (what.length() > 2 && what[0] == '[') // section start
                {
                    std::string name;
                    name.assign(what, 1, what.length() - 2);
                    if (name == "Common Infos")
                    {
                        getLogManager() << Kernel::LogLevel_Trace << "Found section " << what << "\n";
                        status = EStatus::CommonInfos;
                    }
                    else if (name == "Binary Infos")
                    {
                        getLogManager() << Kernel::LogLevel_Trace << "Found section " << what << "\n";
                        status = EStatus::BinrayInfos;
                    }
                    else if (name == "Channel Infos")
                    {
                        getLogManager() << Kernel::LogLevel_Trace << "Found section " << what << "\n";
                        status = EStatus::ChannelInfos;
                    }
                    else if (name == "Comment")
                    {
                        getLogManager() << Kernel::LogLevel_Trace << "Found section " << what << "\n";
                        status = EStatus::Comment;
                    }
                    else
                    {
                        getLogManager() << Kernel::LogLevel_Warning << "{" << what <<
                                "} looked like a new section in the header file but is not know of this parser. Therefore anything after this line will be skipped until a new section is found\n";
                        status = EStatus::Nothing;
                    }
                }
                else if ((equalPos = what.find('=')) != std::string::npos && status != EStatus::Comment) // Option value
                {
                    std::string name;
                    std::string value;
                    name.assign(what, 0, equalPos);
                    value.assign(what, equalPos + 1, what.length() - equalPos - 1);

                    getLogManager() << Kernel::LogLevel_Trace << "| Found option " << name << " with value " << CString(
                        value.c_str()) << "\n";

                    switch (status)
                    {
                        case EStatus::CommonInfos:
                            if (name == "DataFormat")
                            {
                                if (value != "BINARY") { getLogManager() << Kernel::LogLevel_ImportantWarning << "Only binary data is supported\n"; }
                            }
                            else if (name == "DataOrientation")
                            {
                                if (value != "MULTIPLEXED") { getLogManager() << Kernel::LogLevel_ImportantWarning << "Only multiplexed data is supported\n"; }
                            }
                            else if (name == "DataType")
                            {
                                if (value != "TIMEDOMAIN") { getLogManager() << Kernel::LogLevel_ImportantWarning << "Only time domain data is supported\n"; }
                            }
                            else if (name == "Codepage")
                            {
                                if (value != "ANSI")
                                {
                                    getLogManager() << Kernel::LogLevel_Warning << "Header specifies code page as " << value 
                                            << " but it will be forced back to " << CString("ANSI") << "\n";
                                }
                            }
                            else if (name == "DataFile") { dataFilename = value; }
                            else if (name == "MarkerFile") { markerFilename = value; }
                            else if (name == "NumberOfChannels")
                            {
                                m_nChannel = atoi(value.c_str());
                                m_channelScales.clear();
                                m_channelScales.resize(m_nChannel, 1);
                                op_signalMatrix->setDimensionSize(0, m_nChannel);
                            }
                            else if (name == "SamplingInterval")
                            {
                                double samplingInterval = atof(value.c_str());

                                op_sampling = uint64_t(0.5 + 1000000.0 / samplingInterval); // +0.5 for rounding

                                m_sampleCountPerBuffer = int64_t(ip_epochDuration * op_sampling); // $$$ Casted in (int64_t) because of Ubuntu 7.10 crash !
                                op_signalMatrix->setDimensionSize(1, uint32_t(m_sampleCountPerBuffer));

                                // TODO warn if approximated sampling rate
                                getLogManager() << Kernel::LogLevel_Trace << "| -> Calculated sampling frequency " << op_sampling << "Hz\n";
                            }
                            else { getLogManager() << Kernel::LogLevel_Warning << "Skipped option " << name << " with value " << value << "\n"; }
                            break;

                        case EStatus::BinrayInfos:
                            if (name == "BinaryFormat")
                            {
                                if (value == "INT_16") { m_binaryFormat = EBinaryFormat::Integer16; }
                                else if (value == "UINT_16") { m_binaryFormat = EBinaryFormat::UnsignedInteger16; }
                                else if (value == "FLOAT_32" || value == "IEEE_FLOAT_32") { m_binaryFormat = EBinaryFormat::Float32; }
                                else
                                {
                                    m_binaryFormat = EBinaryFormat::Integer16;
                                    getLogManager() << Kernel::LogLevel_ImportantWarning << "Unsupported binary format option value \"" 
                                            << value << "\"... Switched back to 16 bits integer (default)\n";
                                }
                            }
                            else if (name == "UseBigEndianOrder")
                            {
                                if (value == "YES") { m_endianness = EEndianness::BigEndian; }
                                else if (value == "NO") { m_endianness = EEndianness::LittleEndian; }
                                else
                                {
                                    m_endianness = EEndianness::LittleEndian;
                                    getLogManager() << Kernel::LogLevel_ImportantWarning << "Unsupported use big endian order option value \"" 
                                            << value << "\"... Switched back to little endian (default)\n";
                                }
                            }
                            else { getLogManager() << Kernel::LogLevel_Warning << "Skipped option " << name << " with value " << value << "\n"; }
                            break;

                        case EStatus::ChannelInfos:
                        {
                            std::stringstream ss(value);
                            std::string channelName, referenceChannelName, resolutionInUnit, unitName;

                            std::getline(ss, channelName, ',');
                            std::getline(ss, referenceChannelName, ',');
                            std::getline(ss, resolutionInUnit, ',');
                            std::getline(ss, unitName, ',');

                            op_signalMatrix->setDimensionLabel(0, channelIdx, channelName.c_str());
                            m_channelScales[channelIdx] = atof(resolutionInUnit.c_str());
                            channelIdx++;
                        }
                        break;
                        default: break;
                    }
                }
            }
        } while (m_headerFile.good());

        // Changing file location
        std::string fullPath = ip_filename->toASCIIString();
        std::string path;
        std::string::size_type slashPos = fullPath.rfind('/');
        if (slashPos != std::string::npos)
        {
            path.assign(fullPath, 0, slashPos + 1);
            markerFilename = path + markerFilename;
            dataFilename   = path + dataFilename;
        }

        // Opens data file
        m_dataFile.open(dataFilename.c_str(), std::ios::binary);
        if (!m_dataFile.good()) { getLogManager() << Kernel::LogLevel_Error << "Could not open file [" << dataFilename << "]\n"; }
        else { getLogManager() << Kernel::LogLevel_Trace << "Opening " << dataFilename << " succeeded\n"; }

        // Opens marker file
        m_markerFile.open(markerFilename.c_str(), std::ios::binary);
        if (!m_markerFile.good()) { getLogManager() << Kernel::LogLevel_Error << "Could not open file [" << markerFilename << "]\n"; }
        else
        {
            getLogManager() << Kernel::LogLevel_Trace << "Opening " << markerFilename << " succeeded\n";

            status = EStatus::Nothing;
            do
            {
                std::string what;
                std::getline(m_markerFile, what, '\n');
                getLogManager() << Kernel::LogLevel_Debug << what << "\n";

                // optionally removes ending carriage return for windows / linux compatibility
                if (what.length() != 0) { if (what[what.length() - 1] == '\r') { what.erase(what.length() - 1, 1); } }

                if (what.length() != 0)
                {
                    std::string::size_type equalPos;
                    if (what[0] == ';') // comments
                    { }
                    else if (what.length() > 2 && what[0] == '[') // section start
                    {
                        std::string name;
                        name.assign(what, 1, what.length() - 2);
                        if (name == "Common Infos")
                        {
                            getLogManager() << Kernel::LogLevel_Trace << "Found section " << what << "\n";
                            status = EStatus::CommonInfos;
                        }
                        else if (name == "Marker Infos")
                        {
                            getLogManager() << Kernel::LogLevel_Trace << "Found section " << what << "\n";
                            status = EStatus::MarkerInfos;
                        }
                        else
                        {
                            getLogManager() << Kernel::LogLevel_Warning << "{" << what <<
                                    "} looked like a new section in the marker file but is not know of this parser. Therefore anything after this line will be skipped until a new section is found\n";
                            status = EStatus::Nothing;
                        }
                    }
                    else if ((equalPos = what.find('=')) != std::string::npos && status != EStatus::Comment) // Option value
                    {
                        std::string name, value;
                        name.assign(what, 0, equalPos);
                        value.assign(what, equalPos + 1, what.length() - equalPos - 1);

                        getLogManager() << Kernel::LogLevel_Trace << "| Found option " << name << " with value " << value << "\n";

                        switch (status)
                        {
                            case EStatus::CommonInfos:
                                break;

                            case EStatus::MarkerInfos:
                            {
                                std::stringstream ss(value);
                                std::string type, desc, pos, duration, idx, date;

                                std::getline(ss, type, ',');
                                std::getline(ss, desc, ',');
                                std::getline(ss, pos, ',');
                                std::getline(ss, duration, ',');
                                std::getline(ss, idx, ',');
                                std::getline(ss, date, ',');

                                getLogManager() << Kernel::LogLevel_Trace << "| -> Found marker " << type << "," << desc << "," << pos << "," << duration << ","
                                        << idx << "," << date << "\n";

                                if (type == "Stimulus" && desc.length() > 0)
                                {
                                    if (idx != "0")
                                    {
                                        getLogManager() << Kernel::LogLevel_Warning << "Marker [" << type << ":" << desc <<
                                                "] is not marked on channel 0 and OpenViBE only supports global scope stimulations. Therefore this marker will be considered as global\n";
                                    }

                                    uint64_t id;
                                    if (desc[0] == 'S') { id = atoi(desc.substr(1, std::string::npos).c_str()); }
                                    else { id = atoi(desc.c_str()); }

                                    if (ip_convertStimuli)
                                    {
                                        getLogManager() << Kernel::LogLevel_Trace << "Pre-conversion stimulation is " << desc <<
                                                " at sample index [" << pos << "]\n";
                                        id = OVTK_StimulationId_Label(id);
                                    }

                                    stimulation_t stim;
                                    stim.id       = id;
                                    stim.startIdx = atoi(pos.c_str());
                                    stim.duration = atoi(duration.c_str());
                                    stim.name     = desc;
                                    m_stimulations.push_back(stim);

                                    getLogManager() << Kernel::LogLevel_Trace << "Found stimulation " << stim.id << " at sample index [" << stim.startIdx << ":"
                                            << stim.duration << "]\n";
                                }
                                else
                                {
                                    getLogManager() << Kernel::LogLevel_Warning << "Marker [" << type << ":" << desc <<
                                            "] is not supported. Therefore it will be ignored\n";
                                }
                            }
                            break;
                            default: break;
                        }
                    }
                }
            } while (m_markerFile.good());
        }
    }

    if (this->isInputTriggerActive(OVP_Algorithm_BrainampFileReader_InputTriggerId_Seek))
    {
        getLogManager() << Kernel::LogLevel_ImportantWarning << "This has not been implemented yet\n";
    }

    if (this->isInputTriggerActive(OVP_Algorithm_BrainampFileReader_InputTriggerId_Next))
    {
        double* buffer = op_signalMatrix->getBuffer();

#define DO_IT_WITH_TYPE(T) \
        { \
            if (!m_buffer) { m_buffer = new uint8_t[op_signalMatrix->getBufferElementCount() * sizeof(T)]; } \
                uint8_t* fileBuffer = m_buffer; \
                T tValue; \
                m_dataFile.read((char*)fileBuffer, op_signalMatrix->getBufferElementCount() * sizeof(T)); \
                if (m_dataFile.eof()) { memset(fileBuffer, 0, op_signalMatrix->getBufferElementCount() * sizeof(T)); } \
                    bool (*fiileToHost)(const uint8_t*, T*); \
                    if (m_endianness == EEndianness::LittleEndian) { fiileToHost = System::Memory::littleEndianToHost; } \
                    else { fiileToHost = System::Memory::bigEndianToHost; } \
                    for (uint32_t j = 0; j < m_sampleCountPerBuffer; ++j) \
                    { \
                        for (uint32_t i = 0; i < m_nChannel; i++, fileBuffer += sizeof(T)) \
                        { \
                            (*fiileToHost)(fileBuffer, &tValue); \
                            buffer[i * m_sampleCountPerBuffer + j] = m_channelScales[i] * tValue; \
                        } \
                    } \
        }

        if (m_binaryFormat == EBinaryFormat::Integer16) { DO_IT_WITH_TYPE(int16_t); }
        if (m_binaryFormat == EBinaryFormat::UnsignedInteger16) { DO_IT_WITH_TYPE(uint16_t); }
        if (m_binaryFormat == EBinaryFormat::Float32) { DO_IT_WITH_TYPE(float); }

        m_startSampleIdx = m_endSampleIdx;
        m_endSampleIdx += m_sampleCountPerBuffer;

        op_startTime = CTime(op_sampling, m_startSampleIdx).time();
        op_endTime   = CTime(op_sampling, m_endSampleIdx).time();

        // find stimulations in this range
        uint64_t nStim = 0;
        for (const auto& s : m_stimulations) { if (m_startSampleIdx <= s.startIdx && s.startIdx < m_endSampleIdx) { nStim++; } }

        op_stimulations->resize(nStim);
        nStim = 0;
        for (const auto& s : m_stimulations)
        {
            if (m_startSampleIdx <= s.startIdx && s.startIdx < m_endSampleIdx)
            {
                uint64_t date     = CTime(op_sampling, s.startIdx).time();
                uint64_t duration = CTime(op_sampling, s.duration).time();

                op_stimulations->setId(nStim, s.id);
                op_stimulations->setDate(nStim, date);
                op_stimulations->setDuration(nStim, duration);
                nStim++;
            }
        }

        this->activateOutputTrigger(OVP_Algorithm_BrainampFileReader_OutputTriggerId_DataProduced, true);
    }

    if (this->isInputTriggerActive(OVP_Algorithm_BrainampFileReader_InputTriggerId_Close))
    {
        delete [] m_buffer;
        m_buffer = nullptr;

        m_markerFile.close();
        m_dataFile.close();
        m_headerFile.close();
    }

    return true;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
