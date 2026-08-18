#include "ovasCDriverCtfVsmMeg.h"
#include "../ovasCConfigurationNetworkBuilder.h"

#include <system/ovCTime.h>

#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverCtfVsmMeg::CDriverCtfVsmMeg(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_CtfVsmMeg", m_driverCtx.getConfigurationManager())
{
    m_settings.add("Header", &m_header);
    m_settings.add("ServerHostName", &m_sServerHostName);
    m_settings.add("ServerHostPort", &m_serverHostPort);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverCtfVsmMeg::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    // Builds up client connection
    m_connectionClient = Socket::createConnectionClient();

    // Tries to connect to server
    m_connectionClient->connect(m_sServerHostName, m_serverHostPort);

    // Checks if connection is correctly established
    if (!m_connectionClient->isConnected()) {
        // In case it is not, try to reconnect
        m_connectionClient->connect(m_sServerHostName, m_serverHostPort);
    }

    if (!m_connectionClient->isConnected()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Connection problem! Tried 2 times without success! :(\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Verify port number and/or Hostname...\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Client connected\n";
    // Initialize vars for reception
    uint32_t received = 0;
    m_pStructHeader   = (char*)&m_structHeader;
    // Receive Header
    while (received < sizeof(m_structHeader)) {
        const uint32_t reqLength = sizeof(m_structHeader) - received;
        const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_pStructHeader, reqLength);

        received += res;
        m_pStructHeader += res;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Receiving Header....\n";
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Header received\n";

    // Save Header info into m_header
    //m_header.setExperimentID();
    //m_header.setExperimentDate();

    //m_header.setSubjectId();
    //m_header.setSubjectName();
    m_header.setSubjectAge(m_structHeader.subjectAge);
    //m_header.setSubjectGender();

    //m_header.setLaboratoryId();
    //m_header.setLaboratoryName();

    //m_header.setTechnicianId();
    //m_header.setTechnicianName();

    m_header.setChannelCount(m_structHeader.numberOfChannels);
    for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
        m_header.setChannelName(i, m_structHeader.channelLabel[i]);
        m_header.setChannelGain(i, (m_structHeader.qGain[i] * m_structHeader.properGain[i]));
    }

    m_header.setSamplingFrequency(uint32_t(m_structHeader.samplingRate));

    //m_header.setSampleCount(nSamplePerSentBlock);

    m_sample = new float[m_header.getChannelCount() * nSamplePerSentBlock];

    if (!m_sample) {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }

    m_callback = &callback;

    m_nSamplePerSentBlock = nSamplePerSentBlock;

    m_indexIn     = 0;
    m_indexOut    = 0;
    m_socketFlag  = 1976;
    m_buffDataIdx = 0;

    return true;
}

bool CDriverCtfVsmMeg::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_buffDataIdx == 0) {
        // Initialize var to send flag
        uint32_t sent  = 0;
        uint32_t* flag = static_cast<uint32_t*>(&m_socketFlag);

        // Send flag to server
        while (sent < sizeof(m_socketFlag)) {
            const uint32_t reqLength = sizeof(m_socketFlag) - sent;
            const uint32_t res       = m_connectionClient->sendBuffer(static_cast<uint32_t*>(flag), reqLength);
            sent += res;
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " > Sending flag to start....\n";
        }
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Flag sent\n";
    }

    return true;
}

