#include "ovpCGDFFileReader.h"

// @fixme memory leaks on errors

#include <iostream>
#include <cstdlib>
#include <cstring>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

// template<> double FileIO::CGDFFileReader::GDFTypeToFloat64<float>(float val, uint32_t channel) { std::cout << "specialized 1\n"; return val; }
// template<> double FileIO::CGDFFileReader::GDFTypeToFloat64<double>(double val, uint32_t channel) { std::cout << "specialized 2\n"; return val; }

// #define DEBUG_FILE_POSITIONS 1

//Plugin Methods
bool CGDFFileReader::initialize()
{
    m_filename           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_samplesPerBuffer   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    if (m_samplesPerBuffer == 0) {
        this->getLogManager() << Kernel::LogLevel_Error <<
                "SamplesPerBuffer is 0, this will not work\n";
        return false;
    }

    m_xpInfoEncoder = new Toolkit::TExperimentInfoEncoder<CGDFFileReader>;
    m_xpInfoEncoder->initialize(*this, GDFReader_ExperimentInfoOutput);
    m_signalEncoder = new Toolkit::TSignalEncoder<CGDFFileReader>;
    m_signalEncoder->initialize(*this, GDFReader_SignalOutput);
    m_stimulationEncoder = new Toolkit::TStimulationEncoder<CGDFFileReader>;
    m_stimulationEncoder->initialize(*this, GDFReader_StimulationOutput);


    //allocate the structure used to store the experiment information
    m_xpInfoHeader = new CExperimentInfoHeader;

    //opens the gdf file
    if (m_filename != CString("")) {
        m_file.open(m_filename.toASCIIString(), std::ios::binary);
        if (!m_file.good()) {
            this->getLogManager() << Kernel::LogLevel_Error << "Could not open file [" << m_filename << "]\n";
            return false;
        }
    }

    m_file.seekg(0, std::ios::end);
    m_fileSize = size_t(m_file.tellg());
    m_file.seekg(0, std::ios::beg);

    m_header3Length = 0;

    // const char *filename = m_sFileName.toASCIIString();
    // this->getLogManager() << Kernel::LogLevel_Trace << "Opening [" << filename << "]\n";

    //reads the gdf headers and sends the corresponding buffers
    const bool res = readFileHeader();

#ifdef DEBUG_FILE_POSITIONS
                this->getLogManager() << Kernel::LogLevel_Info << "After all headers, file is at " << m_file.tellg() << "\n";
#endif

    return res;
}

bool CGDFFileReader::uninitialize()
{
    if (m_signalEncoder) {
        m_signalEncoder->uninitialize();
        delete m_signalEncoder;
    }
    if (m_xpInfoEncoder) {
        m_xpInfoEncoder->uninitialize();
        delete m_xpInfoEncoder;
    }
    if (m_stimulationEncoder) {
        m_stimulationEncoder->uninitialize();
        delete m_stimulationEncoder;
    }


    //desallocate all of the remaining buffers
    delete[] m_dataRecordBuffer;        //can be done before??
    delete[] m_channelDataInDataRecord;
    delete[] m_matrixBuffer;            //can be done before?
    delete[] m_eventsPositionBuffer;
    delete[] m_eventsTypeBuffer;
    //Close the GDF file
    if (m_file) { m_file.close(); }

    return true;
}

bool CGDFFileReader::processClock(Kernel::CMessageClock& msg)
{
    if (m_signalDesc.m_Sampling == 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Sampling rate is 0 - not supported (in processClock).\n";
        return false;
    }

    const uint64_t time = CTime(m_signalDesc.m_Sampling, m_nSentSample + m_signalDesc.m_NSample).time();
    if (msg.getTime() > time) { getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }
    return true;
}

