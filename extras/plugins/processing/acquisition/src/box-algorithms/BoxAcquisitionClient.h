// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <atomic>

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <CNetwork.h>

#include <openvibe/acquisition-messaging/samples.h>

#include "StimulationDelay.h"

#include <queue>
#include <vector>

namespace Network {
class CClient;
}

namespace OpenViBE {
namespace Plugins {
namespace Acquisition {

class CBoxAcquisitionClient final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
  void release() override { delete this; }

  uint64_t getClockFrequency() override;
  bool initialize() override;
  bool uninitialize() override;
  bool processClock(Kernel::CMessageClock& msg) override;
  bool process() override;

  // Methods to communicate with the acquisition server
	bool readFromServer(void* data, size_t dataSize, uint64_t serverId);
	std::pair<size_t, void*> writeToServer(void* data, uint64_t serverId);
  
  // Method to receive data stream of a specific acquisition from the acquisition server
  bool receiveDataStream(void* data, size_t dataSize, uint64_t serverId);

  _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_AcquisitionClient)

protected:
	Network::CNetwork network;
	Network::CClient* m_client = nullptr;
	bool m_responseReceived = false;

  uint64_t m_startTime = 0;
  uint64_t m_lastEndTime   = 0;

  std::string m_acquisitionName = "";
  uint32_t m_channelCount = 0;
  uint64_t m_sampleRate = 0;
  uint32_t m_sampleChunk = 0;
  bool m_acquisitionConfigured = false;
  bool m_headerSent = false;
  // UG fork: the montage the server sent, passed on to the scenario with the header, see ovChannelInfo.h
  std::string m_montage;
  std::string m_electrodePositions;

  // UG fork: the stamps are kept and the stimulations stay in the server's clock, for StimulationDelay to place. Upstream gives
  // each stimulation a sample within the chunk it came with, and has no stamps here.
  struct ReceivedChunk {
    std::vector<double>        samples;
    std::vector<uint64_t>      stamps;
    std::vector<ovStimulation> stims;
  };

  std::queue<ReceivedChunk> m_chunkQueue;
  StimulationDelay m_stimulationDelay;  // UG fork
  std::mutex m_signalMutex;
  CMatrix* m_signalMatrix = nullptr;

  Toolkit::TSignalEncoder<CBoxAcquisitionClient> m_signalEncoder;
  Toolkit::TStimulationEncoder<CBoxAcquisitionClient> m_stimEncoder;

  void requestStreamFromAcquisition(bool enable);
};

class CBoxAcquisitionClientDesc final : public IBoxAlgorithmDesc
{
public:
  void release() override { }

  CString getName() const override { return "Acquisition client"; }
  CString getAuthorName() const override { return "Thomas Prampart"; }
  CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
  CString getShortDescription() const override { return "Network based acquisition client, from openvibe 4.0"; }
  CString getDetailedDescription() const override { return "This box communicates with the acquisition server to acquire data from the network."; }
  CString getCategory() const override { return "Acquisition and network IO"; }
  CString getVersion() const override { return "1.0"; }

  CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_AcquisitionClient; }
  IPluginObject* create() override { return new CBoxAcquisitionClient; }
  CString getStockItemName() const override { return ""; }

  bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
  {
  //  prototype.addOutput("Experiment information", OV_TypeId_ExperimentInfo);
    prototype.addOutput("Signal stream", OV_TypeId_Signal);
    prototype.addOutput("Stimulations", OV_TypeId_Stimulations);
  //  prototype.addOutput("Channel localisation", OV_TypeId_ChannelLocalisation);
  //  prototype.addOutput("Channel units", OV_TypeId_ChannelUnits);
    prototype.addSetting("Acquisition server hostname", OV_TypeId_String, "127.0.0.1");
    prototype.addSetting("Acquisition server port", OV_TypeId_Integer, "8088");
    return true;
  }

  _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_AcquisitionClientDesc)
};

}  // namespace Acquisition
}  // namespace Plugins
}  // namespace OpenViBE
