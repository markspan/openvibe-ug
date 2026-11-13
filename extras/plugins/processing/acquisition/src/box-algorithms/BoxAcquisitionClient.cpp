// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only


#include <nlohmann/json.hpp>

#include <CClient.h>
#include <CNetwork.h>
#include <openvibe/acquisition-messaging/ovChannelInfo.h>
#include <openvibe/acquisition-messaging/ovCtrlCmdMsg.h>
#include <openvibe/acquisition-messaging/ovMsg.h>
#include <openvibe/acquisition-messaging/ovMsgSerializer.h>
#include <openvibe/acquisition-messaging/samples.h>

#include "BoxAcquisitionClient.h"

#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace Acquisition {

uint64_t CBoxAcquisitionClient::getClockFrequency() { return 64LL << 32; }

bool CBoxAcquisitionClient::initialize()
{


  CString name      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
  uint64_t port = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

  m_signalEncoder.initialize(*this, 0);
  // Linking sampling rate and matrix. Once m_sampleRate is set and matrix resized, header can be encoded.
  m_signalEncoder.getInputSamplingRate().setReferenceTarget(m_sampleRate);
  m_signalMatrix = m_signalEncoder.getInputMatrix();

  m_stimEncoder.initialize(*this, 1);

  // UG fork: no montage until the server sends one
  this->getConfigurationManager().addOrReplaceConfigurationToken(OV_CHANNEL_INFO_MONTAGE_TOKEN, "");
  this->getConfigurationManager().addOrReplaceConfigurationToken(OV_CHANNEL_INFO_POSITIONS_TOKEN, "");

  m_client = network.createClient();

  if (!m_client->setAddress(name.toASCIIString(), static_cast<uint16_t>(port))) {
    this->getLogManager() << Kernel::LogLevel_Error << "Could not set client address and port: " << name.toASCIIString() << ":" << port << "\n";
    return false;
  }

  if (!m_client->connect()) {
    this->getLogManager() << Kernel::LogLevel_Error << "Could not connect to server: " << name.toASCIIString() << ":" << port << "\n";
    return false;
  }

  m_client->registerReadCallback<ControlMsgResponse>(
    [this](void* data, size_t dataSize, uint64_t serverId) {
      return this->readFromServer(data, dataSize, serverId);
    }
  );
  m_client->registerWriteCallback<ControlMsg>(
    [this](void* data, uint64_t serverId) {
      return this->writeToServer(data, serverId);
    }
  );
  m_client->registerReadCallback<ovMsg>(
    [this](void* data, size_t dataSize, uint64_t serverId) {
      return this->receiveDataStream(data, dataSize, serverId);
    }
  );
  
  this->getLogManager() << Kernel::LogLevel_Info << "OpenViBE Acquisition Client initialized\n";

  // Init:
  // 1. Get acquisition list
  ControlMsg controlMsg;
  controlMsg.command = ControlCommand::GET_ACQ_LIST;
  controlMsg.payload = std::monostate{};
  m_client->send<ControlMsg>(&controlMsg);

  // 2. Check acquisition list in processClock() and start acquiring as soon as possible

  return true;
}

bool CBoxAcquisitionClient::uninitialize()
{
  if (m_client) {
    requestStreamFromAcquisition(false);
    m_client->disconnect();
    network.removeClient(m_client);
  }

  m_signalEncoder.uninitialize();
  m_stimEncoder.uninitialize();

  return true;
}

bool CBoxAcquisitionClient::processClock(Kernel::CMessageClock& /*msg*/)
{
  getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
  return true;
}