bool CDriverCtfVsmMeg::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    // Initialize var to receive buffer of data
    uint32_t received = 0;
    m_pStructBuffData = (char*)&m_structBuffData;

    // Read a first buffer of data
    m_buffDataIdx++;
    while (received < sizeof(m_structBuffData)) {
        const uint32_t reqLength = sizeof(m_structBuffData) - received;
        const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_pStructBuffData, reqLength);
        received += res;
        m_pStructBuffData += res;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " > Receiving buffer of data....nb: " << m_buffDataIdx << "\n";
    }

    // if input flow is equal to output one
    if (m_nSamplePerSentBlock == uint32_t(m_structBuffData.nbSamplesPerChanPerBlock)) {
        for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
            for (uint32_t j = 0; j < m_nSamplePerSentBlock; ++j) {
                if ((m_structHeader.channelTypeIndex[i] == 0) | (m_structHeader.channelTypeIndex[i] == 1) | (m_structHeader.channelTypeIndex[i] == 5)) {
                    m_sample[j + i * m_nSamplePerSentBlock] =
                            float(m_structBuffData.data[m_header.getChannelCount() * j + i]) * 1e15F / m_header.getChannelGain(i);
                }
                else {
                    m_sample[j + i * m_nSamplePerSentBlock] =
                            float(m_structBuffData.data[m_header.getChannelCount() * j + i]) / m_header.getChannelGain(i);
                }
            }
        }
        // send data
        m_callback->setSamples(m_sample);
    }
    else {
        // if output flow is bigger
        if (m_nSamplePerSentBlock > uint32_t(m_structBuffData.nbSamplesPerChanPerBlock)) {
            while (m_indexOut + uint32_t(m_structBuffData.nbSamplesPerChanPerBlock) < m_nSamplePerSentBlock) {
                for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
                    for (uint32_t j = 0; j < uint32_t(m_structBuffData.nbSamplesPerChanPerBlock); ++j) {
                        if ((m_structHeader.channelTypeIndex[i] == 0) | (m_structHeader.channelTypeIndex[i] == 1) | (m_structHeader.channelTypeIndex[i] == 5)) {
                            m_sample[m_indexOut + j + i * m_nSamplePerSentBlock] =
                                    float(m_structBuffData.data[m_header.getChannelCount() * j + i]) * 1e15F / m_header.getChannelGain(i);
                        }
                        else {
                            m_sample[m_indexOut + j + i * m_nSamplePerSentBlock] =
                                    float(m_structBuffData.data[m_header.getChannelCount() * j + i]) / m_header.getChannelGain(i);
                        }
                    }
                }

                m_indexOut = m_indexOut + uint32_t(m_structBuffData.nbSamplesPerChanPerBlock);

                // Ask for another buffer
                received          = 0;
                m_pStructBuffData = (char*)&m_structBuffData;

                while (received < sizeof(m_structBuffData)) {
                    const uint32_t reqLength = sizeof(m_structBuffData) - received;
                    const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_pStructBuffData, reqLength);

                    received += res;
                    m_pStructBuffData += res;
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << " > Receiving buffer of data....nb: " << m_buffDataIdx << "\n";
                }
            }

            // Finishing filling up
            for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
                for (uint32_t j = 0; j < m_nSamplePerSentBlock - m_indexOut; ++j) {
                    if ((m_structHeader.channelTypeIndex[i] == 0) | (m_structHeader.channelTypeIndex[i] == 1) | (m_structHeader.channelTypeIndex[i] == 5)) {
                        m_sample[m_indexOut + j + i * m_nSamplePerSentBlock] =
                                float(m_structBuffData.data[m_header.getChannelCount() * j + i]) * 1e15F / m_header.getChannelGain(i);
                    }
                    else {
                        m_sample[m_indexOut + j + i * m_nSamplePerSentBlock] =
                                float(m_structBuffData.data[m_header.getChannelCount() * j + i]) / m_header.getChannelGain(i);
                    }
                }
            }

            // send data
            m_callback->setSamples(m_sample);

            // Reset index out because new output
            m_indexIn  = m_nSamplePerSentBlock - m_indexOut;
            m_indexOut = 0;

            // Save the rest of the buffer input
            for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
                for (uint32_t j = 0; j < uint32_t(m_structBuffData.nbSamplesPerChanPerBlock) - m_indexIn; ++j) {
                    if ((m_structHeader.channelTypeIndex[i] == 0) | (m_structHeader.channelTypeIndex[i] == 1) | (m_structHeader.channelTypeIndex[i] == 5)) {
                        m_sample[j + i * m_nSamplePerSentBlock] =
                                float(m_structBuffData.data[m_indexIn * m_header.getChannelCount() + m_header.getChannelCount() * j + i]) * 1e15F
                                / m_header.getChannelGain(i);
                    }
                    else {
                        m_sample[j + i * m_nSamplePerSentBlock] =
                                float(m_structBuffData.data[m_indexIn * m_header.getChannelCount() + m_header.getChannelCount() * j + i])
                                / m_header.getChannelGain(i);
                    }
                }
            }

            m_indexOut = uint32_t(m_structBuffData.nbSamplesPerChanPerBlock) - m_indexIn;
            m_indexIn  = 0;
        }
        else {
            // if output flow is smaller
            while (m_indexIn + (m_nSamplePerSentBlock - m_indexOut) < uint32_t(m_structBuffData.nbSamplesPerChanPerBlock)) {
                for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
                    for (uint32_t j = 0; j < m_nSamplePerSentBlock - m_indexOut; ++j) {
                        if ((m_structHeader.channelTypeIndex[i] == 0) | (m_structHeader.channelTypeIndex[i] == 1) | (m_structHeader.channelTypeIndex[i] == 5)) {
                            m_sample[j + m_indexOut + i * m_nSamplePerSentBlock] =
                                    float(m_structBuffData.data[m_header.getChannelCount() * m_indexIn + m_header.getChannelCount() * j + i]) * 1e15F
                                    / m_header.getChannelGain(i);
                        }
                        else {
                            m_sample[j + m_indexOut + i * m_nSamplePerSentBlock] =
                                    float(m_structBuffData.data[m_header.getChannelCount() * m_indexIn + m_header.getChannelCount() * j + i])
                                    / m_header.getChannelGain(i);
                        }
                    }
                }
                //send data
                m_callback->setSamples(m_sample);

                m_indexIn  = m_indexIn + (m_nSamplePerSentBlock - m_indexOut);
                m_indexOut = 0;
            }

            // save the rest of buff data
            for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
                for (uint32_t j = 0; j < uint32_t(m_structBuffData.nbSamplesPerChanPerBlock) - m_indexIn; ++j) {
                    if ((m_structHeader.channelTypeIndex[i] == 0) | (m_structHeader.channelTypeIndex[i] == 1) | (m_structHeader.channelTypeIndex[i] == 5)) {
                        m_sample[j + m_indexOut + i * m_nSamplePerSentBlock] =
                                float(m_structBuffData.data[m_header.getChannelCount() * m_indexIn + m_header.getChannelCount() * j + i]) * 1e15F
                                / m_header.getChannelGain(i);
                    }
                    else {
                        m_sample[j + m_indexOut + i * m_nSamplePerSentBlock] =
                                float(m_structBuffData.data[m_header.getChannelCount() * m_indexIn + m_header.getChannelCount() * j + i])
                                / m_header.getChannelGain(i);
                    }
                }
            }

            m_indexOut = uint32_t(m_structBuffData.nbSamplesPerChanPerBlock) - m_indexIn;
            m_indexIn  = 0;
        }
    }

    return true;
}

bool CDriverCtfVsmMeg::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Connection stopped\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverCtfVsmMeg::uninitialize()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    delete [] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    // Cleans up client connection
    m_connectionClient->close();
    m_connectionClient->release();
    m_connectionClient = nullptr;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Client disconnected\n";

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverCtfVsmMeg::configure()
{
    CConfigurationNetworkBuilder config(Directories::getDataDir() + "/applications/acquisition-server/interface-CtfVsm-Meg.ui");

    config.setHostName(m_sServerHostName);
    config.setHostPort(m_serverHostPort);

    if (config.configure(m_header)) {
        m_sServerHostName = config.getHostName();
        m_serverHostPort  = config.getHostPort();

        m_settings.save();

        return true;
    }

    return false;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
