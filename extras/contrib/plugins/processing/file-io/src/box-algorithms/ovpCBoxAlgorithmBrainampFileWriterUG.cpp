/* Project: UG-lab plugins for OpenVibe
 * AUTHORS AND CONTRIBUTORS: Andreev A., Barachant A., Congedo M., Ionescu,Gelu, MMS

 * This file is part of "UG-lab plugins for OpenVibe".
 * You can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * This file is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Brain Invaders. If not, see http://www.gnu.org/licenses/.
 */

#include "ovpCBoxAlgorithmBrainampFileWriterUG.h"

#include <string>
#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {


//documentation Appendix B EEG file format: http://tsgdoc.socsci.ru.nl/images/d/d1/BrainVision_Recorder_UM.pdf

bool CBoxAlgorithmBrainampFileWriterUG::initialize()
{
    m_isVmrkHeaderFileWritten = false;

    //init input signal 1
    m_streamDecoder = new Toolkit::TSignalDecoder<CBoxAlgorithmBrainampFileWriterUG>(*this, 0);

    //init input stimulation 1 
    m_stimDecoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmBrainampFileWriterUG>(*this, 1);

    //Get parameters:
    const CString filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    const CString tmp                = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    const CIdentifier binaryFormatID = this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_BinaryFormat, tmp);

    if (binaryFormatID == OVP_TypeId_BinaryFormat_int16_t) { m_binaryFormat = BinaryFormat_Integer16; }
    else if (binaryFormatID == OVP_TypeId_BinaryFormat_uint16_t) { m_binaryFormat = BinaryFormat_UnsignedInteger16; }
    else if (binaryFormatID == OVP_TypeId_BinaryFormat_float) { m_binaryFormat = BinaryFormat_Float32; }
    else
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Unknown binary format: " << binaryFormatID << "\n";
        return false;
    }

    //Perform checks:
    if (std::string(filename).empty())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Header file path is empty!\n";
        return false;
    }

    if (std::string(filename).substr(filename.length() - 5, 5) != std::string(".vhdr"))
    {
        this->getLogManager() << Kernel::LogLevel_Warning << "The supplied output file does not end with .vhdr\n";
    }

    m_headerFilename = filename;

    m_headerFile.open(m_headerFilename.c_str());
    if (!m_headerFile.good())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not open header file [" << m_headerFilename << "]\n";
        return false;
    }

    //this->getLogManager() << Kernel::LogLevel_ImportantWarning << m_sHeaderFilename << "\n";

    m_dataFilename = m_headerFilename.substr(0, m_headerFilename.length() - 5) + std::string(".eeg");

    m_dataFile.open(m_dataFilename.c_str(), std::ios::binary);
    if (!m_dataFile.good())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not open data file [" << m_dataFilename << "]\n";
        return false;
    }

    m_markerFilename = m_headerFilename.substr(0, m_headerFilename.length() - 5) + std::string(".vmrk");

    m_markerFile.open(m_markerFilename.c_str());
    if (!m_markerFile.good())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not open marker file [" << m_markerFilename << "]\n";
        return false;
    }

    return true;
}

bool CBoxAlgorithmBrainampFileWriterUG::uninitialize()
{
    if (m_streamDecoder)
    {
        m_streamDecoder->uninitialize();
        delete m_streamDecoder;
    }

    // uninit input stimulation
    if (m_stimDecoder)
    {
        m_stimDecoder->uninitialize();
        delete m_stimDecoder;
    }

    //close files
    m_headerFile.flush();
    m_headerFile.close();

    m_dataFile.flush();
    m_dataFile.close();

    m_markerFile.flush();
    m_markerFile.close();

    return true;
}

bool CBoxAlgorithmBrainampFileWriterUG::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmBrainampFileWriterUG::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //1. Process signal
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) //first input channel data
    {
        m_streamDecoder->decode(i);

        //HEADER
        if (m_streamDecoder->isHeaderReceived())
        {
            m_matrix   = m_streamDecoder->getOutputMatrix();
            m_sampling = m_streamDecoder->getOutputSamplingRate();

            writeHeaderFile();
        }

        //BUFFER
        if (m_streamDecoder->isBufferReceived())
        {
            // size_t channelCount = m_Matrix->getDimensionSize(0);
            switch (m_binaryFormat)
            {
                case BinaryFormat_Integer16:
                    saveBuffer(int16_t(0));
                    break;

                case BinaryFormat_UnsignedInteger16:
                    saveBuffer(uint16_t(0));
                    break;

                case BinaryFormat_Float32:
                    saveBuffer(float(0));
                    break;
                default: break;
            }
        }

        //END
        if (m_streamDecoder->isEndReceived()) { }

        boxContext.markInputAsDeprecated(0, i);
    }

    // Make sure the marker file header is written in any case
    if (!m_isVmrkHeaderFileWritten)
    {
        const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        const std::string formated(formatTime(now));

        m_markerFile << "Brain Vision Data Exchange Marker File, Version 1.0" << std::endl << std::endl
                << "[Common Infos]" << std::endl
                << "Codepage=ANSI" << std::endl
                << "DataFile=" << getShortName(m_dataFilename) << std::endl << std::endl
                << "[Marker Infos]" << std::endl
                << "; Each entry: Mk<Marker number>=<Type>,<Description>,<Position in data points>," << std::endl
                << "; <Size in data points>, <Channel number (0 = marker is related to all channels)>" << std::endl
                << "; Fields are delimited by commas, some fields might be omitted (empty)." << std::endl
                << "; Commas in type or description text are coded as \"\\1\"." << std::endl
                << "Mk1=New Segment,,1,1,0," << formated << "000000" << std::endl;

        m_isVmrkHeaderFileWritten = true;
    }

    //2. Process stimulations - input channel 1
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i)
    {
        // uint64_t chunkStartTime =boxContext.getInputChunkStartTime(0, i);

        m_stimDecoder->decode(i);

        //buffer 
        if (m_stimDecoder->isBufferReceived())
        {
            CStimulationSet* stimSet = m_stimDecoder->getOutputStimulationSet();

            // Loop on stimulations
            for (size_t j = 0; j < stimSet->size(); ++j)
            {
                const uint64_t id = stimSet->getId(j);
                // Only write hardware label codes OVTK_StimulationId_Label(1..255)
                if (id < OVTK_StimulationId_LabelStart + 1 || id > OVTK_StimulationId_LabelEnd) { continue; }
                const uint64_t code     = id - OVTK_StimulationId_LabelStart; // extract raw value 1-255
                const uint64_t position = CTime(stimSet->getDate(j)).toSampleCount(m_sampling) + 1;

                m_markerFile << "Mk" << m_stimulationCounter << "=Stimulus," << code << "," << position << ",1,0" << std::endl;

                m_stimulationCounter++;
            }
        }

        boxContext.markInputAsDeprecated(1, i);
    }

    return true;
}