bool CBoxAcquisitionClient::process()
{
  std::lock_guard<std::mutex> lock(m_signalMutex);

  // UG fork: what the network thread found, see receiveDataStream()
  if (!m_notice.empty()) {
    this->getLogManager() << Kernel::LogLevel_Warning << m_notice.c_str() << "\n";
    m_notice.clear();
  }
  if (!m_failure.empty()) {
    this->getLogManager() << Kernel::LogLevel_Error << m_failure.c_str() << "\n";
    return false;
  }

  if (m_acquisitionConfigured && !m_headerSent) {
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    // UG fork: before the header, with which the BrainVision writer writes its own
    this->getConfigurationManager().addOrReplaceConfigurationToken(OV_CHANNEL_INFO_MONTAGE_TOKEN, m_montage.c_str());
    this->getConfigurationManager().addOrReplaceConfigurationToken(OV_CHANNEL_INFO_POSITIONS_TOKEN, m_electrodePositions.c_str());
    if (!m_montage.empty()) {
      this->getLogManager() << Kernel::LogLevel_Info << "Montage " << m_montage << ", from the acquisition server\n";
    }
    m_signalEncoder.encodeHeader();
    m_stimEncoder.encodeHeader();

    m_lastEndTime = this->getPlayerContext().getCurrentTime();
    m_startTime = m_lastEndTime;
    boxContext.markOutputAsReadyToSend(0, m_startTime, m_lastEndTime);
    boxContext.markOutputAsReadyToSend(1, m_startTime, m_lastEndTime);
    m_stimulationDelay.configure(StimulationDelay::chunksFor(m_sampleChunk, m_sampleRate),
                                 [this](const size_t sample) { return CTime(m_sampleRate, sample).time(); });
    m_headerSent = true;
  } else if (m_headerSent && m_blocks.hasChunk()) {
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Drain everything that arrived since the last tick rather than one chunk per tick. The clock
    // runs at getClockFrequency() (64 Hz), so emitting a single chunk per call capped this box at
    // 64 * m_sampleChunk samples/s -- only 2048 Hz with the default 32-sample block. Past that the
    // stream fell behind the wall clock (a 16384 Hz device advanced at an eighth of real time) and
    // the queue grew without bound. Sending several chunks from one process() is the usual
    // pattern here; see CBoxAlgorithmOVCSVFileReader::process().
    while (m_blocks.hasChunk()) {
      SampleBlocks::Chunk chunk = m_blocks.takeChunk();

      double* buffer = m_signalMatrix->getBuffer();
      std::copy(chunk.samples.begin(), chunk.samples.end(), buffer);

      m_signalEncoder.encodeBuffer();

      const uint64_t chunkDuration = CTime(m_sampleRate, m_sampleChunk).time();
      m_lastEndTime = m_startTime + chunkDuration;
      boxContext.markOutputAsReadyToSend(0, m_startTime, m_lastEndTime);

      // UG fork: the stimulations stay behind the signal, so that a late one gets its own sample, see StimulationDelay.
      // Upstream sends them with the chunk they came with.
      m_stimulationDelay.addChunk(m_startTime, std::move(chunk.stamps), chunk.stims);
      if (m_stimulationDelay.hasDueChunk()) {
        const StimulationDelay::Chunk due = m_stimulationDelay.takeDueChunk();
        for (const auto& stim : due.stims) {
          m_stimEncoder.getInputStimulationSet()->push_back(stim.id, stim.date, stim.duration);
        }
        m_stimEncoder.encodeBuffer();
        m_stimEncoder.getInputStimulationSet()->clear();
        boxContext.markOutputAsReadyToSend(1, due.startTime, due.startTime + chunkDuration);
      }

      m_startTime = m_lastEndTime;
    }
  }
  return true;
}