void CGDFFileReader::gdfBufferToDoubleBuffer(double* out, void* in, const uint64_t size, const uint32_t channel)
{
    switch (m_channelType[channel]) {
        case GDF::ChannelType_int8_t:
            gdfTypeBufferToDoubleBuffer<int8_t>(out, reinterpret_cast<int8_t*>(in), size, channel);
            break;

        case GDF::ChannelType_uint8_t:
            gdfTypeBufferToDoubleBuffer<uint8_t>(out, reinterpret_cast<uint8_t*>(in), size, channel);
            break;

        case GDF::ChannelType_int16_t:
            gdfTypeBufferToDoubleBuffer<int16_t>(out, reinterpret_cast<int16_t*>(in), size, channel);
            break;

        case GDF::ChannelType_uint16_t:
            gdfTypeBufferToDoubleBuffer<uint16_t>(out, reinterpret_cast<uint16_t*>(in), size, channel);
            break;

        case GDF::ChannelType_int32_t:
            gdfTypeBufferToDoubleBuffer<int>(out, reinterpret_cast<int*>(in), size, channel);
            break;

        case GDF::ChannelType_uint32_t:
            gdfTypeBufferToDoubleBuffer<uint32_t>(out, reinterpret_cast<uint32_t*>(in), size, channel);
            break;

        case GDF::ChannelType_int64_t:
            gdfTypeBufferToDoubleBuffer<int64_t>(out, reinterpret_cast<int64_t*>(in), size, channel);
            break;

        case GDF::ChannelType_uint64_t:
            gdfTypeBufferToDoubleBuffer<uint64_t>(out, reinterpret_cast<uint64_t*>(in), size, channel);
            break;

        case GDF::ChannelType_float:
            gdfTypeBufferToDoubleBuffer<float>(out, reinterpret_cast<float*>(in), size, channel);
            break;

        case GDF::ChannelType_double:
            gdfTypeBufferToDoubleBuffer<double>(out, reinterpret_cast<double*>(in), size, channel);
            break;

        case GDF::ChannelType_float128:
        {
            //Not handled
            this->getLogManager() << Kernel::LogLevel_Warning << "This data type is currently not handled : float128.\n";
            m_errorOccurred = true;
        }
        break;

        case GDF::ChannelType_int24:
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(in);

            for (uint64_t i = 0; i < (size * 3); i += 3) { out[i] = gdfTypeToDouble(p[i] + (p[i + 1] << 8) + (p[i + 2] << 16), channel); }
        }
        break;

        case GDF::ChannelType_uint24:
        {
            int8_t* p = reinterpret_cast<int8_t*>(in);

            for (uint64_t i = 0; i < (size * 3); i += 3) { out[i] = gdfTypeToDouble(p[i] + (p[i + 1] << 8) + (p[i + 2] << 16), channel); }
        }
        break;

        default:
            //not handled
            this->getLogManager() << Kernel::LogLevel_Warning << "Invalid GDF data type!\n";
            m_errorOccurred = true;
            break;
    }
}

