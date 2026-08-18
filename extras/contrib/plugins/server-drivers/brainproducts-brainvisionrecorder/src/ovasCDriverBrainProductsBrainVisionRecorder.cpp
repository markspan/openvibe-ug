#include "ovasCDriverBrainProductsBrainVisionRecorder.h"
#include "../ovasCConfigurationNetworkBuilder.h"

#include <system/ovCTime.h>

#include <iostream>

#include <cstdlib>
#include <cstring>

namespace OpenViBE {
namespace AcquisitionServer {
CDriverBrainProductsBrainVisionRecorder::CDriverBrainProductsBrainVisionRecorder(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_BrainVisionRecorder", m_driverCtx.getConfigurationManager())
{
    m_settings.add("Header", &m_header);
    m_settings.add("ServerHostName", &m_sServerHostName);
    m_settings.add("ServerHostPort", &m_serverHostPort);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsBrainVisionRecorder::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    // Initialize GUID value
    M_DEFINE_GUID(GUID_RDAHeader, 1129858446, 51606, 19590, uint8_t(175), uint8_t(74), uint8_t(152), uint8_t(187), uint8_t(246), uint8_t(201), uint8_t(20),
                  uint8_t(80));

    // Builds up client connection
    m_connectionClient = Socket::createConnectionClient();

    // Tries to connect to server
    m_connectionClient->connect(m_sServerHostName, m_serverHostPort);

    // Checks if connection is correctly established
    if (!m_connectionClient->isConnected())
    {
        // In case it is not, try to reconnect
        m_connectionClient->connect(m_sServerHostName, m_serverHostPort);
    }

    if (!m_connectionClient->isConnected())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Connection problem! Tried 2 times without success! :(\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Verify port number and/or Hostname...\n";
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Client connected\n";

    // Initialize vars for reception
    m_msgHeader = nullptr;
    RDA_MessageHeader msgHeader;
    m_msgHeaderStr = (char*)&msgHeader;

    uint32_t received = 0;
    uint32_t size     = sizeof(msgHeader);

    // Receive Header
    while (received < size)
    {
        const uint32_t reqLength = size - received;
        const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_msgHeaderStr, reqLength);

        received += res;
        m_msgHeaderStr += res;
    }

    // Check for correct header GUID.
    if (!M_COMPARE_GUID(msgHeader.guid, GUID_RDAHeader))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "GUID received is not correct!\n";
        return false;
    }

    // Check for correct header nType
    if (msgHeader.nType != 1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "First Message received is not an header!\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Try to reconnect....\n";
        return false;
    }

    // Check if we need a larger buffer, allocate
    if (!reallocateHeaderBuffer(msgHeader.nSize)) { return false; }

    // Retrieve rest of data
    memcpy(*(&m_msgHeader), &msgHeader, sizeof(msgHeader));
    m_msgHeaderStr = (char*)(*(&m_msgHeader)) + sizeof(msgHeader);
    received       = 0;
    size           = msgHeader.nSize - sizeof(msgHeader);
    while (received < size)
    {
        const uint32_t reqLength = size - received;
        const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_msgHeaderStr, reqLength);

        received += res;
        m_msgHeaderStr += res;
    }

    m_msgStart = (RDA_MessageStart*)m_msgHeader; // pHeader will retain the pointer ownership

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Header received\n";

    // Save Header info into m_header
    //m_header.setExperimentID();
    //m_header.setExperimentDate();

    //m_header.setSubjectId();
    //m_header.setSubjectName();
    //m_header.setSubjectAge(m_structHeader.subjectAge);
    //m_header.setSubjectGender();

    //m_header.setLaboratoryId();
    //m_header.setLaboratoryName();

    //m_header.setTechnicianId();
    //m_header.setTechnicianName();

    m_header.setChannelCount((uint32_t)m_msgStart->nChannels);

    char* channelNames = (char*)m_msgStart->dResolutions + (m_msgStart->nChannels * sizeof(m_msgStart->dResolutions[0]));
    for (size_t i = 0; i < m_header.getChannelCount(); ++i)
    {
        m_header.setChannelName(i, channelNames);
        m_header.setChannelGain(i, float((m_msgStart->dResolutions[i])));
        m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
        channelNames += strlen(channelNames) + 1;
    }

    m_header.setSamplingFrequency(uint32_t(1000000 / m_msgStart->dSamplingInterval)); //dSamplingInterval in microseconds

    m_nSamplePerSentBlock = nSamplePerSentBlock;

    m_callback = &callback;

    m_indexIn     = 0;
    m_indexOut    = 0;
    m_buffDataIdx = 0;

    m_nMarker  = 0;
    m_nMarkers = 0;

    return true;
}

bool CDriverBrainProductsBrainVisionRecorder::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverBrainProductsBrainVisionRecorder::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    M_DEFINE_GUID(GUID_RDAHeader,
                  1129858446, 51606, 19590, uint8_t(175), uint8_t(74), uint8_t(152), uint8_t(187), uint8_t(246), uint8_t(201), uint8_t(20), uint8_t(80)
    );

    // Initialize var to receive buffer of data
    m_msgHeader = nullptr;
    RDA_MessageHeader msgHeader;
    m_msgHeaderStr    = (char*)&msgHeader;
    uint32_t received = 0;
    uint32_t size     = sizeof(msgHeader);

