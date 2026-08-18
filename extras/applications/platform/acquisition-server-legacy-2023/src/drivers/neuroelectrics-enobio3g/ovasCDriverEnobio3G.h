#pragma once

#if defined(TARGET_HAS_ThirdPartyEnobioAPI)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// Including Enobio headers gave 4275 on 11.07.2014 w/ VS2010
#pragma warning(disable:4275)

#include "enobio3g.h"
#include "StatusData.h"
#include <mutex>

#ifndef _ENOBIO_SAMPLE_RATE_
#ifdef FREQ_SAMP
  #define _ENOBIO_SAMPLE_RATE_ FREQSAMP
#else
#define _ENOBIO_SAMPLE_RATE_ 500
#endif
#endif

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverEnobio3G
 * \author Anton Albajes-Eizagirre (NeuroElectrics) anton.albajes-eizagirre@neuroelectrics.com
 * \date Tue Apr 15 09:25:20 2014
 * \brief The CDriverEnobio3G allows the acquisition server to acquire data from a Enobio3G device.
 *
 * TODO: details
 *
 * \sa CConfigurationEnobio3G
 */
class CDriverEnobio3G final : public IDriver, public IDataConsumer
{
public:

    CDriverEnobio3G(IDriverContext& ctx);
    ~CDriverEnobio3G() override;
    const char* getName() override { return "Enobio3G"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

    // enobio registered consumers callbacks
    void receiveData(const PData& data);
    void newStatusFromDevice(const PData& data);

protected:
    SettingsHelper m_settings;
    IDriverCallback* m_callback = nullptr;

    // Replace this generic Header with any specific header you might have written
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    // sample buffers. We will have a set of buffers that will be cycled. 
    float** m_sample = nullptr;

private:

    /*
     * Insert here all specific attributes, such as USB port number or device ID.
     * Example :
     */
    uint32_t m_nChannels        = 0; // Number of channels on the device reported by the device
    unsigned char* m_macAddress = nullptr; // mac address of the device
    Enobio3G m_enobioDevice; // Enobio device class instantiation
    uint32_t m_sampleRate       = 0; // sampling rate of the device
    uint32_t m_bufHead          = 0; // writing header for the current buffer
    uint32_t m_nBuffers         = 0; // number of buffers
    uint32_t m_currentBuffer    = 0; // current buffer in use
    uint32_t m_lastBufferFilled = 0; // last buffer filled with data ready to be submitted
    bool m_newData              = false; // if there is a new buffer with data ready to be submitted

    std::mutex m_mutex;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
