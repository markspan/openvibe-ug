/* Project: UG-lab plugins for OpenVibe
 * AUTHORS AND CONTRIBUTORS: Andreev A., Barachant A., Congedo M., Ionescu,Gelu, MMS

 * This file is part of "UG-lab plugins for OpenVibe".
 * You can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * This file is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Brain Invaders. If not, see http://www.gnu.org/licenses/.
 */

// UG fork: new in this fork, see ovpCBoxAlgorithmBrainampFileWriterUG.h.

#include "ovpCBoxAlgorithmBrainampFileWriterUG.h"

#include <openvibe/acquisition-messaging/ovChannelInfo.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

CBoxAlgorithmBrainampFileWriterUG::SphericalPosition CBoxAlgorithmBrainampFileWriterUG::toBrainVision(const double theta, const double azimuth)
{
    // BrainVision measures phi from the right (T8) towards the front, as Charmeleon does, but within -90..90: a
    // position on the left half of the head is written with a negative theta and phi turned by 180 degrees,
    // e.g. Fp1 (theta 90, azimuth 108) as -90,-72
    double phi = std::fmod(azimuth, 360.0);
    if (phi > 180.0) { phi -= 360.0; }
    if (phi <= -180.0) { phi += 360.0; }
    if (theta == 0.0) { return {0.0, 0.0}; }
    if (phi > 90.0 + 1e-9 || phi < -90.0 - 1e-9) {
        phi += phi > 0 ? -180.0 : 180.0;
        return {-theta, phi};
    }
    return {theta, phi};
}

//documentation Appendix B EEG file format: http://tsgdoc.socsci.ru.nl/images/d/d1/BrainVision_Recorder_UM.pdf

bool CBoxAlgorithmBrainampFileWriterUG::initialize()
{
    m_isVmrkHeaderFileWritten = false;

    //init input signal 1
    m_streamDecoder = new Toolkit::TSignalDecoder<CBoxAlgorithmBrainampFileWriterUG>(*this, 0);

    //init input stimulation 1 
    m_stimDecoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmBrainampFileWriterUG>(*this, 1);

    //Get parameters:
    const CString directory = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const CString filename  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    const CString tmp                = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
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

    // A header filename with a full path replaces the directory
    const std::filesystem::path headerPath = std::filesystem::u8path(directory.toASCIIString()) / std::filesystem::u8path(filename.toASCIIString());
    std::error_code error;
    if (headerPath.has_parent_path()) { std::filesystem::create_directories(headerPath.parent_path(), error); }
    if (error)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not create directory [" << headerPath.parent_path().generic_u8string() << "]: "
                << error.message() << "\n";
        return false;
    }

    m_headerFilename = headerPath.generic_u8string();
    this->getLogManager() << Kernel::LogLevel_Info << "Recording to [" << m_headerFilename << "]\n";

    m_headerFile.open(headerPath);
    if (!m_headerFile.good())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not open header file [" << m_headerFilename << "]\n";
        return false;
    }

    //this->getLogManager() << Kernel::LogLevel_ImportantWarning << m_sHeaderFilename << "\n";

    m_dataFilename = m_headerFilename.substr(0, m_headerFilename.length() - 5) + std::string(".eeg");

    m_dataFile.open(std::filesystem::u8path(m_dataFilename), std::ios::binary);
    if (!m_dataFile.good())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not open data file [" << m_dataFilename << "]\n";
        return false;
    }

    m_markerFilename = m_headerFilename.substr(0, m_headerFilename.length() - 5) + std::string(".vmrk");

    m_markerFile.open(std::filesystem::u8path(m_markerFilename));
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
            // UG fork: one header per file. Upstream appended a second one to the .vhdr, which then described neither.
            if (m_isHeaderWritten)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "The signal started again with a new header; " << m_headerFilename
                        << " describes the first one only, so the recording stops here.\n";
                return false;
            }
            m_matrix   = m_streamDecoder->getOutputMatrix();
            m_sampling = m_streamDecoder->getOutputSamplingRate();

            writeHeaderFile();
            m_isHeaderWritten = true;
        }

        //BUFFER
        if (m_streamDecoder->isBufferReceived())
        {
            // UG fork: markers are counted from the first sample written, see BrainVisionUG::markerPosition()
            if (!m_hasFirstSample)
            {
                m_firstSampleTime = boxContext.getInputChunkStartTime(0, i);
                m_hasFirstSample  = true;
            }
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

    // UG fork: once, rather than each time, a sample that did not fit the integer format
    if (m_clipped && !m_clipReported)
    {
        this->getLogManager() << Kernel::LogLevel_Warning << "Samples outside the range of the binary format were written as its limits; "
                << "record as IEEE_FLOAT_32 to keep them.\n";
        m_clipReported = true;
    }

    // Make sure the marker file header is written in any case
    if (!m_isVmrkHeaderFileWritten)
    {
        const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        const std::string formated(formatTime(now));

        m_markerFile << "Brain Vision Data Exchange Marker File, Version 1.0" << std::endl << std::endl
                << "[Common Infos]" << std::endl
                << "Codepage=UTF-8" << std::endl  // UG fork: what is written; upstream said ANSI
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
                const uint64_t code = id - OVTK_StimulationId_LabelStart; // extract raw value 1-255

                // UG fork: a marker before any signal waits for the first sample, which it is counted from
                if (m_hasFirstSample) { writeMarker(code, stimSet->getDate(j)); }
                else { m_earlyMarkers.emplace_back(code, stimSet->getDate(j)); }
            }
        }

        boxContext.markInputAsDeprecated(1, i);
    }
    if (m_hasFirstSample)
    {
        for (const auto& [code, date] : m_earlyMarkers) { writeMarker(code, date); }
        m_earlyMarkers.clear();
    }

    return flushFiles();
}