bool CBoxAcquisitionClient::readFromServer(void* data, size_t dataSize, uint64_t serverId)
{
  try {
    // Parse JSON response
    const char* jsonData = reinterpret_cast<const char*>(data);
    std::string jsonStr(jsonData, dataSize);

    nlohmann::json j = nlohmann::json::parse(jsonStr);
    ControlMsgResponse response = j.get<ControlMsgResponse>();
    SuccessResponse successResp;
    // Process response based on command type
    switch (response.command) {
      case CREATE_ACQUISITION:
      {
        auto& payload = response.payload;
        successResp = std::get<SuccessResponse>(payload);
      }
        break;
      case GET_ACQ_LIST:
      {
        auto& payload = response.payload;
        AcquisitionListResponse acqListResp = std::get<AcquisitionListResponse>(payload);
        if(acqListResp.acquisitionNames.size() > 0) {
          m_acquisitionName = acqListResp.acquisitionNames.front();
          requestStreamFromAcquisition(true);
        }
      }
        break;
      case REQUEST_ACQ_STREAM:
        successResp = std::get<SuccessResponse>(response.payload);
        break;
      default:
        break;
    }

    m_responseReceived = true;
    return true;

  } catch (const std::exception& e) {
    return false;
  }
}

std::pair<size_t, void*> CBoxAcquisitionClient::writeToServer(void* data, uint64_t serverId)
{
  try {
        ControlMsg* msg = reinterpret_cast<ControlMsg*>(data);

        // Serialize to JSON
        nlohmann::json j = *msg;
        std::string jsonStr = j.dump();

        // Allocate buffer for JSON string (will be freed by Network++ after sending)
        char* buffer = new char[jsonStr.size()];
        std::memcpy(buffer, jsonStr.data(), jsonStr.size());

        return {jsonStr.size(), buffer};
    } catch (const std::exception& e) {
        return {0, nullptr};
    }
}

