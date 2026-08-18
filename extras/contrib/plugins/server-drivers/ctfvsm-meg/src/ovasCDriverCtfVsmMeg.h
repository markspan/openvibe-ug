#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <openvibe/ov_all.h>

#include <socket/IConnectionClient.h>

#define NB_CHAN_RECORDED_MAX 410
#define NB_SAMP_ACQ_Packet   28160

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverCtfVsmMeg
 * \author Pierre-Emmanuel Aguera (INSERM)
 */
class CDriverCtfVsmMeg final : public IDriver
{
public:

    typedef char str32[32];
    typedef char str100[100];

    explicit CDriverCtfVsmMeg(IDriverContext& ctx);
    ~CDriverCtfVsmMeg() override { }
    const char* getName() override { return "CTF/VSM MEG"; }


    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    Socket::IConnectionClient* m_connectionClient = nullptr;
    CString m_sServerHostName                     = "localhost";
    uint32_t m_serverHostPort                     = 9999;

    struct
    {
        int nbCharExperimentId;
        str100 experimentId;
        int nbCharExperimentDate;
        str32 experimentDate;

        int nbCharSubjectName;
        str32 subjectName;
        int subjectAge;
        char subjectGender[1]; /**F: female or M: Male*/

        int labId;
        str32 labName;
        int technicianId;
        str32 technicianName;

        float samplingRate;
        int numberOfChannels;
        str32 channelLabel[NB_CHAN_RECORDED_MAX];
        int channelTypeIndex[NB_CHAN_RECORDED_MAX];

        float properGain[NB_CHAN_RECORDED_MAX];
        float qGain[NB_CHAN_RECORDED_MAX];
        float ioGain[NB_CHAN_RECORDED_MAX];

        int numberOfCoils[NB_CHAN_RECORDED_MAX];
        int gradOrderNum[NB_CHAN_RECORDED_MAX];
    } m_structHeader;

    char* m_pStructHeader = nullptr;

    struct
    {
        int sampleNumber;
        int nbSamplesPerChanPerBlock;
        int nbSamplesTotPerBlock;
        signed int data[NB_SAMP_ACQ_Packet];
    } m_structBuffData;

    char* m_pStructBuffData = nullptr;

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;

    uint32_t m_indexIn     = 0;
    uint32_t m_indexOut    = 0;
    uint32_t m_socketFlag  = 0;
    uint32_t m_buffDataIdx = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
