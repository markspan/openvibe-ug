#include "ovpCGDFFileWriter.h"

#include <system/ovCMemory.h>

#include <iostream>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <algorithm> // std::min, etc on VS2013

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

void CGDFFileWriter::setChannelCount(const size_t n)
{
    m_fixedHeader.m_NSignals             = n;
    m_fixedHeader.m_NBytesInHeaderRecord = (n + 1) * 256;

    m_variableHeader.setChannelCount(n);
    m_channelScale.resize(n);
    m_channelOffset.resize(n);
    m_samples.resize(n);
    m_nSamples.resize(n);
}

void CGDFFileWriter::setChannelName(const size_t index, const char* name)
{
    const uint32_t size = sizeof(m_variableHeader[index].m_Label);

    // initialize label with spaces
    memset(m_variableHeader[index].m_Label, ' ', size);

    // copy at most size-1 characters, leaving room for 1 space. Note that the label is not NULL terminated.
    const uint32_t len = std::min<uint32_t>(strlen(name), size - 1);
    memcpy(m_variableHeader[index].m_Label, name, len);

    m_variableHeader[index].m_ChannelType          = 17;  //float64
    m_variableHeader[index].m_NSamplesInEachRecord = 1;
    // This scaling values are maxed and not representative of the signal, because values are stored as doubles and not limited in precision.
    m_variableHeader[index].m_PhysicalMin          = std::numeric_limits<float>::lowest()/2;
    m_variableHeader[index].m_PhysicalMax          = std::numeric_limits<float>::max()/2;
    m_variableHeader[index].m_DigitalMin           = std::numeric_limits<int64_t>::lowest()/2;
    m_variableHeader[index].m_DigitalMax           = std::numeric_limits<int64_t>::max()/2;

    m_channelScale[index] = (m_variableHeader[index].m_PhysicalMax - m_variableHeader[index].m_PhysicalMin) / static_cast<double>(m_variableHeader[index].m_DigitalMax - m_variableHeader[index].m_DigitalMin);
    m_channelOffset[index] = m_variableHeader[index].m_PhysicalMin - m_channelScale[index] * static_cast<double>(m_variableHeader[index].m_DigitalMin);

    memcpy(m_variableHeader[index].m_PhysicalDimension, "uV", sizeof("uV"));
}

void CGDFFileWriter::setSampleCountPerBuffer(const size_t n)
{
    m_samplesPerChannel = n;

    //save the fixed header
    if (m_fixedHeader.save(m_file))
    {
        //save the variable header
        if (!m_variableHeader.save(m_file)) { m_error = true; }
    }
    else { m_error = true; }

    if (m_error) { this->getLogManager() << Kernel::LogLevel_Warning << "Error while writing to the output file!\n"; }
}

void CGDFFileWriter::setSamplingRate(const size_t sampling)
{
    m_sampling = sampling;

    m_fixedHeader.m_DurationDataRecordNum = 1;
    m_fixedHeader.m_DurationDataRecordDen = m_sampling;
}

void CGDFFileWriter::setSampleBuffer(const double* buffer)
{
    //for each channel
    for (size_t j = 0; j < m_fixedHeader.m_NSignals; ++j)
    {
        for (size_t i = 0; i < m_samplesPerChannel; ++i)
        {
            //gets a sample value
            const double sample = buffer[j * m_samplesPerChannel + i];

            //copy its current sample
            m_samples[j].push_back((sample - m_channelOffset[j]) / m_channelScale[j]);
        }

        //updates the sample count
        m_nSamples[j] += m_samplesPerChannel;
    }

    // this->getLogManager() << Kernel::LogLevel_Info << "Received up to " << m_nSamples[0] << " samples\n";

    //save in the file
    saveMatrixData();

    //updates the fixed header
    m_fixedHeader.m_NDataRecords = m_nSamples[0];
    //updates the variable header
    if (m_fixedHeader.update(m_file)) { if (!m_variableHeader.update(m_file)) { m_error = true; } }
    else { m_error = true; }

    if (m_error) { this->getLogManager() << Kernel::LogLevel_Warning << "Error while writing to the output file!\n"; }
}