bool CGDFFileReader::readFileHeader()
{
    Kernel::IBoxIO& boxIO = this->getDynamicBoxContext();

    if (!m_xpInfoSent) {
        this->getLogManager() << Kernel::LogLevel_Trace << "Reading experiment information\n";

        //First reads the file type
        char type[3];
        char version[6];    // field has size 5, we add +1 for terminating NULL
        m_file.read(type, 3);

        //if not a gdf file
        if (strncmp(type, "GDF", 3) != 0) {
            //Handle error
            this->getLogManager() << Kernel::LogLevel_Warning << "This is not a valid GDF File!\n";
            m_errorOccurred = true;
            return false;
        }

        m_file.read(version, 5);
        version[5]    = 0;  // The version is not NULL-terminated in the file, we terminate with NULL manually for atof.
        m_fileVersion = float(atof(&version[1]));

        this->getLogManager() << Kernel::LogLevel_Debug << "File version parsed as " << m_fileVersion << "\n";

        if (m_file.bad()) {
            //Handle error
            this->getLogManager() << Kernel::LogLevel_Warning << "Error while reading file.\n";
            m_errorOccurred = true;
            return false;
        }

        if (m_fileVersion < 3) {
            GDF::CFixedGDFHeader* header;

            if (m_fileVersion > 2.12F) { header = new GDF::CFixedGDF251Header; }
            else if (m_fileVersion > 1.90F) { header = new GDF::CFixedGDF2Header; }
            else { header = new GDF::CFixedGDF1Header; }

            if (!header->read(m_file)) {
                this->getLogManager() << Kernel::LogLevel_Error <<
                        "Failure to parse fixed header\n";
                m_errorOccurred = true;
                delete header;
                return false;
            }

            // kludge: header should be 256 bytes long, make sure the file position is there now.
            if (m_fileVersion > 1.90F) { m_file.seekg(256, std::ios_base::beg); }

            m_xpInfoHeader->m_ExperimentID   = header->getExperimentID();
            m_xpInfoHeader->m_ExperimentDate = header->getExperimentDate();
            m_xpInfoHeader->m_SubjectID      = header->getSubjectID();
            m_xpInfoHeader->m_SubjectName    = header->getSubjectName();
            m_xpInfoHeader->m_SubjectAge     = header->getSubjectAge();
            m_xpInfoHeader->m_SubjectSex     = header->getSubjectSex();

            m_xpInfoHeader->m_LaboratoryID   = header->getLaboratoryID();
            m_xpInfoHeader->m_LaboratoryName = header->getLaboratoryName();
            m_xpInfoHeader->m_TechnicianID   = header->getTechnicianID();
            m_xpInfoHeader->m_TechnicianName = header->getTechnicianName();

            //Experiment header ready to send now
            m_xpInfoHeader->m_ReadyToSend = true;

            m_durationDataRecord = header->getDataRecordDuration();
            m_nDataRecords       = header->getNDataRecords();

            //this information is related to the signal
            m_nChannels             = uint16_t(header->getChannelCount());
            m_signalDesc.m_NChannel = m_nChannels;

            //Send the header
            writeExperimentInfo();

            boxIO.markOutputAsReadyToSend(GDFReader_ExperimentInfoOutput, 0, 0);
            m_xpInfoSent = true;


            //not needed anymore
            delete header;
            delete m_xpInfoHeader;
            m_xpInfoHeader = nullptr;
        }
        else {
            //Not a known GDF File version
            this->getLogManager() << Kernel::LogLevel_Error << "GDF file version " << m_fileVersion << " is not supported.\n";
            //Error handling
            m_errorOccurred = true;
            return false;
        }
    }//END of ExperimentHeader

    if (!m_signalDescSent) {
        this->getLogManager() << Kernel::LogLevel_Trace << "Reading signal description\n";

#ifdef DEBUG_FILE_POSITIONS
                    this->getLogManager() << Kernel::LogLevel_Info << "Before variable header, file is at " << m_file.tellg() << "\n";
#endif

        //reads the whole variable header
        char* headerBuffer = new char[m_nChannels * 256];
        m_file.read(headerBuffer, m_nChannels * 256);

#ifdef DEBUG_FILE_POSITIONS
                    this->getLogManager() << Kernel::LogLevel_Info << "After variable header, file is at " << m_file.tellg() << "\n";
#endif

        if (m_file.bad()) {
            //Handle error
            this->getLogManager() << Kernel::LogLevel_Error << "Read error.\n";
            m_errorOccurred = true;
            return false;
        }

        m_signalDesc.m_ChannelNames.resize(m_nChannels);

        //channel's signal gain/translation
        m_channelScale.resize(m_nChannels);
        m_channelOffset.resize(m_nChannels);

        double* physicalMin = reinterpret_cast<double*>(headerBuffer + (104 * m_nChannels));
        double* physicalMax = reinterpret_cast<double*>(headerBuffer + (112 * m_nChannels));
        int64_t* digitalMin = reinterpret_cast<int64_t*>(headerBuffer + (120 * m_nChannels));   // v1?
        int64_t* digitalMax = reinterpret_cast<int64_t*>(headerBuffer + (128 * m_nChannels));
        double* digitalMinD = reinterpret_cast<double*>(headerBuffer + (120 * m_nChannels));    // v2+
        double* digitalMaxD = reinterpret_cast<double*>(headerBuffer + (128 * m_nChannels));

        for (uint16_t i = 0; i < m_nChannels; ++i) {
            if (m_fileVersion > 1.90) {
                m_channelScale[i] = (physicalMax[i] - physicalMin[i]) / (digitalMaxD[i] - digitalMinD[i]);
                m_channelOffset[i] = physicalMin[i] - (static_cast<double>(digitalMinD[i]) * m_channelScale[i]);
            } else {
                m_channelScale[i] = (physicalMax[i] - physicalMin[i]) / (digitalMax[i] - digitalMin[i]);
                m_channelOffset[i] = physicalMin[i] - (static_cast<double>(digitalMin[i]) * m_channelScale[i]);
            }

            this->getLogManager() << Kernel::LogLevel_Debug << "Channel " << i << " physMin " << physicalMin[i] << " physMax " << physicalMax[i]
                    << " digMin " << digitalMin[i] << " digMax " << digitalMax[i]
                    << " digMinF " << digitalMinD[i] << " digMaxF " << digitalMaxD[i]
                    << " scale " << m_channelScale[i] << " offset " << m_channelOffset[i] << "\n";
        }

        //Check if all the channels have the same sampling rate
        uint32_t* nSamplesPerRecordArray = reinterpret_cast<uint32_t*>(headerBuffer + (216 * m_nChannels));

        m_nSamplesPerRecord = nSamplesPerRecordArray[0];

        for (uint16_t i = 1; i < m_nChannels; ++i) {
            //If all the channels don't have the same sampling rate
            if (m_nSamplesPerRecord != nSamplesPerRecordArray[i]) {
                if (m_fileVersion > 1.90F) {
                    this->getLogManager() << Kernel::LogLevel_Error <<
                            "Interpreted GDF file to have channels with varying sampling rates, which is not supported.\n";
                    this->getLogManager() << Kernel::LogLevel_Error << "This can be a misinterpretation of the newer GDF subformats. File claims to follow GDF "
                            << m_fileVersion << ".\n";
                }
                else { this->getLogManager() << Kernel::LogLevel_Error << "Can't handle GDF files with channels having different sampling rates!\n"; }
                m_errorOccurred = true;
                return false;
            }
        }

        //type of the channels' data
        m_channelType.resize(m_nChannels);
        memcpy(m_channelType.data(), headerBuffer + (220 * m_nChannels), m_nChannels * 4);

        m_channelDataSize.resize(m_nChannels);

        this->getLogManager() << Kernel::LogLevel_Debug << "Found " << m_nChannels << " channels...\n";
        for (uint16_t i = 0; i < m_nChannels; ++i) {
            //Find the data size for each channel
            //TODO use enum to specify each type's name
            m_channelDataSize[i] = GDF::GDFDataSize(m_channelType[i]);

            //Here, we can compute the size of a data record, based on the type of each channel
            m_dataRecordSize += m_nSamplesPerRecord * m_channelDataSize[i];

            //reads the channels names
            m_signalDesc.m_ChannelNames[i].assign(headerBuffer + (16 * i), 16);

            this->getLogManager() << Kernel::LogLevel_Debug << " * Channel " << uint32_t(i + 1) << " : " << CString(
                m_signalDesc.m_ChannelNames[i].c_str()) << "\n";
        }

        //This parameter is defined by the user of the plugin
        m_signalDesc.m_NSample = uint32_t(m_samplesPerBuffer);

        //needs to be computed based on the duration of a data record and the number of samples in one of those data records
        m_signalDesc.m_Sampling = uint32_t(0.5 + (m_nSamplesPerRecord / m_durationDataRecord));

        if (m_signalDesc.m_Sampling == 0) {
            this->getLogManager() << Kernel::LogLevel_Error << "Sampling rate is 0 - not supported (in readFileHeader) \n";
            m_errorOccurred = true;
            return false;
        }

        this->getLogManager() << Kernel::LogLevel_Debug << "Samples in file : " << m_nDataRecords * m_nSamplesPerRecord << " samples\n";
        this->getLogManager() << Kernel::LogLevel_Debug << "Sample count per buffer : " << m_samplesPerBuffer << "\n";
        this->getLogManager() << Kernel::LogLevel_Debug << "Sampling rate : " << m_signalDesc.m_Sampling << "\n";

        //computes clock frequency
        if (m_samplesPerBuffer <= m_signalDesc.m_Sampling) {
            if (m_signalDesc.m_Sampling % m_samplesPerBuffer != 0) {
                this->getLogManager() << Kernel::LogLevel_Warning << "The sampling rate isn't a multiple of the buffer size\n";
                this->getLogManager() << Kernel::LogLevel_Warning << "Please consider adjusting the GDFReader settings to correct this!\n";
                this->getLogManager() << Kernel::LogLevel_Warning << "Sampling rate was " << m_signalDesc.m_Sampling << "\n";
                this->getLogManager() << Kernel::LogLevel_Warning << "Buffer size was " << m_samplesPerBuffer << "\n";
            }

            // Intentional parameter swap to get the frequency
            m_clockFrequency = CTime(m_samplesPerBuffer, m_signalDesc.m_Sampling).time();
        }

        // We may need to skip header3 with its tags and take its size into account
        if (m_fileVersion >= 2.10F) {
#ifdef DEBUG_FILE_POSITIONS
                        this->getLogManager() << Kernel::LogLevel_Info << "Before header3, file is at " << m_file.tellg() << "\n";
#endif

            m_header3Length = 0;

            while (true) {
                char buffer[4];
                m_file.read((char*)&buffer, 4);     // Curious Cast
                if (m_file.bad() || m_file.eof()) { break; }
                m_header3Length += 4;

                uint32_t tag    = buffer[0];
                uint32_t length = (uint32_t(buffer[1]) << 0) + (uint32_t(buffer[2]) << 8) + (uint32_t(buffer[3]) << 16);    // src is uint24

                this->getLogManager() << Kernel::LogLevel_Info << "Found tag " << tag << " at pos " << int64_t(m_file.tellg() - std::streamoff(4))
                        << " [length " << length << "], skipping content.\n";

                if (tag == 0) { break; }
                m_file.seekg(std::streamoff(length), std::ios_base::cur);
                m_header3Length += length;
            }
            // Skip possible padding
            const uint64_t paddingRequired = (256 - m_header3Length) % 256;
            m_header3Length += paddingRequired;
            m_file.seekg(paddingRequired, std::ios::cur);

            if (m_file.eof() || m_file.tellg() <= 0) {
                this->getLogManager() << Kernel::LogLevel_Error << "File ended by trying to skip the header.\n";
                return false;
            }

#ifdef DEBUG_FILE_POSITIONS
                        this->getLogManager() << Kernel::LogLevel_Info << "After header3, file is at " << m_file.tellg() << ". Header length: " << m_header3Length << ", padding was " << paddingRequired << "\n";
#endif
        }

        //Send the data to the output
        writeSignalInformation();
        boxIO.markOutputAsReadyToSend(GDFReader_SignalOutput, 0, 0);

        delete[] headerBuffer;

        m_signalDescSent = true;
    }//END of SignalHeader

    return true;
}