void CBoxAlgorithmBrainampFileWriterUG::writeMarker(const uint64_t code, const uint64_t date)
{
    // UG fork: counted from the first sample written, and rounded. Upstream counted from the scenario's start, so every
    // marker was late by the time the Acquisition client started its signal, and rounded down, which put markers one sample
    // early at rates such as 500 Hz.
    const std::optional<uint64_t> position = BrainVisionUG::markerPosition(date, m_firstSampleTime, m_sampling);
    if (!position)
    {
        this->getLogManager() << Kernel::LogLevel_Warning << "Trigger " << code << " is dated before the first sample recorded, at "
                << CTime(date).toSeconds() << " s; it is left out.\n";
        return;
    }

    // BrainVision Recorder names stimulus markers "S" plus the code right-aligned in 3 characters: "S  1", "S 12", "S123"
    m_markerFile << "Mk" << m_stimulationCounter << "=Stimulus,S" << std::setw(3) << code << "," << *position << ",1,0" << std::endl;
    m_stimulationCounter++;
}

bool CBoxAlgorithmBrainampFileWriterUG::flushFiles()
{
    // UG fork: written out at every step, so that a recording is complete on disk up to the last step, also when the program
    // stops unexpectedly. Upstream kept the .vhdr in memory until the end, and a crash left it empty. A file that can not be
    // written, e.g. on a full disk, stops the recording: upstream went on and left the files short of what they claim.
    m_headerFile.flush();
    m_dataFile.flush();
    m_markerFile.flush();
    for (const auto& [file, name] : { std::pair<std::ofstream*, const std::string*>{&m_headerFile, &m_headerFilename},
                                      {&m_dataFile, &m_dataFilename}, {&m_markerFile, &m_markerFilename} })
    {
        if (!file->good())
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Could not write to " << *name << ", e.g. because the disk is full; "
                    << "the recording stops here.\n";
            return false;
        }
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
            << "Codepage=UTF-8" << std::endl  // UG fork: what is written, also the "µ" below; upstream said ANSI
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
        // UG fork: a comma in a name is coded as "\1"; upstream wrote it as is, which split the entry. Resolution 1 and no
        // unit, which BrainVision reads as microvolt: the TMSi and eego drivers deliver microvolt
        m_headerFile << "Ch" << (i + 1) << "=" << BrainVisionUG::escapeName(m_matrix->getDimensionLabel(0, i)) << ",,1," << std::endl;
    }

    m_headerFile << std::endl;

    // UG fork: the montage of the acquisition server, which the Acquisition client box passed on before the signal's
    // header (ovChannelInfo.h); none when the signal comes from elsewhere, e.g. a file
    m_montage = this->getConfigurationManager().lookUpConfigurationTokenValue(OV_CHANNEL_INFO_MONTAGE_TOKEN).toASCIIString();
    m_positions.clear();
    if (!m_montage.empty()) {
        const std::string positions = this->getConfigurationManager().lookUpConfigurationTokenValue(OV_CHANNEL_INFO_POSITIONS_TOKEN).toASCIIString();
        for (const auto& [name, position] : decodeElectrodePositions(positions)) { m_positions.emplace(name, toBrainVision(position.theta, position.phi)); }
    }

    // UG fork: where the electrodes sat, so that EEGLAB, MNE and BrainVision Analyzer load the layout with the data.
    // Radius 1 on a unit sphere; 0,0,0 for a channel the montage does not place (EOG, ECG, ...)
    size_t nPlaced = 0;
    if (!m_positions.empty())
    {
        m_headerFile << "[Coordinates]" << std::endl
                << "; Each entry: Ch<Channel number>=<Radius>,<Theta>,<Phi>, in degrees; Theta is negative on the left" << std::endl;
        for (size_t i = 0; i < nChannel; ++i)
        {
            const auto it = m_positions.find(m_matrix->getDimensionLabel(0, i));
            if (it == m_positions.end()) { m_headerFile << "Ch" << (i + 1) << "=0,0,0" << std::endl; }
            else
            {
                ++nPlaced;
                m_headerFile << "Ch" << (i + 1) << "=1," << std::defaultfloat << std::setprecision(6) << it->second.theta << ","
                        << it->second.phi << std::endl;
            }
        }
        m_headerFile << std::endl;
        this->getLogManager() << Kernel::LogLevel_Info << "Electrode positions of " << nPlaced << " of the " << nChannel
                << " channels recorded, from the acquisition server's montage " << m_montage << "\n";
    }

    m_headerFile << "[Comment]" << std::endl << std::endl
            << "A m p l i f i e r  S e t u p" << std::endl
            << "============================" << std::endl
            << "Number of channels: " << nChannel << std::endl
            << "Sampling Rate [Hz]: " << m_sampling << std::endl
            << "Interval [µS]: " << std::fixed << std::setprecision(5) << samplingInterval << std::endl
            << std::endl;
    if (!m_montage.empty())
    {
        m_headerFile << "Montage: " << m_montage << " (electrode positions of " << nPlaced << " channels)" << std::endl
                << std::endl;
    }

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