bool CBoxAcquisitionClient::receiveDataStream(void* data, size_t dataSize, uint64_t serverId)
{
  try {
    ovMsg msg = deserializeOvMsg(reinterpret_cast<const uint8_t*>(data), dataSize);
    switch(msg.msgType) {
      case ovMsg::msgTypeEnum::OV_MSG_SAMPLES:
        {
          auto& payload = msg.msgData.samplesStruct;
          std::lock_guard<std::mutex> lock(m_signalMutex);
          if (!m_failure.empty()) { break; }  // the scenario stops at the next process()

          // UG fork: the block, of whatever size, goes to SampleBlocks, which cuts it into the stream's chunks and keeps every
          // sample. Upstream wrote it into one chunk of m_sampleChunk samples, and dropped the samples past that: after the server
          // was restarted with blocks of 64 instead of 4, a running scenario kept one sample in 16, and said nothing.
          // The number of values is worked out as deserializeOvMsg() does, from the message's size.
          const size_t fixed  = 4 * sizeof(uint32_t) + size_t(payload.count) * sizeof(timestampType)
                                + size_t(payload.stimCount) * sizeof(ovStimulation);
          const size_t values = msg.msgSize >= fixed ? (msg.msgSize - fixed) / sizeof(sampleType) : 0;
          if (!m_blocks.add(payload.samples, values, payload.timestamp, payload.count, payload.stimulations, payload.stimCount)) {
            m_failure = !m_blocks.configured()
                          ? std::string("The acquisition server sent samples before its settings; they can not be placed.")
                          : "The acquisition server sent a block of " + std::to_string(payload.count) + " samples with "
                            + std::to_string(values) + " values, which is not " + std::to_string(m_channelCount)
                            + " channels: its channels changed while the scenario ran. Start the scenario again.";
          }
        }
        break;
      case ovMsg::msgTypeEnum::OV_MSG_ACQUISITION_CONFIG:
        {
          std::lock_guard<std::mutex> lock(m_signalMutex);  // UG fork: the matrix is also read by process()
          auto& payload = msg.msgData.acquisitionConfigStruct;

          // UG fork: the server sends the channel names, a channel without one is numbered from 1, and the montage
          // that named them (ovChannelInfo.h). Upstream leaves the channels unnamed
          const ovChannelInfo info = decodeChannelInfo(payload.channelNames, payload.channelNamesSize, payload.channelCount);
          std::vector<std::string> names(payload.channelCount);
          for (size_t i = 0; i < names.size(); ++i) { names[i] = info.names[i].empty() ? std::to_string(i + 1) : info.names[i]; }

          // UG fork: the server sends its settings again when it starts an acquisition anew (Acquisition::initAcquisition()).
          // Upstream took only the first, and read the blocks of a restarted server with the settings of the one before.
          if (m_headerSent) {
            // The stream has gone out with the first settings, and can not change them. Another block size is fine: the
            // blocks are cut into the stream's chunks. Another rate, or other channels, can not be placed in the stream.
            if (payload.sampleRate != m_sampleRate || payload.channelCount != m_channelCount || names != m_channelNames
                || info.montage != m_montage) {
              m_failure = "The acquisition server was started again with other settings (" + std::to_string(payload.sampleRate)
                          + " Hz, " + std::to_string(payload.channelCount) + " channels; the scenario has " + std::to_string(m_sampleRate)
                          + " Hz, " + std::to_string(m_channelCount) + " channels" + (names != m_channelNames ? ", other names" : "")
                          + (info.montage != m_montage ? ", another montage" : "") + "). Start the scenario again.";
            }
            else if (payload.sampleChunk != m_sampleChunk) {
              m_notice = "The acquisition server now sends blocks of " + std::to_string(payload.sampleChunk) + " samples; the signal stream keeps its chunks of "
                         + std::to_string(m_sampleChunk) + ", and every sample.";
            }
          }
          else {
            if (m_blocks.pendingSamples() > 0 || m_blocks.hasChunk()) {
              m_notice = "The acquisition server was started again before the scenario's signal stream began; the samples before that are left out.";
            }
            m_channelCount = payload.channelCount;
            m_sampleRate = payload.sampleRate;
            m_sampleChunk = payload.sampleChunk;
            m_signalMatrix->resize(m_channelCount, m_sampleChunk);
            for (size_t i = 0; i < m_channelCount; ++i) { m_signalMatrix->setDimensionLabel(0, i, names[i].c_str()); }
            m_channelNames       = std::move(names);
            m_montage            = info.montage;
            m_electrodePositions = encodeElectrodePositions(info);
            m_blocks.configure(m_channelCount, m_sampleChunk);

            m_acquisitionConfigured = true;
            std::cout << "Acquisition started" << std::endl;
            std::cout << "Channel count: " << m_channelCount << std::endl;
            std::cout << "Sample rate: " << m_sampleRate << std::endl;
            std::cout << "Sample chunk: " << m_sampleChunk << std::endl;
          }
        }
        break;
      default:
        break;
    }

    // UG fork: deserializeOvMsg() allocates the arrays it fills, and the data has been copied out of them; upstream
    // leaked them
    if (msg.msgType == ovMsg::msgTypeEnum::OV_MSG_SAMPLES) {
      delete[] msg.msgData.samplesStruct.timestamp;
      delete[] msg.msgData.samplesStruct.samples;
      delete[] msg.msgData.samplesStruct.stimulations;
    }
    else if (msg.msgType == ovMsg::msgTypeEnum::OV_MSG_ACQUISITION_CONFIG) {
      delete[] msg.msgData.acquisitionConfigStruct.channelNames;
    }
    return true;

  } catch (const std::exception& e) {
    return false;
  }
}

void CBoxAcquisitionClient::requestStreamFromAcquisition(bool enable)
{
  if (m_acquisitionName.empty()) {
    return;
  }

  ControlMsg controlMsg;
  controlMsg.command = ControlCommand::REQUEST_ACQ_STREAM;
  RequestAcqStreamRequest request;
  request.acquisitionName = m_acquisitionName;
  request.enable = enable;
  controlMsg.payload = request;
  m_client->send<ControlMsg>(&controlMsg);
}

}  // namespace Acquisition
}  // namespace Plugins
}  // namespace OpenViBE
