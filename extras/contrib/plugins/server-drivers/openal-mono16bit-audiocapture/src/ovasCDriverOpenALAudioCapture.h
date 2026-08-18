#pragma once

#if defined TARGET_HAS_ThirdPartyOpenAL

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows
#include <al.h>
#include <alc.h>
#elif defined TARGET_OS_Linux
    #include <AL/al.h>
    #include <AL/alc.h>
#elif defined TARGET_OS_MacOS
  #include "al.h"
  #include "alc.h"
#endif

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverOpenALAudioCapture
 * \author Aurelien Van Langhenhove (CIC-IT Garches)
 * \date Mon May 16 16:55:49 2011
 * \erief The CDriverOpenALAudioCapture allows the acquisition server to acquire data from a OpenAL audio capture device.
 *
 * TODO: details
 *
 * \sa CConfigurationOpenALAudioCapture
 */
class CDriverOpenALAudioCapture final : public IDriver
{
public:

    explicit CDriverOpenALAudioCapture(IDriverContext& ctx);
    ~CDriverOpenALAudioCapture() override;
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override;
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;

    // Replace this generic Header with any specific header you might have written
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;
    ALshort* Samples               = nullptr;

private:

    /*
     * Insert here all specific attributes, such as USB port number or device ID.
     * Example :
     */
    // uint32_t m_connectionID;
    ALCdevice* Device        = nullptr;
    ALCcontext* Context      = nullptr;
    ALCdevice* CaptureDevice = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif //TARGET_HAS_ThirdPartyOpenAL
