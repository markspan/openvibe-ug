#include "ovpCBoxAlgorithmBrainampFileWriter.h"
#include <fs/Files.h>

#include <cstring>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <strings.h>
#define _strcmpi strcasecmp
#endif

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

bool CBoxAlgorithmBrainampFileWriter::initialize()
{
    // Creates algorithms
    m_signalDecoder      = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_stimulationDecoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));

    m_signalDecoder->initialize();
    m_stimulationDecoder->initialize();

    // Signal stream encoder parameters
    op_sampling.initialize(m_signalDecoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));


    // Connect parameters together
    ip_signalBuffer.initialize(m_signalDecoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode));
    ip_stimulationsBuffer.initialize(m_stimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    op_matrix.initialize(m_signalDecoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    op_stimSet.initialize(m_stimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    // Configures settings according to box
    m_filePath                 = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_dictionaryFilename       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_transformStimulations    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_shouldWriteFullFilenames = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    // Markers inside the output file are numbered, we need to keep trace of how many of them we have already written
    m_markersWritten         = 0;
    m_wasMarkerHeaderWritten = false;

    // Create header, EEG and marker files
    char parentPath[1024];

    // Add the extension if it wasn't done
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    if((m_filePath.length() < 5) || (strcasecmp(m_filePath.toASCIIString() + m_filePath.length() - 5, ".vhdr") != 0))
#else
    if ((m_filePath.length() < 5) || (_strcmpi(m_filePath.toASCIIString() + m_filePath.length() - 5, ".vhdr") != 0))
#endif
    {
        m_filePath = m_filePath + ".vhdr";
    }
    if (!FS::Files::getParentPath(m_filePath.toASCIIString(), parentPath))
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Cannot access " << m_filePath << "\n";
        return false;
    }

    if (!FS::Files::directoryExists(parentPath) && !FS::Files::createParentPath(m_filePath))
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Directory [" << parentPath << "] cannot be created\n";
        return false;
    }

    // Create header file
    char filenameWithoutExtension[1024];
    FS::Files::getFilenameWithoutExtension(m_filePath.toASCIIString(), filenameWithoutExtension);

    FS::Files::openOFStream(m_headerFStream, m_filePath.toASCIIString());
    if (m_headerFStream.fail())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Cannot open " << m_filePath << " : " << strerror(errno) << "\n";
        return false;
    }

    // Create EEG file
    std::string eegFilePath = std::string(parentPath) + "/" + std::string(filenameWithoutExtension) + ".eeg";
    FS::Files::openOFStream(m_eegFStream, eegFilePath.c_str(), std::ios::out | std::ios::binary);
    if (m_eegFStream.fail())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Cannot open " << eegFilePath << " : " << strerror(errno) << "\n";
        return false;
    }

    // Create Markers file
    std::string markerFilePath = std::string(parentPath) + "/" + std::string(filenameWithoutExtension) + ".vmrk";
    FS::Files::openOFStream(m_markerFStream, markerFilePath.c_str());
    if (m_markerFStream.fail())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Cannot open " << markerFilePath << " : " << strerror(errno) << "\n";
        return false;
    }

    // Opens Marker to OpenViBE Stimulation dictionary file
    if (m_dictionaryFilename != CString(""))
    {
        std::ifstream ifs;
        FS::Files::openIFStream(ifs, m_dictionaryFilename.toASCIIString());
        if (!ifs.good()) { getLogManager() << Kernel::LogLevel_Error << "Could not open dictionary file [" << m_dictionaryFilename << "]\n"; }
        else
        {
            getLogManager() << Kernel::LogLevel_Trace << "Opening " << m_dictionaryFilename << " succeeded\n";

            // read the values from the dictionar, they are specified in format
            // <Marker Type> , <Marker Name> , <OpenViBE Stimulation>
            // Stimulations can be either written as decimal numbers, or as strings
            do
            {
                std::string line;
                std::getline(ifs, line, '\n');

                // optionally removes ending carriage return for windows / linux compatibility
                if (line.length() != 0) { if (line[line.length() - 1] == '\r') { line.erase(line.length() - 1, 1); } }

                if (line.length() != 0)
                {
                    if (line[0] == ';') // comments
                    { }
                    else
                    {
                        std::string type, name, stimulationID;
                        std::stringstream ss(line);

                        std::getline(ss, type, ',');
                        std::getline(ss, name, ',');
                        std::getline(ss, stimulationID);

                        uint64_t id                = this->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_Stimulation, stimulationID.c_str());
                        m_stimulationToMarkers[id] = type + "," + name;
                    }
                }
            } while (ifs.good());
        }
    }

    return true;
}

bool CBoxAlgorithmBrainampFileWriter::uninitialize()
{
    m_stimulationDecoder->uninitialize();
    m_signalDecoder->uninitialize();

    this->getAlgorithmManager().releaseAlgorithm(*m_stimulationDecoder);
    this->getAlgorithmManager().releaseAlgorithm(*m_signalDecoder);

    if (m_eegFStream.is_open()) { m_eegFStream.close(); }

    if (m_markerFStream.is_open()) { m_markerFStream.close(); }

    return true;
}

