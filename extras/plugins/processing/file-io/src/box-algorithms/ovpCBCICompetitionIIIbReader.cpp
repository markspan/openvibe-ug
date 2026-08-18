#include "ovpCBCICompetitionIIIbReader.h"

#include <iostream>
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

#define BCICompetitionIIIbReader_UndefinedClass 0xFFFFFFFFFFLL

bool CBCICompetitionIIIbReader::initialize()
{
    m_signalEncoder.initialize(*this, 0);
    m_stimEncoder.initialize(*this, 1);

    // Parses box settings to find filename
    const CString file = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    //opens the file
    if (file) { m_file.open(file); }
    if (!m_file.good())
    {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << file << "]\n";
        return false;
    }

    m_file.seekg(0, std::ios::end);
    m_fileSize = size_t(m_file.tellg());
    m_file.seekg(0, std::ios::beg);

    m_trialLength     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10);
    m_cueDisplayStart = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);
    m_feedbackStart   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 12);

    readTriggers();
    readLabels();
    readArtifacts();
    readTrueLabels();

    // Gets the size of output buffers
    m_samplesPerBuffer = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    //Offline/Online
    const bool offline = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
    if (!offline)
    {
        //computes clock frequency
        if (m_samplesPerBuffer <= m_sampling)
        {
            if (m_sampling % m_samplesPerBuffer != 0)
            {
                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning <<
                        "The sampling rate isn't a multiple of the buffer size\n" <<
                        "Please consider adjusting the BCI Competition IIIb reader settings to correct this!\n";
            }

            if (m_samplesPerBuffer == 0)
            {
                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error <<
                        "SamplesPerBuffer is 0, this will not work\n";
                return false;
            }

            // Intentional parameter swap to get the frequency
            m_clockFrequency = CTime(m_samplesPerBuffer, m_sampling).time();
        }
    }

    //Test/Training
    m_keepTrainingSamples = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
    m_keepTestSamples     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
    m_keepArtifactSamples = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9);

    writeSignalInformation();

    m_signalEncoder.encodeHeader();
    m_stimEncoder.encodeHeader();

    getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
    getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(1, 0, 0);

    m_buffer = m_signalEncoder.getInputMatrix();

    return true;
}

bool CBCICompetitionIIIbReader::uninitialize()
{
    m_stimEncoder.uninitialize();
    m_signalEncoder.uninitialize();
    if (m_file) { m_file.close(); }
    return true;
}

bool CBCICompetitionIIIbReader::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (!m_endOfFile) { getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }
    return true;
}

bool CBCICompetitionIIIbReader::process()
{
    if (m_errorOccurred) { return false; }

    Kernel::IBoxIO* boxIO = getBoxAlgorithmContext()->getDynamicBoxContext();

    //reading signal
    //reset vector
    double* buffer = m_buffer->getBuffer();
    for (uint32_t i = 0; i < m_buffer->getBufferElementCount(); ++i) { buffer[i] = 0; }

    std::istringstream ss;
    std::string line;
    double sample;

    uint32_t count = 0;
    for (; count < m_samplesPerBuffer && !m_endOfFile; ++count)
    {
        if (!getline(m_file, line)) { m_endOfFile = true; }

        ss.clear();
        ss.str(line);

        ss >> sample;
        if (std::isnan(sample)) { buffer[count] = (count != 0) ? buffer[count - 1] : 0.0; }
        else { buffer[count] = sample; }

        ss >> sample;
        if (std::isnan(sample)) { buffer[count + m_samplesPerBuffer] = (count != 0) ? buffer[count + m_samplesPerBuffer - 1] : 0.0; }
        else { buffer[count + m_samplesPerBuffer] = sample; }
    }

    m_nSentSample += count;

    //A signal matrix is ready to be output
    m_signalEncoder.encodeBuffer();

    const uint64_t start = CTime(m_sampling, uint64_t(m_nSentSample - count)).time();
    const uint64_t end   = CTime(m_sampling, uint64_t(m_nSentSample)).time();

    boxIO->markOutputAsReadyToSend(0, start, end);
    //////

    //Stimulations
    std::vector<std::pair<uint64_t, uint64_t>> events;
    bool changed = true;

    while (changed)
    {
        changed = false;

        const bool keepCurrentTrial =
                ((m_artifacts[m_currentTrial] && m_keepArtifactSamples) || !m_artifacts[m_currentTrial]) &&
                ((m_classLabels[m_currentTrial] == BCICompetitionIIIbReader_UndefinedClass && m_keepTestSamples) ||
                 (m_classLabels[m_currentTrial] != BCICompetitionIIIbReader_UndefinedClass && m_keepTrainingSamples));

        if (m_triggerTimes[m_currentTrial] > (m_nSentSample - count) &&
            m_triggerTimes[m_currentTrial] <= m_nSentSample
        )
        {
            if (keepCurrentTrial)
            {
                //start of trial
                events.push_back(std::pair<uint64_t, uint64_t>(0x300, m_triggerTimes[m_currentTrial]));
                //display cross
                events.push_back(std::pair<uint64_t, uint64_t>(0x312, m_triggerTimes[m_currentTrial]));
            }
        }

        //send CUE stimulation
        if (m_cueDisplayStarts[m_currentTrial] > (m_nSentSample - count) &&
            m_cueDisplayStarts[m_currentTrial] <= m_nSentSample
        )
        {
            if (keepCurrentTrial)
            {
                if (m_classLabels[m_currentTrial] != BCICompetitionIIIbReader_UndefinedClass)
                {
                    //send class label
                    events.push_back(std::pair<uint64_t, uint64_t>(0x300 + m_classLabels[m_currentTrial], m_cueDisplayStarts[m_currentTrial]));
                }
                else
                {
                    //send true label
                    events.push_back(std::pair<uint64_t, uint64_t>(0x300 + m_trueLabels[m_currentTrial], m_cueDisplayStarts[m_currentTrial]));
                }
            }
        }

        //send feedback start stimulation
        if (m_feedbackStarts[m_currentTrial] > (m_nSentSample - count) && m_feedbackStarts[m_currentTrial] <= m_nSentSample)
        {
            if (keepCurrentTrial) { events.push_back(std::pair<uint64_t, uint64_t>(0x30D, m_feedbackStarts[m_currentTrial])); }
        }

        //send end of trial stimulation
        if (m_endOfTrials[m_currentTrial] > (m_nSentSample - count) && m_endOfTrials[m_currentTrial] <= m_nSentSample
        )
        {
            if (keepCurrentTrial) { events.push_back(std::pair<uint64_t, uint64_t>(0x320, m_endOfTrials[m_currentTrial])); }
            m_currentTrial++;
            changed = true;
        }
    }

    if (!events.empty() || m_endOfFile)
    {
        CStimulationSet* stimSet = m_stimEncoder.getInputStimulationSet();

        stimSet->resize(events.size() + ((m_endOfFile) ? 1 : 0));


        for (size_t j = 0; j < events.size(); ++j)
        {
            //compute date
            const uint64_t date = CTime(m_sampling, events[j].second).time();
            stimSet->insert(j, events[j].first, date, 0);
        }

        //add the ending stim
        if (m_endOfFile)
        {
            //compute date
            const uint64_t date = CTime(m_sampling, m_nSentSample).time();
            stimSet->insert(events.size(), 0x3FF, date, 0);
        }

        m_stimEncoder.encodeBuffer();

        boxIO->markOutputAsReadyToSend(1, start, end);
    }

    return true;
}

