#pragma once

#if defined TARGET_HAS_ThirdPartyNeXus

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include "ovas_base.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include "ovasCConfigurationTMSIRefa32B.h"

// This dll comes from the sdk-nexus.zip dependency archive
#define RTLOADER "RTINST.Dll"
#include <gtk/gtk.h>
// Get Signal info

#define SIGNAL_NAME 40
#define MAX_BUFFER_SIZE 0xFFFFFFFF

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverTMSiRefa32B
 * \author Baptiste Payan (INRIA)
 */
class CDriverTMSiRefa32B final : virtual public IDriver
{
public:

    explicit CDriverTMSiRefa32B(IDriverContext& ctx);
    ~CDriverTMSiRefa32B() override { this->uninitialize(); }
    virtual void release() { delete this; }
    const char* getName() override { return "TMSi Refa32B Legacy"; }

    bool isFlagSet(const EDriverFlag flag) const override { return (flag == EDriverFlag::IsUnstable) || (flag == EDriverFlag::IsDeprecated); }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }
    static bool measureMode(uint32_t mode, uint32_t info);

protected:
    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;
    bool m_valid                   = true;

    uint32_t m_sampleIdx           = 0;
    uint32_t m_totalSampleReceived = 0;
    CStimulationSet m_stimSet;

    bool m_checkImpedance       = false;
    int m_nTriggerChannel       = -1;
    uint32_t m_lastTriggerValue = 255;

    bool refreshDevicePath() const;


#define EXG (ULONG) 0x0001
#define AUX (ULONG) 0x0002
#define DEVICE_FEATURE_TYPE         0x0303
#define DEVICE_FEATURE_MODE         0x0302
#define DEVICE_FEATURE_RTC          0x0301
#define DEVICE_FEATURE_HIGHPASS     0x0401
#define DEVICE_FEATURE_LOWPASS      0x0402
#define DEVICE_FEATURE_GAIN         0x0403
#define DEVICE_FEATURE_OFFSET       0x0404
#define DEVICE_FEATURE_IO           0x0500
#define DEVICE_FEATURE_MEMORY       0x0501
#define DEVICE_FEATURE_STORAGE      0x0502
#define DEVICE_FEATURE_CORRECTION   0x0503
#define DEVICE_FEATURE_ID           0x0504

#define MEASURE_MODE_NORMAL         ((ULONG)0x0)
#define MEASURE_MODE_IMPEDANCE      ((ULONG)0x1)
#define MEASURE_MODE_CALIBRATION    ((ULONG)0x2)
#define MEASURE_MODE_IMPEDANCE_EX   ((ULONG)0x3)
#define MEASURE_MODE_CALIBRATION_EX ((ULONG)0x4)
    //for MEASURE_MODE_IMPEDANCE
#define IC_OHM_002 0    // 2K Impedance limit
#define IC_OHM_005 1    // 5K Impedance limit
#define IC_OHM_010 2    // 10K Impedance limit
#define IC_OHM_020 3    // 20K Impedance limit
#define IC_OHM_050 4    // 50K Impedance limit
#define IC_OHM_100 5    // 100K Impedance limit
    //for MEASURE_MODE_CALIBRATION
#define IC_VOLT_050 0   //50 uV t-t Calibration voltage
#define IC_VOLT_100 1   //100 uV t-t
#define IC_VOLT_200 2   //200 uV t-t
#define IC_VOLT_500 3   //500 uV t-t
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
