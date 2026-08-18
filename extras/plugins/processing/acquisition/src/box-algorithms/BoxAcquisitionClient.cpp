// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only


#include <nlohmann/json.hpp>

#include <CClient.h>
#include <CNetwork.h>
#include <openvibe/acquisition-messaging/ovCtrlCmdMsg.h>
#include <openvibe/acquisition-messaging/ovMsg.h>
#include <openvibe/acquisition-messaging/ovMsgSerializer.h>
#include <openvibe/acquisition-messaging/samples.h>

#include "BoxAcquisitionClient.h"

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

  if (m_acquisitionConfigured && !m_headerSent) {
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    m_signalEncoder.encodeHeader();
    m_stimEncoder.encodeHeader();

    m_lastEndTime = this->getPlayerContext().getCurrentTime();
    m_startTime = m_lastEndTime;
    boxContext.markOutputAsReadyToSend(0, m_startTime, m_lastEndTime);
    boxContext.markOutputAsReadyToSend(1, m_startTime, m_lastEndTime);
    m_headerSent = true;
  } else if (m_headerSent && !m_chunkQueue.empty()) {
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    ReceivedChunk& chunk = m_chunkQueue.front();

    double* buffer = m_signalMatrix->getBuffer();
    std::copy(chunk.samples.begin(), chunk.samples.end(), buffer);

    for (const auto& stim : chunk.stims)
      m_stimEncoder.getInputStimulationSet()->push_back(stim.id, stim.date, stim.duration);

    m_signalEncoder.encodeBuffer();
    m_stimEncoder.encodeBuffer();
    m_stimEncoder.getInputStimulationSet()->clear();

    m_chunkQueue.pop();

    m_lastEndTime = m_startTime + CTime(m_sampleRate, m_sampleChunk).time();
    boxContext.markOutputAsReadyToSend(0, m_startTime, m_lastEndTime);
    boxContext.markOutputAsReadyToSend(1, m_startTime, m_lastEndTime);
    m_startTime = m_lastEndTime;
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

          ReceivedChunk chunk;
          chunk.samples.resize(m_channelCount * m_sampleChunk);
          for (size_t i = 0; i < payload.count; ++i)
            for (size_t j = 0; j < m_channelCount; ++j)
              chunk.samples[j * m_sampleChunk + i] = static_cast<double>(payload.samples[i * m_channelCount + j]);

          // Offset m_startTime by the number of chunks already waiting so each
          // chunk's stims are anchored to the correct future OV time slot.
          const uint64_t chunkDuration  = CTime(m_sampleRate, m_sampleChunk).time();
          const uint64_t chunkStartTime = m_startTime + m_chunkQueue.size() * chunkDuration;

          for (size_t i = 0; i < payload.stimCount; ++i) {
            size_t idx = 0;
            while (idx + 1 < payload.count && payload.timestamp[idx + 1] <= payload.stimulations[i].date)
              idx++;
            chunk.stims.push_back({payload.stimulations[i].id,
                                   chunkStartTime + CTime(m_sampleRate, idx).time(),
                                   payload.stimulations[i].duration});
          }

          m_chunkQueue.push(std::move(chunk));
        }
        break;
      case ovMsg::msgTypeEnum::OV_MSG_ACQUISITION_CONFIG:
        {
          if (! m_acquisitionConfigured) {
            auto& payload = msg.msgData.acquisitionConfigStruct;
            m_channelCount = payload.channelCount;
            m_sampleRate = payload.sampleRate;
            m_sampleChunk = payload.sampleChunk;
            m_signalMatrix->resize(m_channelCount, m_sampleChunk);
            m_acquisitionConfigured = true;
            std::cout << "Acquisition started" << std::endl;
            std::cout << "Channel count: " << m_channelCount << std::endl;
            std::cout << "Sample rate: " << m_sampleRate << std::endl;
            std::cout << "Sample chunk: " << m_sampleChunk << std::endl;

            /* TODO: get channel names from acquisition config, or elsewhere
            // Convention: channel shown as users go as 1,2,...
            for (size_t i = 0; i < m_nChannel; ++i) {
                matrix->setDimensionLabel(0, i, ("channel " + std::to_string(i + 1)).c_str());
            }
              */
          }
        }
        break;
      default:
        break;
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