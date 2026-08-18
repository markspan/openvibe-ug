#pragma once

#include <sys/timeb.h>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <iostream> // log
#include <utility>

namespace OpenViBE {
class CStimulationConnection
{
public:
    /**
     * Create a new OpenvibeStimulationConnection object.
     *
     * Initiates the message queue.
     *
     * @param queueName The name of the queue.
     * @throw boost::interprocess::interprocess_exception Throws an
     * interprocess exception in the queue fails to be created.
     */
    explicit CStimulationConnection(std::string queueName = "openvibeExternalStimulations") : m_messageQueueName(std::move(queueName))
    {
        boost::interprocess::message_queue::remove(m_messageQueueName.c_str());

        try {
            m_messageQueue = new boost::interprocess::message_queue(boost::interprocess::create_only, m_messageQueueName.c_str(), m_maxMessages,
                                                                    m_chunkLength * sizeof(uint64_t));
        }
        catch (boost::interprocess::interprocess_exception& exception) {
            std::cout << exception.what() << std::endl; // log
            throw;
        }
    }

    /**
     * Send a stimulation to the OpenViBE Acquisition server on the initiated
     * queue.
     *
     * @throw boost::interprocess::interprocess_exception Throws an exception
     * if the message sending failed.
     */
    void SendStimulation(const uint64_t stimulationID) const
    {
        struct timeb currentTime;

        ftime(&currentTime);

        const uint64_t stimulationTime = currentTime.time * 1000 + currentTime.millitm;
        uint64_t message[3];

        message[0] = 0; // unused at the moment
        message[1] = stimulationID;
        message[2] = stimulationTime;

        try { m_messageQueue->send(&message, sizeof(message), 0); }
        catch (boost::interprocess::interprocess_exception& exception) {
            std::cout << exception.what() << std::endl; // log
            throw;
        }
    }

protected:
    std::string m_messageQueueName;
    int m_chunkLength = 3;
    int m_maxMessages = 5000;

    // openvibe currently uses messages of length of 3
    boost::interprocess::message_queue* m_messageQueue = nullptr;
};
}   // namespace OpenViBE
