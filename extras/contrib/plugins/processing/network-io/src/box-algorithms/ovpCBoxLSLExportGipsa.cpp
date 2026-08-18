#if defined TARGET_HAS_ThirdPartyLSL

#include "ovpCBoxLSLExportGipsa.h"

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {


bool CBoxAlgorithmLSLExportGipsa::initialize()
{
    m_inputChannel1.initialize(this);

    m_streamName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_streamType = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_outlet = nullptr;
    m_stims.clear();

    return true;
}

bool CBoxAlgorithmLSLExportGipsa::uninitialize()
{
    m_inputChannel1.uninitialize();
    m_stims.clear();
    delete m_outlet;
    return true;
}

bool CBoxAlgorithmLSLExportGipsa::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmLSLExportGipsa::process()
{
    if (!m_inputChannel1.isWorking()) {
        m_inputChannel1.waitForSignalHeader();

        if (m_inputChannel1.isWorking()) {
            try {
                //if it fails here then most likely you are using the wrong dll - e.x debug instead of release or vice-versa
                lsl::stream_info info(m_streamName.toASCIIString(), m_streamType.toASCIIString(), int(m_inputChannel1.getNChannels()) + 1,
                                      double(m_inputChannel1.getSamplingRate()), lsl::cf_float32);

                lsl::xml_element channels = info.desc().append_child("channels");

                for (size_t i = 0; i < m_inputChannel1.getNChannels(); ++i) {
                    channels.append_child("channel")
                            .append_child_value("label", m_inputChannel1.getChannelName(i))
                            .append_child_value("type", "EEG")
                            .append_child_value("unit", "microvolts");
                }

                channels.append_child("channel")
                        .append_child_value("label", "Stimulations")
                        .append_child_value("type", "marker");

                if (m_outlet != nullptr) { this->getLogManager() << Kernel::LogLevel_Error << "Possible double initialization!\n"; }

                m_outlet = new lsl::stream_outlet(info); //here the length of the buffered signal can be specified  
            }
            catch (std::exception& e) {
                this->getLogManager() << Kernel::LogLevel_Error << "Could not initialize LSL library: " << e.what() << "\n";
                return false;
            }
        }
    }
    else {
        //stimulations
        for (size_t i = 0; i < m_inputChannel1.getNStimulationBuffers(); ++i) {
            uint64_t tStart, tEnd;
            const CStimulationSet* set = m_inputChannel1.getStimulation(tStart, tEnd, i);

            for (size_t j = 0; j < set->size(); ++j) {
                uint64_t time             = m_inputChannel1.getStartTimestamp() + set->getDate(j);
                const uint64_t identifier = set->getId(j);


                if (m_stims.empty()) {
                    m_stims.push_back(std::pair<uint64_t, uint64_t>(identifier, time));
                    //std::cout<< "added: " << m_stims[m_stims.size()-1].first << " " << m_stims[m_stims.size()-1].second<< "\n";
                }
                else {
                    const auto& last = m_stims[m_stims.size() - 1];
                    if (last.first != identifier && last.second != time) {
                        m_stims.push_back(std::pair<uint64_t, uint64_t>(identifier, time));
                        //std::cout<< "added: " << m_stims[m_stims.size()-1].first << " " << m_stims[m_stims.size()-1].second<< "\n";
                    }
                    else {
                        //std::cout<< "duplicate: " << m_stims[m_stims.size()-1].first << " " << m_stims[m_stims.size()-1].second<< "\n";
                    }
                }
            }
        }

        //signal
        for (size_t i = 0; i < m_inputChannel1.getNSignalBuffers(); ++i) {
            uint64_t tStart, tEnd;
            double* inputBuffer = m_inputChannel1.getSignal(tStart, tEnd, i);

            if (inputBuffer) {
                const size_t samplesPerChannelInput = m_inputChannel1.getNSamples();
                std::vector<std::vector<float>> mychunk(samplesPerChannelInput);

                for (size_t k = 0; k < samplesPerChannelInput; ++k) { mychunk[k] = std::vector<float>(m_inputChannel1.getNChannels() + 1); }

                //Fill a matrix - OpenVibe provides the data ch1 (all values from all samples), ch2(all values from all samples) ... chN, 
                //In the generated chunk every row is a single sample (containing the data from all channels) and every column number is the number of the channel
                for (size_t k = 0; k < m_inputChannel1.getNChannels(); ++k) {
                    for (size_t j = 0; j < samplesPerChannelInput; ++j) {
                        const size_t index = (k * samplesPerChannelInput) + j;
                        mychunk[j][k]      = float(inputBuffer[index]); // @note 64bit->32bit conversion
                    }
                }

                //Process stimulations and add them to the output in a dedicated channel
                std::vector<float> stimChan = std::vector<float>(samplesPerChannelInput);

                auto it = m_stims.begin();
                while (it != m_stims.end()) {
                    const auto& current = *it;

                    if (!(current.second >= tStart && current.second <= tEnd)) {
                        // not in current time range, do not send now.
                        ++it;
                        continue;
                    }
                    const uint64_t posCurrent = CTime(current.second).toSampleCount(m_inputChannel1.getSamplingRate());
                    const uint64_t posStart   = CTime(tStart).toSampleCount(m_inputChannel1.getSamplingRate());
                    //uint64_t posEnd = CTime(tStart).toSampleCount(m_inputChannel1.getSamplingRate());

                    int pos = int(posCurrent) - int(posStart);
                    if (pos < 0) { pos = 0; }                                       //fix position
                    if (pos == int(stimChan.size())) { pos = int(stimChan.size() - 1); }    //fix position

                    if (pos >= 0 && pos < int(stimChan.size())) {
                        stimChan[pos] = float(current.first);
                        //std::cout<< "pos relative: " << pos << " value: " << stim_chan[pos] << " time:" << CTime(current.second).toSeconds()<< "\n";
                    }
                    else { this->getLogManager() << Kernel::LogLevel_Warning << "Bad stimulation position: " << pos << "stim code: " << current.first << "\n"; }

                    // processed, erase
                    it = m_stims.erase(it);
                }

                //add the stim channel at the end of the matrix
                const size_t k = m_inputChannel1.getNChannels();
                for (size_t j = 0; j < samplesPerChannelInput; ++j) { mychunk[j][k] = stimChan[j]; }

                //send all channels
                m_outlet->push_chunk(mychunk);
            }
        }
    }

    return true;
}

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE

#endif
