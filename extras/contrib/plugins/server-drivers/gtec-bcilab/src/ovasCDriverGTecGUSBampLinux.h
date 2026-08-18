#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI_Linux

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <gAPI.h>
#include "Queue.h"

namespace OpenViBE {
namespace AcquisitionServer {
void OnDataReady(void* param);

/**
 * \class CDriverGTecGUSBampLinux
 * \author Tom Stewart (University of Tsukuba)
 * \date Mon Feb  9 18:59:22 2015
 * \brief The CDriverGTecGUSBampLinux allows the acquisition server to acquire data from a g.tec g.USBamp from Linux.
 *
 * \sa CConfigurationGTecGUSBampLinux
 */
class CDriverGTecGUSBampLinux final : public IDriver
{
    static const int ReceiveBufferSize = 8192;
public:
    friend void OnDataReady(void* param);

    explicit CDriverGTecGUSBampLinux(IDriverContext& ctx);
    ~CDriverGTecGUSBampLinux() override {}
    const char* getName() override { return "g.tec g.USBamp Linux BCI-Lab"; }

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

    uint32_t m_nSamplePerSentBlock;

    float *m_sampleSend, *m_sampleReceive, *m_sampleBuffer;
    Queue<float> m_sampleQueue;
private:

    /*
     * Insert here all specific attributes, such as USB port number or device ID.
     */
    std::string m_deviceName;
    gt_usbamp_config m_config;
    gt_usbamp_analog_out_config m_analogOutConfig;

    // Keeps track of where we are with filling up the buffer
    uint32_t m_currentSample, m_currentChannel;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI_Linux
