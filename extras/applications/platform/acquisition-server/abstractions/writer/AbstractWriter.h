/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * @file AbstractWriter.h
 * @brief Header file for the AbstractWriter class.
 *
 * This file declares the `AbstractWriter` class, which serves as an abstract base
 * class for acquisition data writers.
 */

#pragma once

#include <samplesBuffer.h>

#include <openvibe/acquisition-messaging/drift.h>
#include <openvibe/acquisition-messaging/ovChannelInfo.h>
#include <openvibe/acquisition-messaging/ovMsg.h>
#include <openvibe/parameters/parameters.h>

#include <OVAcquisitionAbstractions_Export.h>

#include <functional>
#include <list>
#include <string>
#include <vector>

/**
 * @brief Abstract base class for acquisition data writers.
 *
 * The `AbstractWriter` class defines a common interface for writing acquisition data to various destinations.
 * It provides pure virtual methods for initialization, writing, and cleanup, as well as a virtual method for providing
 * a list of clients to send data to.
 */
class OVACQUISITIONABSTRACTIONS_EXPORT AbstractWriter
{
public:
   /// Callback type used to retrieve the list of active client identifiers.
   using ClientProvider = std::function<std::list<uint64_t>()>;

   explicit AbstractWriter(const std::string& writerName);
   virtual ~AbstractWriter();

   virtual std::string name() const { return m_name; }

   /**
    * @brief Initializes the writer with buffer dimensions.
    * @param chunkSamplesCount Number of samples per chunk.
    * @param chanelCount Number of acquisition channels.
    * @return @c true on success, @c false otherwise.
    */
   bool init(uint32_t chunkSamplesCount, uint32_t chanelCount);

   /**
    * @brief Retrieves the parameters of the driver.
    * @return A vector of parameters.
    */
   virtual OpenViBE::Parameters::Parameters getParameters() { return m_parameters; }

   /**
    * @brief Called between acquisition runs to release run-time resources (e.g. LSL outlets).
    * Base implementation is a no-op; subclasses override as needed.
    */
   virtual void uninit() {}

   /**
    * @brief Called after init() with driver-provided acquisition parameters.
    *
    * Override if needed, to perform setup that requires the sample rate or channel names
    *
    * @param sampleRate   Sampling rate reported by the driver (in Hz).
    * @param channelNames Names of the acquired channels.
    */
   virtual void onAcquisitionSetup(uint32_t /*sampleRate*/, const std::vector<std::string>& /*channelNames*/) {}

   /**
    * @brief Sets the callback used to retrieve the list of active clients.
    * @param provider Callable returning a list of client identifiers.
    */
   void setClientProvider(ClientProvider provider) { m_clientProvider = std::move(provider); }

   /**
    * @brief Serializes and sends the acquisition configuration message.
    * @param channelCount Number of channels.
    * @param sampleRate   Sampling rate (in Hz).
    * @param sampleChunk  Number of samples per chunk.
    * @param channels     Names of the channels, and the montage that named them; missing names are sent empty. UG fork:
    *                     upstream sends neither.
    * @param clientId     Target client identifier, or @c -1 to broadcast to all clients.
    * @return Number of bytes sent.
    */
   size_t writeAcquisitionConfig(uint32_t channelCount, uint32_t sampleRate, uint32_t sampleChunk, const ovChannelInfo& channels,
                                 uint64_t clientId = -1);

   /**
    * @brief Serializes and sends a drift information message.
    * @param drift Drift data to be transmitted.
    * @return Number of bytes sent.
    */
   size_t writeDrifts(drift_st const& drift);

   /**
    * @brief Serializes and sends a samples message.
    * @param buffer Buffer containing the samples to write.
    * @return Number of bytes sent.
    */
   size_t writeSamples(samplesBuffer* buffer);

protected:
   /**
    * @brief Sends a message to one or all clients.
    *
    * Pure virtual — each concrete writer implements the actual transport.
    *
    * @param msg      Message to send.
    * @param clientId Target client identifier, or @c -1 to broadcast to all clients.
    * @return Number of bytes sent.
    */
   virtual size_t send(ovMsg* msg, uint64_t clientId = -1) = 0;

   /**
    * @brief Returns the list of active clients from the registered provider.
    * @return List of client identifiers, or an empty list if no provider is set.
    */
   std::list<uint64_t> getClients() const { return m_clientProvider ? m_clientProvider() : std::list<uint64_t>{}; }

   std::string m_name; ///< Writer name.

   size_t m_channelCount;      ///< Number of acquisition channels.
   size_t m_chunkSamplesCount; ///< Number of samples per chunk.

   /**
    * @brief Writer configuration parameters.
    *
    * This list must be populated by each concrete writer subclass.
    */
   OpenViBE::Parameters::Parameters m_parameters;

private:
   ovMsg          m_msg;            ///< Reusable message buffer; pre-allocated to avoid incomplete-type errors.
   ClientProvider m_clientProvider; ///< Callback providing the list of active client identifiers.
};
