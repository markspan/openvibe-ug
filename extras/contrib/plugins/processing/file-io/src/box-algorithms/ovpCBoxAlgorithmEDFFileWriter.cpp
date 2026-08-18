#include "ovpCBoxAlgorithmEDFFileWriter.h"

#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

/*******************************************************************************/

bool CBoxAlgorithmEDFFileWriter::initialize()
{
    m_experimentInfoDecoder.initialize(*this, 0);
    m_signalDecoder.initialize(*this, 1);
    m_stimDecoder.initialize(*this, 2);

    m_filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    m_sampling  = 0;
    m_nChannels = 0;

    m_isFileOpened = false;
    m_fileHandle   = -1;

    return true;
}

/*******************************************************************************/

bool CBoxAlgorithmEDFFileWriter::uninitialize()
{
    if (!m_isFileOpened) {
        // Fine, we didn't manage to write anything
        this->getLogManager() << Kernel::LogLevel_Warning << "Exiting without writing a file (open failed or no signal header received).\n";

        // Clear the queue
        std::queue<double> empty;
        std::swap(m_buffer, empty);

        m_experimentInfoDecoder.uninitialize();
        m_signalDecoder.uninitialize();
        m_stimDecoder.uninitialize();

        return true;
    }

    this->getLogManager() << Kernel::LogLevel_Info << "Writing the file, this may take a moment.\n";

     for (size_t c = 0; c < m_nChannels; ++c) {
        if (edf_set_samplefrequency(m_fileHandle, c, m_sampling) == -1) {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_set_sampling failed!\n";
            return false;
        }
        if (edf_set_physical_maximum(m_fileHandle, c, m_channelInfo[c].max) == -1)//1.7*10^308)
        {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_set_physical_maximum failed!\n";
            return false;
        }
        if (edf_set_physical_minimum(m_fileHandle, c, m_channelInfo[c].min) == -1)//-1.7*10^308)
        {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_set_physical_minimum failed!\n";
            return false;
        }
        if (edf_set_digital_maximum(m_fileHandle, c, 32767) == -1) {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_set_digital_maximum failed!\n";
            return false;
        }
        if (edf_set_digital_minimum(m_fileHandle, c, -32768) == -1) {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_set_digital_minimum failed!\n";
            return false;
        }
        if (edf_set_label(m_fileHandle, c, m_signalDecoder.getOutputMatrix()->getDimensionLabel(0, c)) == -1) {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_set_label failed!\n";
            return false;
        }
     }
     const size_t n = m_sampling * m_nChannels;
     std::vector<double> tmpBuffer(n), tmpBufferToWrite(n); // A buffer chunk and Transposed buffer chunk

    // Write out all the complete buffers
    if (m_buffer.size() >= n) {
        this->getLogManager() << Kernel::LogLevel_Trace << "((int) buffer.size() >= m_sampling*m_NChannels)\n";
        while (m_buffer.size() >= n) {
            this->getLogManager() << Kernel::LogLevel_Trace << "while((int)buffer.size() >= m_sampling*m_NChannels)\n";
            for (size_t i = 0; i < n; ++i) {
                tmpBuffer[i] = double(m_buffer.front());
                m_buffer.pop();
            }

            for (size_t c = 0; c < m_nChannels; ++c) {
                for (size_t s = 0; s < m_sampling; ++s) { tmpBufferToWrite[c * m_sampling + s] = tmpBuffer[s * m_nChannels + c]; }
            }

            if (edf_blockwrite_physical_samples(m_fileHandle, tmpBufferToWrite.data()) == -1) {
                this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_blockwrite_physical_samples: Could not write samples in file [" << m_filename << "]\n";
                return false;
            }
        }
    }

    // Do we have a partial buffer? If so, write it out padded by zeroes
    if (!m_buffer.empty()) {
        this->getLogManager() << Kernel::LogLevel_Trace << "if(buffer.size() > 0))\n";
        for (size_t i = 0; i < n; ++i) { tmpBuffer[i] = 0; }

        for (size_t i = 0; i < m_buffer.size(); ++i) {
            tmpBuffer[i] = double(m_buffer.front());
            m_buffer.pop();
        }

        for (size_t c = 0; c < m_nChannels; ++c) {
            for (size_t s = 0; s < m_sampling; ++s) { tmpBufferToWrite[c * m_sampling + s] = tmpBuffer[s * m_nChannels + c]; }
        }

        if (edf_blockwrite_physical_samples(m_fileHandle, tmpBufferToWrite.data()) == -1) {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edf_blockwrite_physical_samples: Could not write samples in file [" << m_filename << "]\n";
            return false;
        }
    }

    if (edfclose_file(m_fileHandle) == -1) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edfclose_file: Could not close file [" << m_filename << "]\n";
        return false;
    }

    m_experimentInfoDecoder.uninitialize();
    m_signalDecoder.uninitialize();
    m_stimDecoder.uninitialize();

    return true;
}

/*******************************************************************************/

bool CBoxAlgorithmEDFFileWriter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

/*******************************************************************************/