bool CBoxAlgorithmBrainampFileWriterUG::writeHeaderFile()
{
    const size_t nChannel = m_matrix->getDimensionSize(0);
    // size_t samplesPerChunk = m_Matrix->getDimensionSize(1);

    const double samplingInterval = 1000000.0 / double(m_sampling);

    CString format("UNKNOWN");
    switch (m_binaryFormat)
    {
        case BinaryFormat_Integer16:
            format = "INT_16";
            break;

        case BinaryFormat_UnsignedInteger16:
            format = "UINT_16";
            break;

        case BinaryFormat_Float32:
            format = "IEEE_FLOAT_32";
            break;
        default:
            this->getLogManager() << Kernel::LogLevel_Error << "EEG format unknown!\n";
            break;
    }

    m_headerFile << "Brain Vision Data Exchange Header File Version 1.0" << std::endl
            << "; Data created by the Vision Recorder" << std::endl << std::endl
            << "[Common Infos]" << std::endl
            << "Codepage=ANSI" << std::endl
            << "DataFile=" << getShortName(m_dataFilename) << std::endl
            << "MarkerFile=" << getShortName(m_markerFilename) << std::endl
            << "DataFormat=BINARY" << std::endl
            << "; Data orientation: MULTIPLEXED=ch1,pt1, ch2,pt1 ..." << std::endl //sample 1, sample 2 ...
            << "DataOrientation=MULTIPLEXED" << std::endl
            << "NumberOfChannels=" << nChannel << std::endl
            << "; Sampling interval in microseconds" << std::endl
            << "SamplingInterval=" << std::fixed << std::setprecision(5) << samplingInterval << std::endl
            << std::endl
            << "[Binary Infos]" << std::endl
            << "BinaryFormat=" << format.toASCIIString() << std::endl
            << std::endl;

    m_headerFile << "[Channel Infos]" << std::endl
            << "; Each entry: Ch<Channel number>=<Name>,<Reference channel name>," << std::endl
            << "; <Resolution in \"Unit\">,<Unit>, Future extensions.." << std::endl
            << "; Fields are delimited by commas, some fields might be omitted (empty)." << std::endl
            << "; Commas in channel names are coded as \"\\1\"." << std::endl;

    for (size_t i = 0; i < nChannel; ++i)
    {
        m_headerFile << "Ch" << (i + 1) << "=" << m_matrix->getDimensionLabel(0, i) << ",,1," << std::endl; //resolution = 1 
    }

    m_headerFile << std::endl;

    m_headerFile << "[Comment]" << std::endl << std::endl
            << "A m p l i f i e r  S e t u p" << std::endl
            << "============================" << std::endl
            << "Number of channels: " << nChannel << std::endl
            << "Sampling Rate [Hz]: " << m_sampling << std::endl
            << "Interval [µS]: " << std::fixed << std::setprecision(5) << samplingInterval << std::endl
            << std::endl;

    return true;
}

std::string CBoxAlgorithmBrainampFileWriterUG::getShortName(std::string fullpath)
{
    size_t pos = fullpath.find_last_of('\\');
    if (pos == std::string::npos) { pos = fullpath.find_last_of('/'); }
    if (pos != std::string::npos) { return fullpath.substr(pos + 1, fullpath.size() - pos); }
    return fullpath;
}

std::string CBoxAlgorithmBrainampFileWriterUG::formatTime(const boost::posix_time::ptime now)
{
    using namespace boost::posix_time;

    static std::locale loc(std::wcout.getloc(), new wtime_facet(L"%Y%m%d%H%M%S"));

    std::basic_stringstream<wchar_t> wss;
    wss.imbue(loc);
    wss << now;
    std::wstring wstr = wss.str();
    std::string str(wstr.begin(), wstr.end());

    return str;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