/*
* Experiment callback
*
*/
void CGDFFileWriter::setExperimentInfo()
{
    uint64_t value = m_xpInfoDecoder->getOutputExperimentID();
    sprintf(m_fixedHeader.m_RecordingID, "0x%08llX", value);
    m_fixedHeader.m_RecordingID[10] = ' ';

    value = m_xpInfoDecoder->getOutputSubjectID();
    sprintf(m_fixedHeader.m_PatientID, "0x%08llX ", value);
    m_fixedHeader.m_PatientID[11] = ' ';


    m_xpInfoDecoder->getOutputSubjectAge();
    // TODO using the experiment date, compute the birthdate?

    value = m_xpInfoDecoder->getOutputSubjectGender();
    switch (value)
    {
        case OVTK_Value_Sex_Female:
            m_fixedHeader.m_PatientID[17] = 'F';
            break;

        case OVTK_Value_Sex_Male:
            m_fixedHeader.m_PatientID[17] = 'M';
            break;

        case OVTK_Value_Sex_Unknown:
        case OVTK_Value_Sex_NotSpecified:
        default:
            m_fixedHeader.m_PatientID[17] = 'X';
            break;
    }
    m_fixedHeader.m_PatientID[18] = ' ';


    value                        = m_xpInfoDecoder->getOutputLaboratoryID();
    m_fixedHeader.m_LaboratoryID = value;


    value                        = m_xpInfoDecoder->getOutputTechnicianID();
    m_fixedHeader.m_TechnicianID = value;

    CString* subjectName = m_xpInfoDecoder->getOutputSubjectName();
    std::string formattedSubjectName((*subjectName).toASCIIString());
    formattedSubjectName.replace(formattedSubjectName.begin(), formattedSubjectName.end(), ' ', '_');
    sprintf(m_fixedHeader.m_PatientID + 31, "%s", formattedSubjectName.c_str());

    if (!m_fixedHeader.save(m_file))
    {
        m_error = true;

        this->getLogManager() << Kernel::LogLevel_Warning << "Error while writing to the output file!\n";
    }
}

void CGDFFileWriter::setStimulation(const uint64_t identifier, const uint64_t date) { m_events.push_back(std::pair<uint64_t, uint64_t>(identifier, date)); }


bool CGDFFileWriter::initialize()
{
    m_signalDecoder      = new Toolkit::TSignalDecoder<CGDFFileWriter>;
    m_xpInfoDecoder      = new Toolkit::TExperimentInfoDecoder<CGDFFileWriter>;
    m_stimulationDecoder = new Toolkit::TStimulationDecoder<CGDFFileWriter>;

    m_signalDecoder->initialize(*this, 1);
    m_xpInfoDecoder->initialize(*this, 0);
    m_stimulationDecoder->initialize(*this, 2);

    // Parses box settings to find filename
    m_filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    return true;
}

bool CGDFFileWriter::uninitialize()
{
    // See that no event is outside the signal by padding the signal with zeroes if necessary
    padByEvents();

    //If the file is not open, that mean that the box is muted. If the file is not open because of a bug, it should have already been notify
    if (m_file.is_open())
    {
        //update the fixed header
        if (!m_nSamples.empty())
        {
            m_fixedHeader.m_NDataRecords = m_nSamples[0];
            this->getLogManager() << Kernel::LogLevel_Trace << "Saving " << m_nSamples[0] << " data records\n";
        }

        if (m_fixedHeader.update(m_file))
        {
            //To save the Physical/Digital max/min values
            if (!m_variableHeader.update(m_file)) { m_error = true; }
        }
        else { m_error = true; }

        //write events
        if (!m_events.empty())
        {
            this->getLogManager() << Kernel::LogLevel_Trace << "Saving " << m_events.size() << " events\n";
            saveEvents();
        }

        if (m_error) { this->getLogManager() << Kernel::LogLevel_Warning << "Error while writing to the output file!\n"; }

        m_file.close();
    }

    if (m_signalDecoder)
    {
        m_signalDecoder->uninitialize();
        delete m_signalDecoder;
    }
    if (m_xpInfoDecoder)
    {
        m_xpInfoDecoder->uninitialize();
        delete m_xpInfoDecoder;
    }
    if (m_stimulationDecoder)
    {
        m_stimulationDecoder->uninitialize();
        delete m_stimulationDecoder;
    }

    return true;
}

