#include "ovtkCSignalTrial.hpp"

namespace OpenViBE {
namespace Toolkit {

bool CSignalTrial::setSamplingRate(const size_t sampling)
{
    m_sampling = sampling;
    return m_sampling != 0;
}

bool CSignalTrial::setChannelCount(const size_t count)
{
    size_t i;
    for (i = 0; i < count; ++i) { if (m_channelSamples.find(i) == m_channelSamples.end()) { m_channelSamples[i] = new double[m_nSampleReserved]; } }
    for (i = count; i < m_nChannel; ++i)
    {
        delete [] m_channelSamples[i];
        m_channelSamples.erase(m_channelSamples.find(i));
    }

    m_nChannel = count;
    m_nSample  = 0;
    m_channelNames.clear();
    return m_nChannel != 0;
}

bool CSignalTrial::setChannelName(const size_t index, const char* name)
{
    if (index < m_nChannel)
    {
        m_channelNames[index] = name;
        return true;
    }
    return false;
}

bool CSignalTrial::setLabelIdentifier(const CIdentifier& labelID)
{
    m_labelID = labelID;
    return true;
}

bool CSignalTrial::setSampleCount(const size_t count, const bool preserve)
{
    const size_t nSampleRounding = 0x00000fff;

    if (count > m_nSampleReserved)
    {
        const size_t nSampleReserved = (count + nSampleRounding + 1) & (~nSampleRounding);
        for (auto it = m_channelSamples.begin(); it != m_channelSamples.end(); ++it)
        {
            double* sample = new double[nSampleReserved];
            if (preserve) { memcpy(sample, it->second, (count < m_nSample ? count : m_nSample) * sizeof(double)); }
            delete [] it->second;
            it->second = sample;
        }
        m_nSampleReserved = nSampleReserved;
    }
    m_nSample = count;
    return true;
}

// ________________________________________________________________________________________________________________
//

const char* CSignalTrial::getChannelName(const size_t index) const
{
    const auto it = m_channelNames.find(index);
    if (it != m_channelNames.end()) { return it->second.c_str(); }
    return "";
}

double* CSignalTrial::getChannelSampleBuffer(const size_t index) const
{
    const auto it = m_channelSamples.find(index);
    if (it != m_channelSamples.end()) { return it->second; }
    return nullptr;
}

// ________________________________________________________________________________________________________________
//

ISignalTrial* createSignalTrial() { return new CSignalTrial(); }
void releaseSignalTrial(ISignalTrial* trial) { delete trial; }

}  // namespace Toolkit
}  // namespace OpenViBE