bool CBoxAlgorithmBrainampFileWriter::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmBrainampFileWriter::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Signal
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); i++)
    {
        ip_signalBuffer = boxContext.getInputChunk(0, i);
        m_signalDecoder->process();
        if (m_signalDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader))
        {
            // BrainAmp format uses sampling interval (between two samples) in microseconds
            const size_t samplingInterval = 1000000 / size_t(op_sampling);
            const size_t nChannels        = op_matrix->getDimensionSize(0);

            if (m_headerFStream.is_open())
            {
                std::string auxFilename = "$b";
                if (m_shouldWriteFullFilenames)
                {
                    char filenameWithoutExtension[1024];
                    FS::Files::getFilenameWithoutExtension(m_filePath.toASCIIString(), filenameWithoutExtension);
                    auxFilename = filenameWithoutExtension;
                }

                m_headerFStream << "Brain Vision Data Exchange Header File Version 1.0" << std::endl;
                m_headerFStream << "[Common Infos]" << std::endl;
                m_headerFStream << "DataFile=" << auxFilename << ".eeg" << std::endl;
                m_headerFStream << "MarkerFile=" << auxFilename << ".vmrk" << std::endl;
                m_headerFStream << "DataFormat=BINARY" << std::endl;
                m_headerFStream << "DataOrientation=MULTIPLEXED" << std::endl;
                m_headerFStream << "NumberOfChannels=" << nChannels << std::endl;
                m_headerFStream << "SamplingInterval=" << samplingInterval << std::endl;
                m_headerFStream << "[Binary Infos]" << std::endl;
                m_headerFStream << "BinaryFormat=IEEE_FLOAT_32" << std::endl; // TODO_JL extend this to more formats
                m_headerFStream << "[Channel Infos]" << std::endl;

                for (size_t c = 0; c < nChannels; c++)
                {
                    // OpenViBE format does not specify any units
                    m_headerFStream << "Ch" << (c + 1) << "=" << op_matrix->getDimensionLabel(0, c) << ",,1" << std::endl;
                }
                m_headerFStream.close();
            }
            else
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Cannot open " << m_filePath << "\n";
                return false;
            }
        }

        if (m_signalDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer))
        {
            if (m_eegFStream.good())
            {
                for (uint32_t j = 0; j < op_matrix->getDimensionSize(1); ++j)
                {
                    for (uint32_t c = 0; c < op_matrix->getDimensionSize(0); c++)
                    {
                        float value = float(op_matrix->getBuffer()[c * op_matrix->getDimensionSize(1) + j]);
                        m_eegFStream.write(reinterpret_cast<char*>(&value), sizeof(float));
                    }
                }
            }
        }

        boxContext.markInputAsDeprecated(0, i);
    }

    // Stimulations
    for (uint32_t i = 0; i < boxContext.getInputChunkCount(1); i++)
    {
        ip_stimulationsBuffer = boxContext.getInputChunk(1, i);

        m_stimulationDecoder->process();

        // Handle Header
        if (!m_wasMarkerHeaderWritten && m_stimulationDecoder->isOutputTriggerActive(
                OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader))
        {
            std::string auxFilename = "$b";
            if (m_shouldWriteFullFilenames)
            {
                char filenameWithoutExtension[1024];
                FS::Files::getFilenameWithoutExtension(m_filePath.toASCIIString(), filenameWithoutExtension);
                auxFilename = filenameWithoutExtension;
            }

            m_markerFStream << "Brain Vision Data Exchange Marker File, Version 1.0" << std::endl;
            m_markerFStream << "[Common Infos]" << std::endl;
            m_markerFStream << "DataFile=" << auxFilename << ".eeg" << std::endl;
            m_markerFStream << "[Marker Infos]" << std::endl;
            m_wasMarkerHeaderWritten = true;
        }

        if (m_stimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer))
        {
            for (uint32_t j = 0; j < op_stimSet->size(); j++)
            {
                const uint64_t id       = op_stimSet->getId(j);
                const uint64_t date     = op_stimSet->getDate(j);
                const uint64_t duration = op_stimSet->getDuration(j);

                std::string writtenMarker;
                bool writeStimulation = false;

                if (m_stimulationToMarkers.count(id))
                {
                    writtenMarker    = m_stimulationToMarkers[id];
                    writeStimulation = true;
                }
                else if (m_transformStimulations)
                {
                    std::string str  = this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_Stimulation, id).toASCIIString();
                    writtenMarker    = "Stimulus," + (str.empty() ? std::to_string(id) : str);
                    writeStimulation = true;
                }

                if (writeStimulation && m_markerFStream.good())
                {
                    m_markersWritten++;
                    m_markerFStream << "Mk" << m_markersWritten << "=" << writtenMarker << ",";

                    // Calclulate the index of the sample to which the marker is attached
                    const uint64_t sampleIdx = CTime(date).toSampleCount(op_sampling);
                    m_markerFStream << sampleIdx << ",";

                    // Minimal duration of markers inside BrainAmp format seems to be 1 sample
                    const uint64_t durationInSamples = CTime(duration).toSampleCount(op_sampling) + 1;
                    m_markerFStream << durationInSamples << ",";

                    // 0 means that the stimulation is attached to all channels
                    m_markerFStream << "0" << std::endl;
                }
            }
        }

        boxContext.markInputAsDeprecated(1, i);
    }
    return true;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