    // Receive Header
    while (received < size)
    {
        const uint32_t reqLength = size - received;
        const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_msgHeaderStr, reqLength);
        received += res;
        m_msgHeaderStr += res;
    }

    // Check for correct header nType
    if (msgHeader.nType == 1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Message received is a header!\n";
        return false;
    }
    if (msgHeader.nType == 3)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Message received is a STOP!\n";
        return false;
    }
    if (msgHeader.nType != 4) { return true; }

    // Check for correct header GUID.
    if (!M_COMPARE_GUID(msgHeader.guid, GUID_RDAHeader))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "GUID received is not correct!\n";
        return false;
    }

    // Check if we need a larger buffer, allocate
    if (!reallocateHeaderBuffer(msgHeader.nSize)) { return false; }

    // Retrieve rest of block.
    memcpy(*(&m_msgHeader), &msgHeader, sizeof(msgHeader));
    m_msgHeaderStr = (char*)(*(&m_msgHeader)) + sizeof(msgHeader);
    received       = 0;
    size           = msgHeader.nSize - sizeof(msgHeader);
    while (received < size)
    {
        const uint32_t reqLength = size - received;
        const uint32_t res       = m_connectionClient->receiveBuffer((char*)m_msgHeaderStr, reqLength);

        received += res;
        m_msgHeaderStr += res;
    }
    m_buffDataIdx++;

    // Put the data into MessageData32 structure
    m_msgData = nullptr;
    m_msgData = (RDA_MessageData32*)m_msgHeader;

    //////////////////////
    //Markers
    if (m_msgData->nMarkers > 0)
    {
        //      if (m_msgData->nMarkers == 0)
        //      {
        //          return true;
        //      }

        m_marker = (RDA_Marker*)((char*)m_msgData->fData + m_msgData->nPoints * m_header.getChannelCount() * sizeof(m_msgData->fData[0]));

        m_nMarkers = m_msgData->nMarkers;

        m_stimulationIDs.assign(m_nMarkers, 0);
        m_stimulationDates.assign(m_nMarkers, 0);
        m_stimulationSamples.assign(m_nMarkers, 0);

        for (uint32_t i = 0; i < m_msgData->nMarkers; ++i)
        {
            char* type = m_marker->sTypeDesc;
            char* desc = type + strlen(type) + 1;

            m_stimulationIDs[i]     = atoi(strtok(desc, "S"));
            m_stimulationDates[i]   = CTime(m_header.getSamplingFrequency(), m_marker->nPosition).time();
            m_stimulationSamples[i] = m_marker->nPosition;
            m_marker                = (RDA_Marker*)((char*)m_marker + m_marker->nSize);
        }

        m_nMarker += m_msgData->nMarkers;
    }

    size = m_header.getChannelCount() * uint32_t(m_msgData->nPoints);
    if (m_signalBuffers.size() < size) { m_signalBuffers.resize(size); }

    for (size_t i = 0; i < m_header.getChannelCount(); ++i)
    {
        for (uint32_t j = 0; j < uint32_t(m_msgData->nPoints); ++j)
        {
            m_signalBuffers[j + (i * uint32_t(m_msgData->nPoints))] =
                    float(m_msgData->fData[(m_header.getChannelCount() * j) + i]) * m_header.getChannelGain(i);
        }
    }

    // send data
    CStimulationSet stimSet;
    stimSet.resize(m_nMarkers);
    for (uint32_t i = 0; i < m_nMarkers; ++i)
    {
        stimSet.setId(i, OVTK_StimulationId_Label(m_stimulationIDs[i]));
        stimSet.setDate(i, m_stimulationDates[i]);
        stimSet.setDuration(i, 0);
    }

    m_callback->setSamples(&m_signalBuffers[0], uint32_t(m_msgData->nPoints));
    m_callback->setStimulationSet(stimSet);
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

    m_nMarkers = 0;

    return true;
}

bool CDriverBrainProductsBrainVisionRecorder::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Connection stopped\n";
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverBrainProductsBrainVisionRecorder::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_msgHeader)
    {
        free(m_msgHeader);
        m_msgHeader = nullptr;
    }

    m_msgHeaderStr = nullptr;
    m_msgStart     = nullptr;
    m_msgStop      = nullptr;
    m_msgData      = nullptr;
    m_marker       = nullptr;

    m_callback = nullptr;

    m_headerBufferSize = 0;
    m_signalBuffers.clear();

    // Cleans up client connection
    m_connectionClient->close();
    m_connectionClient->release();
    m_connectionClient = nullptr;
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "> Client disconnected\n";

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverBrainProductsBrainVisionRecorder::configure()
{
    CConfigurationNetworkBuilder
            config(Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProducts-BrainVisionRecorder.ui");

    config.setHostName(m_sServerHostName);
    config.setHostPort(m_serverHostPort);

    if (config.configure(m_header))
    {
        m_sServerHostName = config.getHostName();
        m_serverHostPort  = config.getHostPort();

        m_settings.save();

        return true;
    }

    return false;
}

bool CDriverBrainProductsBrainVisionRecorder::reallocateHeaderBuffer(const size_t newSize)
{
    // Reallocate buffer?
    if (newSize > m_headerBufferSize)
    {
        if (m_msgHeader) { free(m_msgHeader); }
        m_headerBufferSize = newSize;
        m_msgHeader        = (RDA_MessageHeader*)malloc(m_headerBufferSize);
        if (!m_msgHeader)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Couldn't allocate memory\n";
            return false;
        }
    }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
