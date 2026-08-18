#pragma once

#include <boost/asio.hpp>

#include "IStimulusSender.h"

namespace TCPTagging {
/*
* \class CStimulusSender
* \author Jussi T. Lindgren / Inria
* \brief Simple client to send stimuli to Acquisition Server's TCP Tagging
*/
class CStimulusSender : public IStimulusSender
{
public:
    CStimulusSender() : m_oStimulusSocket(m_ioService) { }
    ~CStimulusSender() override;

    // Connect to the TCP Tagging plugin of the Acquisition Server
    // If sAddress is empty string, the StimulusSender will be inactive and connect() will not print an error but returns false.        
    bool connect(const char* sAddress, const char* sStimulusPort) override;

    // Send a stimulation. 
    bool sendStimulation(uint64_t stimulation, uint64_t timestamp = 0, uint64_t flags = (Flag_Fptime | Flag_Autostamp_Clientside)) override;

protected:
    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::socket m_oStimulusSocket;
    bool m_bConnectedOnce    = false;
    uint64_t m_lastTimestamp = 0;
};
}  // namespace TCPTagging