void CGDFFileReader::writeExperimentInfo() const
{
    // Here we have to declare some variables in the same scope as the encoding call, because otherwise they might be freed before the 
    // encoder gets to process the data. The point of these is just to convert from std::string to CString as needed by the encoder.
    CString date(m_xpInfoHeader->m_ExperimentDate.c_str());
    CString name(m_xpInfoHeader->m_SubjectName.c_str());
    CString labName(m_xpInfoHeader->m_LaboratoryName.c_str());
    CString techName(m_xpInfoHeader->m_TechnicianName.c_str());

    if (m_xpInfoHeader->m_ExperimentID != NO_VALUE_I) { m_xpInfoEncoder->getInputExperimentID() = m_xpInfoHeader->m_ExperimentID; }
    if (m_xpInfoHeader->m_ExperimentDate != NO_VALUE_S) { m_xpInfoEncoder->getInputExperimentDate() = &date; }
    if (m_xpInfoHeader->m_SubjectID != NO_VALUE_I) { m_xpInfoEncoder->getInputSubjectID() = m_xpInfoHeader->m_SubjectID; }
    if (m_xpInfoHeader->m_SubjectName != NO_VALUE_S) { m_xpInfoEncoder->getInputSubjectName() = &name; }
    if (m_xpInfoHeader->m_SubjectAge != NO_VALUE_I) { m_xpInfoEncoder->getInputSubjectAge() = m_xpInfoHeader->m_SubjectAge; }
    if (m_xpInfoHeader->m_SubjectSex != NO_VALUE_I) { m_xpInfoEncoder->getInputSubjectGender() = m_xpInfoHeader->m_SubjectSex; }
    if (m_xpInfoHeader->m_LaboratoryID != NO_VALUE_I) { m_xpInfoEncoder->getInputLaboratoryID() = m_xpInfoHeader->m_LaboratoryID; }
    if (m_xpInfoHeader->m_LaboratoryName != NO_VALUE_S) { m_xpInfoEncoder->getInputLaboratoryName() = &labName; }
    if (m_xpInfoHeader->m_TechnicianID != NO_VALUE_I) { m_xpInfoEncoder->getInputTechnicianID() = m_xpInfoHeader->m_TechnicianID; }
    if (m_xpInfoHeader->m_TechnicianName != NO_VALUE_S) { m_xpInfoEncoder->getInputTechnicianName() = &techName; }

    m_xpInfoEncoder->encodeHeader();
}

