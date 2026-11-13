// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <atomic>

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <CNetwork.h>

#include <openvibe/acquisition-messaging/samples.h>

#include "SampleBlocks.h"
#include "StimulationDelay.h"

#include <string>
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

  // UG fork: the channel names of the stream's header, against which a later configuration from the server is checked
  std::vector<std::string> m_channelNames;

  // UG fork: the server's blocks, of any size, cut into the stream's chunks, with the server's stamps, see SampleBlocks. Upstream
  // queued each block as one chunk and dropped what did not fit.
  SampleBlocks m_blocks;
  StimulationDelay m_stimulationDelay;  // UG fork
  // UG fork: set by the network thread, logged by process(). A failure stops the scenario: the server's samples no longer fit the
  // stream (another rate, other channels), and no box can take a stream that changes midway.
  std::string m_failure;
  std::string m_notice;
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