bool CBoxAlgorithmEDFFileWriter::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //iterate over all chunk on signal input
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        m_signalDecoder.decode(i);

        if (m_signalDecoder.isHeaderReceived()) {
            m_sampling         = size_t(m_signalDecoder.getOutputSamplingRate());
            m_nChannels        = m_signalDecoder.getOutputMatrix()->getDimensionSize(0);
            m_nSamplesPerChunk = m_signalDecoder.getOutputMatrix()->getDimensionSize(1);

            m_fileHandle = edfopen_file_writeonly(m_filename.toASCIIString(), EDFLIB_FILETYPE_EDFPLUS, m_nChannels);
            if (m_fileHandle < 0) {
                this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << m_filename << "]\n";
                switch (m_fileHandle) {
                    case EDFLIB_MALLOC_ERROR:
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "EDFLIB_MALLOC_ERROR: ";
                        break;
                    case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY:
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "EDFLIB_NO_SUCH_FILE_OR_DIRECTORY: ";
                        break;
                    case EDFLIB_MAXFILES_REACHED:
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "EDFLIB_MAXFILES_REACHED: ";
                        break;
                    case EDFLIB_FILE_ALREADY_OPENED:
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "EDFLIB_FILE_ALREADY_OPENED: ";
                        break;
                    case EDFLIB_NUMBER_OF_SIGNALS_INVALID:
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "EDFLIB_NUMBER_OF_SIGNALS_INVALID: ";
                        break;
                    default:
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "EDFLIB_UNKNOWN_ERROR: ";
                        break;
                }
                return false;
            }

            //set file parameters
            edf_set_startdatetime(m_fileHandle, 0, 0, 0, 0, 0, 0);

            for (size_t c = 0; c < m_nChannels; ++c) {
                //Creation of one information channel structure per channel
                channel_info_t channelInfo = { std::numeric_limits<double>::max(), -std::numeric_limits<double>::max() };
                m_channelInfo.push_back(channelInfo);
            }

            m_isFileOpened = true;
        }

        if (m_signalDecoder.isBufferReceived()) {
            //put sample in the buffer
            CMatrix* matrix = m_signalDecoder.getOutputMatrix();
            for (size_t s = 0; s < m_nSamplesPerChunk; ++s) {
                for (size_t c = 0; c < m_nChannels; ++c) {
                    double value = matrix->getBuffer()[c * m_nSamplesPerChunk + s];
                    if (value > m_channelInfo[c].max) { m_channelInfo[c].max = value; }
                    if (value < m_channelInfo[c].min) { m_channelInfo[c].min = value; }
                    m_buffer.push(value);
                }
            }
        }

        if (m_signalDecoder.isEndReceived()) { }
    }

    if (m_isFileOpened) {
        //iterate over all chunk on experiment information input
        for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
            m_experimentInfoDecoder.decode(i);

            if (m_experimentInfoDecoder.isHeaderReceived()) {
                //set patient code
                edf_set_patientcode(m_fileHandle, std::to_string(m_experimentInfoDecoder.getOutputSubjectID()).c_str());

                //set patient gender
                switch (m_experimentInfoDecoder.getOutputSubjectGender()) {
                    case 2: //female
                        edf_set_gender(m_fileHandle, 0);
                        break;
                    case 1: //male
                        edf_set_gender(m_fileHandle, 1);
                        break;
                    case 0: //unknown
                        break;
                    case 9: //unspecified
                    default:
                        break;
                }

                //set patient age
                const std::string patientAge = ("Patient age = " + std::to_string(m_experimentInfoDecoder.getOutputSubjectAge()));
                edf_set_patient_additional(m_fileHandle, patientAge.c_str());
            }
        }
    }


    //iterate over all chunk on stimulation input
    for (size_t i = 0; i < boxContext.getInputChunkCount(2); ++i) {
        m_stimDecoder.decode(i);

        if (m_stimDecoder.isHeaderReceived()) { }

        if (m_stimDecoder.isBufferReceived()) {
            const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();
            for (size_t j = 0; j < stimSet->size(); ++j) {
                const uint64_t date    = stimSet->getDate(j);
                const int64_t stimDate = int64_t(CTime(date).toSeconds() / 0.0001);

                const uint64_t duration    = stimSet->getDuration(j);
                const int64_t stimDuration = int64_t(CTime(duration).toSeconds() / 0.0001);

                const uint64_t id = stimSet->getId(j);
                CString stimID    = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, id);
                if (stimID.length() == 0) {
                    // If the stimulation number has not been registered as an enum, just pass the number
                    std::stringstream ss;
                    ss << id;
                    stimID = ss.str().c_str();
                }
                const int result = edfwrite_annotation_utf8(m_fileHandle, stimDate, stimDuration, stimID.toASCIIString());
                if (result == -1) {
                    this->getLogManager() << Kernel::LogLevel_ImportantWarning << "edfwrite_annotation_utf8 failed!\n";
                    return false;
                }
            }
        }

        if (m_stimDecoder.isEndReceived()) { }
    }

    return true;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
