#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include "ovasCSmartingAmp.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverMBTSmarting
 * \author mBrainTrain dev team (mBrainTrain)
 * \date Tue Oct 14 16:09:43 2014
 * \brief The CDriverMBTSmarting allows the acquisition server to acquire data from a MBTSmarting device.
 *
 * TODO: details
 *
 * \sa CConfigurationMBTSmarting
 */
class CDriverMBTSmarting final : public IDriver
{
public:

    explicit CDriverMBTSmarting(IDriverContext& ctx);
    ~CDriverMBTSmarting() override {}
    const char* getName() override { return "mBrainTrain Smarting"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; } // change to false if your device is not configurable
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

#ifdef TARGET_OS_Windows
    bool isFlagSet(const EDriverFlag /*flag*/) const override { return false; }
#elif defined TARGET_OS_Linux
        bool isFlagSet(const EDriverFlag /*flag*/) const override { return true; }
#endif

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;

    // Replace this generic Header with any specific header you might have written
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;

private:

    /*
     * Insert here all specific attributes, such as USB port number or device ID.
     * Example :
     */
    uint32_t m_connectionID = 1;
    std::shared_ptr<SmartingAmp> m_pSmartingAmp;
    std::vector<unsigned char> m_byteArray;
    int sample_number = 0;
    int latency       = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
