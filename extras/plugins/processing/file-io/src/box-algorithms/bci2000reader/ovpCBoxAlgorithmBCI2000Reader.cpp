#include "ovpCBoxAlgorithmBCI2000Reader.h"

#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

bool CBoxAlgorithmBCI2000Reader::initialize()
{
    const CString filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_helper               = new BCI2000::CBCI2000ReaderHelper(filename);
    if (!m_helper->isGood())
    {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not open file [" << filename << "]\n";
        m_helper = nullptr;
        return false;
    }
    std::stringstream ss;
    m_helper->printInfo(ss);
    this->getLogManager() << Kernel::LogLevel_Trace << "Metadata from [" << filename << "] :\n" << ss.str() << "\n";

    m_headerSent       = false;
    m_nChannel         = m_helper->getChannels();
    m_nSamplePerBuffer = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
    m_samplesSent      = 0;

    if (m_nSamplePerBuffer == 0)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "SampleCountPerBuffer is 0, this will not work\n";
        return false;
    }

    m_buffer.resize(m_nChannel * m_nSamplePerBuffer);
    m_states.resize(m_helper->getStateVectorSize() * m_nSamplePerBuffer);
    m_rate = size_t(m_helper->getRate());
    if (int(m_rate) == -1)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Sampling rate could not be extracted from the file.\n";
        return false;
    }
    if (m_rate == 0)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Sampling rate of 0 is not supported.\n";
        return false;
    }

    m_signalEncoder.initialize(*this, 0);
    m_oSignalMatrix = m_signalEncoder.getInputMatrix();
    m_oSignalMatrix->resize(m_nChannel, m_nSamplePerBuffer);
    for (size_t i = 0; i < m_nChannel; ++i) { m_oSignalMatrix->setDimensionLabel(0, i, m_helper->getChannelName(i)); }

    m_stateEncoder.initialize(*this, 1);
    m_oStateMatrix = m_stateEncoder.getInputMatrix();
    m_oStateMatrix->resize(m_helper->getStateVectorSize(), m_nSamplePerBuffer);
    for (size_t i = 0; i < m_helper->getStateVectorSize(); ++i) { m_oStateMatrix->setDimensionLabel(0, i, m_helper->getStateName(i)); }
    m_signalEncoder.getInputSamplingRate() = m_rate;
    m_stateEncoder.getInputSamplingRate()  = m_rate;
    for (size_t i = 0; i < m_helper->getStateVectorSize(); ++i)
    {
        this->getLogManager() << Kernel::LogLevel_Trace << "BCI2000 state var " << i << " is : " << m_helper->getStateName(i) << "\n";
    }
    return true;
}

bool CBoxAlgorithmBCI2000Reader::uninitialize()
{
    delete m_helper;
    m_buffer.clear();
    m_states.clear();
    m_signalEncoder.uninitialize();
    m_stateEncoder.uninitialize();

    return true;
}

bool CBoxAlgorithmBCI2000Reader::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (m_helper->getSamplesLeft() > 0)
    {
        getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
        return true;
    }

    return false;
}

uint64_t CBoxAlgorithmBCI2000Reader::getClockFrequency()
{
    // Intentional parameter swap to get the frequency
    return CTime(m_nSamplePerBuffer, m_rate).time();
}

void CBoxAlgorithmBCI2000Reader::sendHeader()
{
    m_signalEncoder.encodeHeader();
    m_stateEncoder.encodeHeader();
    m_headerSent = true;
    getDynamicBoxContext().markOutputAsReadyToSend(0, 0, 0);
    getDynamicBoxContext().markOutputAsReadyToSend(1, 0, 0);
}

bool CBoxAlgorithmBCI2000Reader::process()
{
    if (!m_headerSent) { sendHeader(); }

    //prepare data
    const int nRead = m_helper->readSamples(m_buffer.data(), m_states.data(), m_nSamplePerBuffer);
    if (nRead > 0)
    {
        // padding. TODO: is it necessary ? or even dangerous ?
        for (uint32_t i = nRead; i < m_nSamplePerBuffer; ++i) { for (uint32_t j = 0; j < m_nChannel; ++j) { m_buffer[i * m_nChannel + j] = 0.0; } }
        // transpose (yeah, I know... ugly)
        for (uint32_t i = 0; i < m_nSamplePerBuffer; ++i)
        {
            for (uint32_t j = 0; j < m_nChannel; ++j) { m_oSignalMatrix->getBuffer()[j * m_nSamplePerBuffer + i] = m_buffer[i * m_nChannel + j]; }
        }
        m_signalEncoder.encodeBuffer();
        const uint64_t start = CTime(m_rate, m_samplesSent).time();
        const uint64_t end   = CTime(m_rate, m_samplesSent + m_nSamplePerBuffer).time();
        m_samplesSent += nRead;
        if (m_helper->getSamplesLeft() == 0) { m_signalEncoder.encodeEnd(); }

        getDynamicBoxContext().markOutputAsReadyToSend(0, start, end);

        // padding. TODO: is it necessary ? or even dangerous ?
        for (size_t i = nRead; i < m_nSamplePerBuffer; ++i)
        {
            for (size_t j = 0; j < m_helper->getStateVectorSize(); ++j) { m_states[i * m_helper->getStateVectorSize() + j] = 0; }
        }
        // transpose (yeah, I know... ugly)
        for (size_t i = 0; i < m_nSamplePerBuffer; ++i)
        {
            for (size_t j = 0; j < m_helper->getStateVectorSize(); ++j)
            {
                m_oStateMatrix->getBuffer()[j * m_nSamplePerBuffer + i] = m_states[i * m_helper->getStateVectorSize() + j];
            }
        }
        m_stateEncoder.encodeBuffer();

        if (m_helper->getSamplesLeft() == 0) { m_signalEncoder.encodeEnd(); }

        getDynamicBoxContext().markOutputAsReadyToSend(1, start, end);
    }
    else
    {
        this->getLogManager() << Kernel::LogLevel_Error << "An error occurred while trying to get new samples from file. The file may be corrupted.\n";
        return false;
    }
    return true;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
