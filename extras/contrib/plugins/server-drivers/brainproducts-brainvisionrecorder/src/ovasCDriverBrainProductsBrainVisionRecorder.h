#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <vector>

#include <socket/IConnectionClient.h>

#ifndef M_DEFINE_GUID
#define M_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif

#ifndef M_COMPARE_GUID
#define M_COMPARE_GUID(name1, name2) \
        (   name1.Data1 == name2.Data1 && name1.Data2 == name2.Data2 && \
            name1.Data3 == name2.Data3 && name1.Data4[0] == name2.Data4[0] && \
            name1.Data4[1] == name2.Data4[1] && name1.Data4[2] == name2.Data4[2] &&\
            name1.Data4[3] == name2.Data4[3] && name1.Data4[4] == name2.Data4[4] &&\
            name1.Data4[5] == name2.Data4[5] && name1.Data4[6] == name2.Data4[6] &&\
            name1.Data4[7] == name2.Data4[7] \
        )
#endif

namespace OpenViBE {
namespace AcquisitionServer {
class CDriverBrainProductsBrainVisionRecorder final : public IDriver
{
public:

    explicit CDriverBrainProductsBrainVisionRecorder(IDriverContext& ctx);
    ~CDriverBrainProductsBrainVisionRecorder() override {}
    const char* getName() override { return "Brain Products amplifiers (through BrainVision Recorder)"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;

    const IHeader* getHeader() override { return &m_header; }

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback                   = nullptr;
    Socket::IConnectionClient* m_connectionClient = nullptr;
    CString m_sServerHostName                     = "localhost";
    uint32_t m_serverHostPort                     = 51244;

    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;

    uint32_t m_indexIn     = 0;
    uint32_t m_indexOut    = 0;
    uint32_t m_buffDataIdx = 0;

    uint32_t m_nMarker  = 0;
    uint32_t m_nMarkers = 0;

    std::vector<size_t> m_stimulationIDs;
    std::vector<size_t> m_stimulationDates;
    std::vector<size_t> m_stimulationSamples;

#pragma pack(push)
#pragma pack(1)

    typedef struct
    {
        uint32_t Data1;
        unsigned short Data2;
        unsigned short Data3;
        uint8_t Data4[8];
    } GUID;

    struct RDA_Marker           //; A single marker in the marker array of RDA_MessageData
    {
        uint32_t nSize;         // Size of this marker.
        uint32_t nPosition;     // Relative position in the data block.
        uint32_t nPoints;       // Number of points of this marker
        int nChannel;           // Associated channel number (-1 = all channels).
        char sTypeDesc[1];      // Type, description in ASCII delimited by '\0'.
    };

    struct RDA_MessageHeader    //; Message header
    {
        GUID guid;              // Always GUID_RDAHeader
        uint32_t nSize;         // Size of the message block in bytes including this header
        uint32_t nType;         // Message type.
    };

    // **** Messages sent by the RDA server to the clients. ****
    struct RDA_MessageStart : RDA_MessageHeader //; Setup / Start infos, Header -> nType = 1
    {
        uint32_t nChannels;         // Number of channels
        double dSamplingInterval;   // Sampling interval in microseconds
        double dResolutions[1];     // Array of channel resolutions -> double dResolutions[nChannels] coded in microvolts. i.e. RealValue = resolution * A/D value
        char sChannelNames[1];      // Channel names delimited by '\0'. The real size is larger than 1.
    };

    struct RDA_MessageStop : RDA_MessageHeader { }; //; Data acquisition has been stopped. // Header -> nType = 3

    struct RDA_MessageData32 : RDA_MessageHeader    //; Block of 32-bit floating point data, Header -> nType = 4, sent only from port 51244
    {
        uint32_t nBlock;        // Block number, i.e. acquired blocks since acquisition started.
        uint32_t nPoints;       // Number of data points in this block
        uint32_t nMarkers;      // Number of markers in this data block
        float fData[1];         // Data array -> float fData[nChannels * nPoints], multiplexed
        RDA_Marker Markers[1];  // Array of markers -> RDA_Marker Markers[nMarkers]
    };
#pragma pack(pop)

    RDA_MessageHeader* m_msgHeader = nullptr;
    char* m_msgHeaderStr           = nullptr;
    size_t m_headerBufferSize      = 0;

    RDA_MessageStart* m_msgStart = nullptr;
    RDA_MessageStop* m_msgStop   = nullptr;
    RDA_MessageData32* m_msgData = nullptr;
    RDA_Marker* m_marker         = nullptr;

    std::vector<float> m_signalBuffers;

private:

    bool reallocateHeaderBuffer(size_t newSize);
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