void CGDFFileReader::writeSignalInformation()
{
    m_signalEncoder->getInputSamplingRate() = m_signalDesc.m_Sampling;

    CMatrix* iMatrix = m_signalEncoder->getInputMatrix();
    iMatrix->resize(m_signalDesc.m_NChannel, m_signalDesc.m_NSample);

    for (uint32_t i = 0; i < m_signalDesc.m_NChannel; ++i) { iMatrix->setDimensionLabel(0, i, m_signalDesc.m_ChannelNames[i].c_str()); }

    m_signalEncoder->encodeHeader();
}

void CGDFFileReader::writeEvents()
{
    CStimulationSet* stimSet = m_stimulationEncoder->getInputStimulationSet();
    stimSet->clear();

    for (size_t i = 0; i < m_events.size(); ++i) {
        //compute date
        const uint64_t date = CTime(m_signalDesc.m_Sampling, m_events[i].m_Position).time();
        stimSet->push_back(m_events[i].m_Type, date, 0);
    }

    m_stimulationEncoder->encodeBuffer();
}

bool CGDFFileReader::process()
{
    //Don't do anything if an error as occurred while reading the input file
    //for instance, if the file has channels with different sampling rates
    if (m_errorOccurred) {
        this->getLogManager() << Kernel::LogLevel_Error << "Some error occurred, aborting.";
        return false;
    }

    uint64_t start = 0;
    uint64_t end   = 0;

    Kernel::IBoxIO& boxIO = this->getDynamicBoxContext();

    // Process Matrices
    if (m_signalDescSent && !m_matricesSent) {
        //If the matrix buffer is not allocated yet
        //"first time"
        if (!m_matrixBuffer) {
            //output matrix buffer
            m_matrixBufferSize = m_signalDesc.m_NSample * m_signalDesc.m_NChannel;
            m_matrixBuffer     = new double[size_t(m_matrixBufferSize)];

            //We also have to read the first data record
            m_dataRecordBuffer = new uint8_t[size_t(m_dataRecordSize)];
            m_file.read(reinterpret_cast<char*>(m_dataRecordBuffer), std::streamsize(m_dataRecordSize));

            if (m_file.bad()) {
                //Handle error
                this->getLogManager() << Kernel::LogLevel_Error <<
                        "Read error\n";
                m_errorOccurred = true;
                return false;
            }

            //initialize subpointers
            m_channelDataInDataRecord = new uint8_t*[m_nChannels];

            m_channelDataInDataRecord[0] = m_dataRecordBuffer;
            for (int i = 1; i < m_nChannels; ++i) {
                m_channelDataInDataRecord[i] = m_channelDataInDataRecord[i - 1] + m_channelDataSize[i - 1] * m_nSamplesPerRecord;
            }
        }

        uint32_t nSample = 0;
        bool readyToSend = false;

        while (!readyToSend) {
            //there is the same number of samples
            const uint64_t samplesRemainingInDataRecord = m_nSamplesPerRecord - m_currentSampleInDataRecord;

            //If there is enough data in the current data record to read a matrix buffer
            if ((m_samplesPerBuffer - nSample) <= samplesRemainingInDataRecord) {
                for (int i = 0; i < m_nChannels; ++i) {
                    //reads m_samplesPerBuffer samples and converts/writes them in output buffer
                    gdfBufferToDoubleBuffer(m_matrixBuffer + (i * m_samplesPerBuffer) + nSample,
                                            m_channelDataInDataRecord[i] + (m_currentSampleInDataRecord * m_channelDataSize[i]),
                                            m_samplesPerBuffer - nSample, i);
                }

                m_currentSampleInDataRecord += uint32_t(m_samplesPerBuffer - nSample);

                //Prepares for the next matrix
                nSample = 0;

                //We can send the matrix
                readyToSend = true;
            }
            //Not enough data in the current data record. Read the remaining samples, then load a new data record and read the rest
            else {
                //copy what is remaining in the current buffer
                for (int i = 0; i < m_nChannels; ++i) {
                    gdfBufferToDoubleBuffer(m_matrixBuffer + (i * m_samplesPerBuffer) + nSample,
                                            m_channelDataInDataRecord[i] + (m_currentSampleInDataRecord * m_channelDataSize[i]),
                                            samplesRemainingInDataRecord, i);
                }

                //Updates the index in the output matrix
                nSample += uint32_t(samplesRemainingInDataRecord);

                //reads the next data record if there is one
                if (m_currentDataRecord < m_nDataRecords - 1) {
                    //reads a data record
                    m_file.read(reinterpret_cast<char*>(m_dataRecordBuffer), std::streamsize(m_dataRecordSize));

                    if (m_file.bad()) {
                        //Handle error
                        this->getLogManager() << Kernel::LogLevel_Error <<
                                "Read error\n";
                        m_errorOccurred = true;
                        return false;
                    }

                    m_currentSampleInDataRecord = 0;
                    m_currentDataRecord++;
                }
                //if there are no more data records
                else {
                    //we can (for instance) pad the rest of the matrix with 0s
                    for (int i = 0; i < m_nChannels; ++i) {
                        memset(m_matrixBuffer + (((i * m_samplesPerBuffer) + nSample)), 0, size_t(m_samplesPerBuffer - nSample) * sizeof(double));
                    }

                    //We can send the matrix
                    readyToSend = true;
                    //No more data after that
                    m_matricesSent = true;
                }
            }

            //Check if we have finished the current data record
            if (m_currentSampleInDataRecord == m_nSamplesPerRecord) {
                m_currentSampleInDataRecord = 0;
                m_currentDataRecord++;

                //if there are no more data records
                if (m_currentDataRecord >= m_nDataRecords - 1) {
                    //We don't have to read data records anymore
                    m_matricesSent = true;
                }
                else {
                    //reads a data record
                    m_file.read(reinterpret_cast<char*>(m_dataRecordBuffer), std::streamsize(m_dataRecordSize));

                    if (m_file.bad()) {
                        //Handle error
                        this->getLogManager() << Kernel::LogLevel_Error <<
                                "Read error\n";
                        m_errorOccurred = true;
                        return false;
                    }
                }
            }
        }

        m_nSentSample += m_signalDesc.m_NSample;

        // this->getLogManager() << Kernel::LogLevel_Trace << "Sent " << m_nSentSample << " samples\n";

        //A signal matrix is ready to be output

        start = CTime(m_signalDesc.m_Sampling, uint64_t(m_nSentSample - m_signalDesc.m_NSample)).time();
        end   = CTime(m_signalDesc.m_Sampling, uint64_t(m_nSentSample)).time();

        CMatrix* iMatrix = m_signalEncoder->getInputMatrix();
        double* buffer   = iMatrix->getBuffer();
        for (uint32_t i = 0; i < iMatrix->getBufferElementCount(); ++i) { buffer[i] = *(m_matrixBuffer + i); }
        m_signalEncoder->encodeBuffer();
        boxIO.markOutputAsReadyToSend(GDFReader_SignalOutput, start, end);
    }

    //Events
    if (m_signalDescSent && !m_eventsSent) {
        //reads the events table header if it hasn't been done already
        if (!m_eventsPositionBuffer) {
            const std::streamoff backupPos    = m_file.tellg();
            const std::streamoff eventDataPos = std::streamoff((256 * (m_nChannels + 1)) + m_header3Length + (m_nDataRecords * m_dataRecordSize));

            //checks if there are event information
            if (size_t(eventDataPos) + 1 < m_fileSize) {
                m_file.seekg(eventDataPos);

                //reads the event table mode
                m_file >> m_eventTableMode;
            }
            //no event information
            else {
                m_file.seekg(backupPos);

                m_eventsSent = true;
                return true;
            }

            uint32_t eventTableHeaderMain[7];
            uint8_t* eventTableHeader = reinterpret_cast<uint8_t*>(eventTableHeaderMain);
            memset(eventTableHeaderMain, 0, sizeof(eventTableHeaderMain));
            m_file.read(reinterpret_cast<char*>(eventTableHeader), 7);

            if (m_fileVersion > 1.90F) { m_nEvents = *(reinterpret_cast<uint32_t*>(eventTableHeader + 0)); }
            else { m_nEvents = *(reinterpret_cast<uint32_t*>(eventTableHeader + 3)); }

            this->getLogManager() << Kernel::LogLevel_Trace << "The file has " << m_nEvents << " events\n";

            m_eventsPositionBuffer = new uint32_t[m_nEvents * 4];
            m_eventsTypeBuffer     = new uint16_t[m_nEvents * 2];

            //we have to read all the events' position and type
            m_file.read(reinterpret_cast<char*>(m_eventsPositionBuffer), m_nEvents * 4);
            m_file.read(reinterpret_cast<char*>(m_eventsTypeBuffer), m_nEvents * 2);

            m_file.seekg(backupPos);

            // Sanity check the events & shift -1 sample
            for (uint32_t i = 0; i < m_nEvents; ++i) {
                // GDF Spec v2.51, #33: sample indexing starts from 1, hence here we compensate with -1 as in OV the first sample is in index 0
                if (m_eventsPositionBuffer[i] > 0) { m_eventsPositionBuffer[i]--; }

                if (m_eventsPositionBuffer[i] >= (m_nDataRecords * m_nSamplesPerRecord)) {
                    this->getLogManager() << Kernel::LogLevel_Warning << "File has stimulation " << m_eventsTypeBuffer[i] << " at sample count "
                            << m_eventsPositionBuffer[i] << " but the file has only " << (m_nDataRecords * m_nSamplesPerRecord)
                            << " samples of signal. Stimulation will be dropped.\n";
                    // Note that with the current design of this box its not possible to keep producing stimulation chunks after the signal has ended, and
                    // it'd be an openvibe stream convention violation to append the stimulation at t to any chunk where t \notin [chunkStart,chunkEnd].
                    // Hence drop.
                }
            }

            m_stimulationEncoder->encodeHeader();
            boxIO.markOutputAsReadyToSend(GDFReader_StimulationOutput, 0, 0);
        }

        GDF::CGDFEvent event;
        //todo check inclusive/exclusive conditions
        while ((m_currentEvent != m_nEvents)
               && m_eventsPositionBuffer[m_currentEvent] >= m_nSentSample - m_signalDesc.m_NSample
               && m_eventsPositionBuffer[m_currentEvent] < m_nSentSample)          // In current chunk range
        {
            //reads an event
            event.m_Position = m_eventsPositionBuffer[m_currentEvent];
            event.m_Type     = m_eventsTypeBuffer[m_currentEvent];

            //adds it to the list of events
            m_events.push_back(event);

            // If input already has an EOF marker, we don't add our own
            if (event.m_Type == OVTK_StimulationId_EndOfFile) { m_appendEOF = false; }

            m_currentEvent++;
        }

        //if we just read the last event
        if (m_currentEvent == m_nEvents) {
            m_eventsSent = true;
            delete[] m_eventsPositionBuffer;
            m_eventsPositionBuffer = nullptr;
            delete[] m_eventsTypeBuffer;
            m_eventsTypeBuffer = nullptr;
        }
    }

    // Send out stims. Logic:
    // If we haven't yet sent out all matrices, or we have a pending EOF stim, send a stim chunk
    // If we have sent out all matrices and we have a pending EOF, append it
    if (!m_matricesSent || m_appendEOF) {
        if (m_matricesSent && m_appendEOF) {
            //creates an end of file event
            GDF::CGDFEvent event;
            event.m_Position = uint32_t(m_nDataRecords * m_nSamplesPerRecord);
            event.m_Type     = OVTK_StimulationId_EndOfFile;

            //adds it to the list of events
            m_events.push_back(event);

            m_appendEOF = false;
        }

        // In OpenViBE, we should always send a stimulus chunk even if it was empty
        writeEvents();
        m_events.clear();

        boxIO.markOutputAsReadyToSend(GDFReader_StimulationOutput, start, end);
    }

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