void CBCICompetitionIIIbReader::writeSignalInformation()
{
    m_signalEncoder.getInputSamplingRate() = 125;
    m_signalEncoder.getInputMatrix()->resize(2, m_samplesPerBuffer);
    m_signalEncoder.getInputMatrix()->setDimensionLabel(0, 0, "+C3a-C3p");
    m_signalEncoder.getInputMatrix()->setDimensionLabel(0, 1, "+C4a-C4p");
}


void CBCICompetitionIIIbReader::readTriggers()
{
    const CString str = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    std::ifstream file;
    if (str) { file.open(str); }

    std::string line;
    std::istringstream ss;
    uint64_t value;
    while (getline(file, line))
    {
        ss.clear();
        ss.str(line);
        ss >> value;

        m_triggerTimes.push_back(value);
        m_cueDisplayStarts.push_back(value + uint64_t(floor(m_sampling * m_cueDisplayStart)));
        m_feedbackStarts.push_back(value + uint64_t(floor(m_sampling * m_feedbackStart)));
        m_endOfTrials.push_back(value + uint64_t(floor(m_sampling * m_trialLength)));
    }
    file.close();
}

void CBCICompetitionIIIbReader::readLabels()
{
    const CString str = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    std::ifstream file;
    if (str) { file.open(str); }

    std::string line;
    std::istringstream ss;
    uint64_t value;
    while (getline(file, line))
    {
        if (line.compare(0, 3, "NaN", 0, 3) == 0) { m_classLabels.push_back(BCICompetitionIIIbReader_UndefinedClass); }
        else
        {
            ss.clear();
            ss.str(line);
            ss >> value;
            m_classLabels.push_back(value);
        }
    }
    file.close();
}

void CBCICompetitionIIIbReader::readArtifacts()
{
    const CString str = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    std::ifstream file;
    if (str) { file.open(str); }

    std::string line;
    std::istringstream ss;
    uint64_t value;
    while (getline(file, line))
    {
        ss.clear();
        ss.str(line);
        ss >> value;
        m_artifacts.push_back(value == 1);
    }
    file.close();
}

void CBCICompetitionIIIbReader::readTrueLabels()
{
    const CString str = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    std::ifstream file;
    if (str) { file.open(str); }

    std::string line;
    std::istringstream ss;
    uint64_t value;
    while (getline(file, line))
    {
        ss.clear();
        ss.str(line);
        ss >> value;
        m_trueLabels.push_back(value);
    }
    file.close();
}

}  // namespace  FileIO
}  // namespace Plugins
}  // namespace OpenViBE
