#pragma once

#if defined(TARGET_HAS_ThirdPartyMCS)

#include <vector>
#include "ovasIDriver.h"
#include "../../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../../ovasCSettingsHelper.h"
#include "../../ovasCSettingsHelperOperators.h"
#include "NVX.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverMKSNVXDriver
 * \author mkochetkov (MKS)
 * \date Tue Jan 21 23:21:03 2014
 * \brief The CDriverMKSNVXDriver allows the acquisition server to acquire data from a MKSNVXDriver device.
 *
 * TODO: details
 *
 * \sa CConfigurationMKSNVXDriver
 */
const size_t maxNumberOfChannels = 36;

class CDriverMKSNVXDriver : public IDriver
{
    uint8_t deviceBuffer_[1024 * maxNumberOfChannels];
    int nvxDeviceId_ = -1;
    t_NVXDataSettins nvxDataSettings_;
    int nvxDataModel_  = 0;
    uint32_t dataMode_ = 0; // normal, test, impedance
    t_NVXProperty nvxProperty_;
    uint32_t samplesCounter_ = 0; // previous t_NVXDataModelXX.Counter
    uint32_t triggerStates_  = 0;
    bool showAuxChannels_    = false;
    t_NVXInformation nvxInfo_;
public:

    CDriverMKSNVXDriver(IDriverContext& ctx);
    size_t getDeviceBufferSamplesCapacity() const
    {
        return sizeof(deviceBuffer_) / sizeof(t_NVXDataModel36) * maxNumberOfChannels;
    }  // I really do not know if it is accurate for all modes.
    ~CDriverMKSNVXDriver() override;
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
    std::vector<float> sampleData_;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE


#endif
