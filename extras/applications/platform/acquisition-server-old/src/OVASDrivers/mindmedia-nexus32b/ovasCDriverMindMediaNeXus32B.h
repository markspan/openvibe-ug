#pragma once

#if defined(TARGET_HAS_ThirdPartyNeXus)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverMindMediaNeXus32B
 * \author Yann Renard (INRIA)
 */
class CDriverMindMediaNeXus32B : public IDriver
{
public:

    CDriverMindMediaNeXus32B(IDriverContext& ctx);
    virtual void release();
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    virtual void processData(uint32_t sampleCount, uint32_t channel, float* sample);

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;
    uint32_t m_sampleIdx           = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // defined TARGET_OS_Windows
#endif
