#pragma once
#include "defines.h"

namespace TCPTagging {
/*
* \class IStimulusSender
* \author Jussi T. Lindgren / Inria
* \brief Interface of a simple client to send stimuli to Acquisition Server's TCP Tagging
*/
class OV_API IStimulusSender
{
public:
    // Connect to the TCP Tagging plugin of the Acquisition Server
    // If sAddress is empty string, the StimulusSender will be inactive and connect() will not print an error but returns false.        
    virtual bool connect(const char* sAddress, const char* sStimulusPort) = 0;

    // Send a stimulation. 
    // Or flags with FLAG_FPTIME if the provided time is fixed point.
    // Or flags with FLAG_AUTOSTAMP_CLIENTSIDE to set the latest timestamp before sending. Then, timestamp is ignored.
    // Or flags with FLAG_AUTOSTAMP_SERVERSIDE to request these server to stamp on receiveing. Then, timestamp is ignored.
    virtual bool sendStimulation(uint64_t stimulation, uint64_t timestamp = 0, uint64_t flags = (Flag_Fptime | Flag_Autostamp_Clientside)) = 0;

    // To allow derived class' destructor to be called
    virtual ~IStimulusSender() = default;

    // Note: duplicated in TCP Tagging plugin in AS
    enum ETCPTaggingFlags
    {
        Flag_Fptime = (1LL << 0),                       // The time given is fixed point time.
        Flag_Autostamp_Clientside = (1LL << 1),         // Ignore given stamp, bake timestamp on client side before sending
        Flag_Autostamp_Serverside = (1LL << 2)          // Ignore given stamp, bake timestamp on server side when receiving
    };
};

// Clients are constructed via this call.
extern OV_API IStimulusSender* CreateStimulusSender();
}  // namespace TCPTagging
