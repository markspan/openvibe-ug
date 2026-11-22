// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only


#include <nlohmann/json.hpp>

#include <CClient.h>
#include <CNetwork.h>
#include <openvibe/acquisition-messaging/ovChannelInfo.h>  // UG fork
#include <openvibe/acquisition-messaging/ovCtrlCmdMsg.h>
#include <openvibe/acquisition-messaging/ovMsg.h>
#include <openvibe/acquisition-messaging/ovMsgSerializer.h>
#include <openvibe/acquisition-messaging/samples.h>

#include "BoxAcquisitionClient.h"

#include <algorithm>
#include <openvibe/ImpedanceRequest.hpp>  // UG fork
#include <cmath>

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

  m_localisationEncoder.initialize(*this, 2);  // UG fork

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
  m_localisationEncoder.uninitialize();  // UG fork

  return true;
}

bool CBoxAcquisitionClient::processClock(Kernel::CMessageClock& /*msg*/)
{
  getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
  return true;
}

// UG fork: upstream asked for the stream as soon as the list of acquisitions came in. A scenario that records to BrainVision first
// has the server measure the impedances, which come with the settings when the stream starts (ovChannelInfo.h).
void CBoxAcquisitionClient::requestStreamWhenReady()
{
  // A server from before the request does not answer it; the recording then starts without the impedances
  constexpr auto ANSWER_TIMEOUT = std::chrono::seconds(20);

  if (m_streamRequested || !m_listReceived) { return; }

  if (!m_impedancesAsked && ImpedanceRequest::wanted(this->getConfigurationManager())) {
    m_impedancesAsked   = true;
    m_impedancesAskedAt = std::chrono::steady_clock::now();
    this->getLogManager() << Kernel::LogLevel_Info << "Asking the acquisition server for the impedances, for the recording; its signal "
                             "stops for a few seconds while the amplifier measures them\n";
    ControlMsg controlMsg;
    controlMsg.command = ControlCommand::MEASURE_IMPEDANCES;
    controlMsg.payload = MeasureImpedancesRequest{m_acquisitionName};
    m_client->send<ControlMsg>(&controlMsg);
    return;
  }

  if (m_impedancesAsked && !m_impedancesAnswered) {
    if (std::chrono::steady_clock::now() - m_impedancesAskedAt < ANSWER_TIMEOUT) { return; }
    this->getLogManager() << Kernel::LogLevel_Warning << "The acquisition server did not answer the request for the impedances (it may "
                             "be older than OpenViBE 4.0.1-ug.11); the recording starts without them\n";
  }
  else if (m_impedancesAsked && !m_impedancesMeasured) {
    this->getLogManager() << Kernel::LogLevel_Warning << "The acquisition server measured no impedances (its log says why); the "
                             "recording has none\n";
  }

  m_streamRequested = true;
  requestStreamFromAcquisition(true);
}

bool CBoxAcquisitionClient::process()
{
  requestStreamWhenReady();  // UG fork

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
    if (!m_montage.empty()) {
      this->getLogManager() << Kernel::LogLevel_Info << "Montage " << m_montage << ", from the acquisition server\n";
    }
    m_signalEncoder.encodeHeader();
    m_stimEncoder.encodeHeader();

    m_lastEndTime = this->getPlayerContext().getCurrentTime();
    m_startTime = m_lastEndTime;
    boxContext.markOutputAsReadyToSend(0, m_startTime, m_lastEndTime);
    // UG fork: from the scenario's start, as a stimulation stream has to be continuous from 0; the signal starts when the server's
    // settings come in, which is later when the server first measures the impedances (UG.md 2.63). The kernel warned otherwise.
    boxContext.markOutputAsReadyToSend(1, 0, m_lastEndTime);

    // UG fork: where the montage places the channels, once, at the same time as the signal's header, so that a box that
    // takes both (the BrainVision writer (UG)) has the positions when the signal starts. With the impedances measured for it,
    // when the server sent them, in a fourth column (impedanceColumnLabel())
    if (!m_localisation.empty()) {
      const size_t columns = m_impedanceTime.empty() ? 3 : 4;
      CMatrix* matrix = m_localisationEncoder.getInputMatrix();
      matrix->resize(m_localisation.size(), columns);
      matrix->setDimensionLabel(1, 0, "x");
      matrix->setDimensionLabel(1, 1, "y");
      matrix->setDimensionLabel(1, 2, "z");
      if (columns == 4) {
        matrix->setDimensionLabel(1, 3, impedanceColumnLabel(m_impedanceTime).c_str());
        this->getLogManager() << Kernel::LogLevel_Info << "Impedances measured at " << m_impedanceTime.c_str() << " by the acquisition server\n";
      }
      double* buffer = matrix->getBuffer();
      for (size_t i = 0; i < m_localisation.size(); ++i) {
        matrix->setDimensionLabel(0, i, m_localisation[i].name.c_str());
        buffer[columns * i]     = m_localisation[i].position.x;
        buffer[columns * i + 1] = m_localisation[i].position.y;
        buffer[columns * i + 2] = m_localisation[i].position.z;
        if (columns == 4) { buffer[columns * i + 3] = m_localisation[i].impedance; }
      }
      m_localisationEncoder.getInputDynamic() = false;
      m_localisationEncoder.encodeHeader();
      boxContext.markOutputAsReadyToSend(2, m_startTime, m_startTime);
      m_localisationEncoder.encodeBuffer();
      boxContext.markOutputAsReadyToSend(2, m_startTime, m_startTime);
    }
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
          m_listReceived    = true;  // UG fork: the stream is asked for in process(), see requestStreamWhenReady()
        }
      }
        break;
      case REQUEST_ACQ_STREAM:
        successResp = std::get<SuccessResponse>(response.payload);
        break;
      case MEASURE_IMPEDANCES:  // UG fork
        m_impedancesMeasured = std::get<SuccessResponse>(response.payload).success;
        m_impedancesAnswered = true;
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
            m_localisation.clear();
            m_impedanceTime = info.impedanceTime;
            for (size_t i = 0; i < m_channelNames.size(); ++i) {
              const bool placed      = i < info.positions.size() && info.positions[i].known;
              const double impedance = i < info.impedances.size() ? info.impedances[i] : std::nan("");
              if (placed || std::isfinite(impedance)) {
                m_localisation.push_back({m_channelNames[i], placed ? toChannelLocalisation(info.positions[i]) : ovCartesianPosition{}, impedance});
              }
            }
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
