#include "CStimulusSender.h"

#include <iostream>
#include <system/ovCTime.h>

// @fixme Should use some logging facility instead of std::cout
namespace TCPTagging {

CStimulusSender::~CStimulusSender() { if (m_oStimulusSocket.is_open()) { m_oStimulusSocket.close(); } }

bool CStimulusSender::connect(const char* sAddress, const char* sStimulusPort)
{
    boost::asio::ip::tcp::resolver resolver(m_ioService);

    if (!sAddress || !sStimulusPort) {
        std::cout << "Error: Do not pass NULL pointers to CStimulusSender::connect()\n";
        return false;
    }
    if (sAddress[0] == 0) { return false; } // Empty string is ok, in that case Stimulus Sender is disabled

    // Stimulus port
    std::cout << "Connecting to Acquisition Server's TCP Tagging [" << sAddress << " , port " << sStimulusPort << "]\n";
    try {
        boost::system::error_code error;

        const boost::asio::ip::tcp::resolver::query query = boost::asio::ip::tcp::resolver::query(
            boost::asio::ip::tcp::v4(), sAddress, sStimulusPort, boost::asio::ip::resolver_query_base::numeric_service);
        const auto endpointIterator = resolver.resolve(query);
        m_oStimulusSocket.connect(*endpointIterator, error);
        if (error) {
            std::cout << "-- Boost ASIO connection error: " << error << "\n";
            return false;
        }
    }
    catch (boost::system::system_error& error) {
        std::cout << "-- Issue '" << error.code().message() << "' with opening connection to server\n";
        return false;
    }

    m_bConnectedOnce = true;
    m_lastTimestamp  = 0;

    return true;
}

bool CStimulusSender::sendStimulation(uint64_t stimulation, uint64_t timestamp, uint64_t flags /* = FPTIME|CLIENTSIDE */)
{
    if (!m_bConnectedOnce) { return false; }

    if (!m_oStimulusSocket.is_open()) {
        std::cout << "Error: Cannot send stimulation, socket is not open\n";
        return false;
    }

    if (flags & Flag_Autostamp_Clientside) {
        timestamp = System::Time::zgetTimeRaw(false);
        flags |= Flag_Fptime;
    }

    if (timestamp < m_lastTimestamp) {
        std::cout << "Error: Stimulations must be inserted in increasing time order (now: "
                << timestamp << ", prev: " << m_lastTimestamp << ", stim=" << stimulation << ")\n";
        return false;
    }
    m_lastTimestamp = timestamp;

    try {
        write(m_oStimulusSocket, boost::asio::buffer(static_cast<void*>(&flags), sizeof(uint64_t)));
        write(m_oStimulusSocket, boost::asio::buffer(static_cast<void*>(&stimulation), sizeof(uint64_t)));
        write(m_oStimulusSocket, boost::asio::buffer(static_cast<void*>(&timestamp), sizeof(uint64_t)));
    }
    catch (boost::system::system_error& error) {
        std::cout << "Issue '" << error.code().message() << "' with writing stimulus to server\n";
        return false;
    }

    return true;
}

}  // namespace TCPTagging
