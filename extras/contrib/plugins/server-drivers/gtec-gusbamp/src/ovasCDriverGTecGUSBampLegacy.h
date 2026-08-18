#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <gtk/gtk.h>
#include <vector>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGTecGUSBampLegacy
 * \author Yann Renard (Inria)
 * \date unknown
 * \brief GTEC driver 
 *
 */
class CDriverGTecGUSBampLegacy final : public IDriver
{
public:

    explicit CDriverGTecGUSBampLegacy(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "g.tec gUSBamp Legacy"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsDeprecated; }

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    uint32_t m_deviceIdx           = 0;
    uint32_t m_actualDeviceIdx     = 0;
    uint32_t m_bufferSize          = 0;
    uint8_t* m_Buffer              = nullptr;
    float* m_sampleTranspose       = nullptr;
    float* m_sample                = nullptr;
    void* m_Device                 = nullptr;
    void* m_pEvent                 = nullptr;
    void* m_pOverlapped            = nullptr;

    uint32_t m_actualImpedanceIdx = 0;

    uint8_t m_commonGndAndRefBitmap = 0;

    int m_notchFilterIdx    = 0;
    int m_bandPassFilterIdx = 0;

    bool m_bTriggerInputEnabled = false;
    uint32_t m_lastStimulation  = 0;

    // EVENT CHANNEL : contribution Anton Andreev (Gipsa-lab) - 0.14.0
    typedef enum
    {
        STIMULATION_0 = 0,
        STIMULATION_64 = 64,
        STIMULATION_128 = 128,
        STIMULATION_192 = 192
    } gtec_triggers_t;

    uint32_t m_totalHardwareStimulations = 0; //since start button clicked
    uint32_t m_totalDriverChunksLost     = 0; //since start button clicked
    uint32_t m_nAcquiredChannel          = 0; //number of channels 1..16 specified bu user
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