bool CGDFFileWriter::processInput(const size_t /*index*/)
{
    if (m_error) { return false; }
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

void CGDFFileWriter::saveMatrixData()
{
    if (!m_file.is_open())
    {
        m_file.open(m_filename, std::ios::binary | std::ios::trunc);

        if (!m_file.good())
        {
            m_error = true;
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << m_filename << "]\n";
            return;
        }
    }

    size_t j;
    m_file.seekp(0, std::ios::end);

    //to "convert" (if needed) the double in little endian format
    uint8_t littleEndianBuffer[8];

    for (size_t i = 0; i < m_samples[0].size(); ++i)
    {
        for (j = 0; j < m_samples.size(); ++j)
        {
            const double v = m_samples[j][i];
            System::Memory::hostToLittleEndian(v, littleEndianBuffer);
            m_file.write(reinterpret_cast<char*>(littleEndianBuffer), sizeof(littleEndianBuffer));
        }
    }
    for (j = 0; j < m_samples.size(); ++j) { m_samples[j].clear(); }
}

void CGDFFileWriter::padByEvents()
{
    if (!m_file.is_open())
    {
        m_file.open(m_filename, std::ios::binary | std::ios::trunc);

        if (!m_file.good())
        {
            m_error = true;

            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << m_filename << "]\n";
            return;
        }
    }

    uint32_t maxPos = 0;
    for (const auto& e : m_events)
    {
        // GDF Spec v2.51, #33: sample indexing starts from 1, hence +1
        const uint32_t pos = uint32_t(CTime(e.second).toSampleCount(m_sampling) + 1);
        if (pos > m_nSamples[0] + 1)
        {
            this->getLogManager() << Kernel::LogLevel_Warning << "Stimulation " << uint16_t(e.first & 0xFFFF) << " will be written at " << pos
                    << " (after last sample at " << m_nSamples[0] + 1 << "), padding the signal\n";
            maxPos = std::max<uint32_t>(pos, maxPos);
        }
    }
    if (maxPos > 0)
    {
        const CMatrix* oMatrix = m_signalDecoder->getOutputMatrix();
        CMatrix zeros;
        zeros.copyDescription(*oMatrix);
        while (maxPos >= m_nSamples[0] + 1) { setSampleBuffer(zeros.getBuffer()); }
    }
}

void CGDFFileWriter::saveEvents()
{
    if (!m_file.is_open())
    {
        m_file.open(m_filename, std::ios::binary | std::ios::trunc);

        if (!m_file.good())
        {
            m_error = true;

            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << m_filename << "]\n";
            return;
        }
    }

    m_file.seekp(0, std::ios::end);

    //event mode
    m_file.put(1);

    //sample rate associated with event positions
    m_file.put(0);
    m_file.put(0);
    m_file.put(0);

    //number of events
    uint8_t littleEndianBuffer[sizeof(size_t)];  // Needs to be as long as the type returned by m_events.size() (see impl. of hostToLittleEndian())
    System::Memory::hostToLittleEndian((int)m_events.size(), littleEndianBuffer);
    m_file.write(reinterpret_cast<char*>(littleEndianBuffer), sizeof(uint32_t));

    // write event positions
    for (const auto& e : m_events)
    {
        // GDF Spec v2.51, #33: sample indexing starts from 1, hence +1
        const uint32_t pos = uint32_t(CTime(e.second).toSampleCount(m_sampling) + 1);

        System::Memory::hostToLittleEndian(pos, littleEndianBuffer);
        m_file.write(reinterpret_cast<char*>(littleEndianBuffer), sizeof(uint32_t));
    }

    // write event types
    for (const auto& e : m_events)
    {
        //Force to use only 16bits stimulations IDs
        const uint16_t type = uint16_t(e.first & 0xFFFF);

        System::Memory::hostToLittleEndian(type, littleEndianBuffer);
        m_file.write(reinterpret_cast<char*>(littleEndianBuffer), sizeof(uint16_t));
    }
}

bool CGDFFileWriter::process()
{
    if (!m_file.is_open())
    {
        m_file.open(m_filename, std::ios::binary | std::ios::trunc);

        if (!m_file.good())
        {
            m_error = true;
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << m_filename << "]\n";
            return false;
        }
    }

    Kernel::IBoxIO& boxIO = this->getDynamicBoxContext();

    //Experiment information
    for (size_t i = 0; i < boxIO.getInputChunkCount(0); ++i)
    {
        m_xpInfoDecoder->decode(i);
        if (m_xpInfoDecoder->isHeaderReceived()) { setExperimentInfo(); }
        boxIO.markInputAsDeprecated(0, i);
    }


    //Signal
    for (size_t i = 0; i < boxIO.getInputChunkCount(1); ++i)
    {
        m_signalDecoder->decode(i);
        if (m_signalDecoder->isHeaderReceived())
        {
            CMatrix* oMatrix      = m_signalDecoder->getOutputMatrix();
            const size_t nChannel = oMatrix->getDimensionSize(0);
            setChannelCount(nChannel);
            for (size_t c = 0; c < nChannel; ++c) { setChannelName(c, oMatrix->getDimensionLabel(0, c)); }
            setSamplingRate(size_t(m_signalDecoder->getOutputSamplingRate()));
            setSampleCountPerBuffer(oMatrix->getDimensionSize(1));
        }
        if (m_signalDecoder->isBufferReceived())
        {
            CMatrix* oMatrix = m_signalDecoder->getOutputMatrix();
            double* buffer   = oMatrix->getBuffer();
            setSampleBuffer(buffer);
        }
        boxIO.markInputAsDeprecated(1, i);
    }

    //Stimulations
    for (uint32_t i = 0; i < boxIO.getInputChunkCount(2); ++i)
    {
        m_stimulationDecoder->decode(i);

        if (m_stimulationDecoder->isBufferReceived())
        {
            CStimulationSet* stimulationSet = m_stimulationDecoder->getOutputStimulationSet();
            for (size_t j = 0; j < stimulationSet->size(); ++j)
            {
                setStimulation(stimulationSet->getId(j), stimulationSet->getDate(j));
            }
        }

        boxIO.markInputAsDeprecated(2, i);
    }

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
