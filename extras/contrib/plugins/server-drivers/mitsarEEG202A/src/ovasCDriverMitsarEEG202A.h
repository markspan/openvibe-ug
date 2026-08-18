#pragma once

#if defined(TARGET_HAS_ThirdPartyMitsar)
#if defined TARGET_OS_Windows

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverMitsarEEG202A
 * \author Gelu Ionescu (GIPSA-lab)
 * \date 26 April 2012
 * \brief The CDriverMitsarEEG202A allows the acquisition server to acquire data from a Mitsar EEG 202A amplifier.
 *
 * submitted by Anton Andreev (GIPSA-lab)
 */
class CDriverMitsarEEG202A final : public IDriver
{
    typedef enum
    {
        CHANNEL_NB = 33,
        SAMPLING_RATE = 500,
        SAMPLES_NB = 33,
        STIMULATION_0 = 0,
        STIMULATION_128 = 128,
        STIMULATION_64 = 64,
        STIMULATION_192 = (STIMULATION_128 + STIMULATION_64), //both buttons pressed
    } misc_t;

public:

    explicit CDriverMitsarEEG202A(IDriverContext& ctx);//modif new Idriver
    void release();
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override;
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    //??? virtual void processData (cf neXus)

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_refIdx               = 0;
    bool m_eventAndBioChannelsState = false;

    uint32_t m_lastStimulation  = 0;
    float* m_stimulationChannel = nullptr;
    std::vector<size_t> m_stimulationIDs;
    std::vector<size_t> m_stimulationDates;

    std::vector<float> m_iSamples;
    std::vector<float> m_oSamples;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
#endif
