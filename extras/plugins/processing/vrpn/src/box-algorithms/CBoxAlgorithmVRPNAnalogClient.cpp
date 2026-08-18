///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmVRPNAnalogClient.cpp
/// \brief Classes implementation for the Box VRPN Analog client.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmVRPNAnalogClient.hpp"

namespace OpenViBE {
namespace Plugins {
namespace VRPN {


static void VRPN_CALLBACK VRPNAnalogCB(void* data, const vrpn_ANALOGCB a)
{
    CBoxAlgorithmVRPNAnalogClient* client = static_cast<CBoxAlgorithmVRPNAnalogClient*>(data);
    client->SetAnalog(a.num_channel, a.channel);
}

bool CBoxAlgorithmVRPNAnalogClient::initialize()
{
    m_vrpnAnalogRemote = nullptr;

    m_peripheralName      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_sampling            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_nChannel            = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));
    m_nSamplePerSentBlock = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3));

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_encoder->initialize();

    ip_matrix.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Matrix));
    ip_sampling.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));
    op_buffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    m_firstStart       = true;
    m_lastChunkEndTime = 0;

    m_chunkDuration = CTime(m_sampling, m_nSamplePerSentBlock).time();

    m_lastSamples.resize(m_nChannel);
    m_sampleBuffer.clear();

    return true;
}

bool CBoxAlgorithmVRPNAnalogClient::uninitialize()
{
    if (m_vrpnAnalogRemote) {
        delete m_vrpnAnalogRemote;
        m_vrpnAnalogRemote = nullptr;
    }

    if (m_encoder) {
        m_encoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_encoder);
        m_encoder = nullptr;
    }

    m_sampleBuffer.clear();
    m_lastSamples.clear();

    return true;
}

bool CBoxAlgorithmVRPNAnalogClient::processClock(Kernel::CMessageClock& /*msg*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmVRPNAnalogClient::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    if (m_firstStart) {
        ip_sampling = m_sampling;
        ip_matrix->resize(m_nChannel, m_nSamplePerSentBlock);

        // @TODO do labels ?

        op_buffer = boxContext.getOutputChunk(0);
        m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);
        boxContext.markOutputAsReadyToSend(0, 0, 0);

        m_lastSamples.resize(m_nChannel);
        for (size_t i = 0; i < m_nChannel; ++i) { m_lastSamples[i] = 0; }

        m_firstStart = false;
    }

    if (!m_vrpnAnalogRemote) {
        m_vrpnAnalogRemote = new vrpn_Analog_Remote(m_peripheralName.toASCIIString());
        m_vrpnAnalogRemote->register_change_handler(this, &VRPNAnalogCB);
    }

    m_vrpnAnalogRemote->mainloop();

    const uint64_t time = this->getPlayerContext().getCurrentTime();

    if ((time - m_lastChunkEndTime) >= m_chunkDuration) {
        // Time to send a chunk. Copy our current sample buffer to the output matrix.
        size_t idx      = 0;
        double* oBuffer = ip_matrix->getBuffer();

        while (!m_sampleBuffer.empty()) {
            const std::vector<double>& tmp = m_sampleBuffer.front();

            for (size_t j = 0; j < m_nChannel; ++j) { oBuffer[j * m_nSamplePerSentBlock + idx] = tmp[j]; }

            m_sampleBuffer.pop_front();
            idx++;
        }
        // If the buffer didn't have enough data from callbacks, pad with the last sample
        while (idx < m_nSamplePerSentBlock) {
            for (size_t j = 0; j < m_nChannel; ++j) { oBuffer[j * m_nSamplePerSentBlock + idx] = m_lastSamples[j]; }
            idx++;
        }

        op_buffer = boxContext.getOutputChunk(0);
        m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);
        boxContext.markOutputAsReadyToSend(0, m_lastChunkEndTime, m_lastChunkEndTime + m_chunkDuration);
        m_lastChunkEndTime += m_chunkDuration;
#if defined _DEBUG
        const double diff = CTime(m_lastChunkEndTime).toSeconds() - CTime(time).toSeconds();
        if (std::abs(diff) > 1) { this->getLogManager() << Kernel::LogLevel_Warning << "Time difference detected: " << diff << " secs.\n"; }
#endif
    }

    return true;
}

void CBoxAlgorithmVRPNAnalogClient::SetAnalog(const size_t nAnalog, const double* analog)
{
    // Count how many samples are encoded in pAnalog
    const size_t nSample = nAnalog / m_nChannel;
    for (size_t i = 0; i < nSample; ++i) {
        // Append the sample to the buffer
        for (size_t j = 0; j < m_nChannel; ++j) { m_lastSamples[j] = analog[j * nSample + i]; }
        m_sampleBuffer.push_back(m_lastSamples);

        // Drop the oldest sample if our buffer got full. This means that VRPN is sending data faster than we consume.
        if (m_sampleBuffer.size() > m_nSamplePerSentBlock) { m_sampleBuffer.pop_front(); }
    }
}

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
