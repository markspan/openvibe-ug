#pragma once

#ifdef TARGET_OS_Windows

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CDriverMensiaAcquisition final : public IDriver
{
public:
    CDriverMensiaAcquisition(IDriverContext& ctx, const char* driverID);
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

protected:
    SettingsHelper m_settings;
    bool m_valid = false;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;

    uint32_t m_nTotalSample = 0;
    uint64_t m_startTime    = 0;

    uint32_t* m_sampleCountPerBuffer = nullptr;

private:
    // Settings
    CString m_deviceURL;

    template <typename T>
    void loadDLLfunct(T* pointer, const char* name);
    uint32_t m_driverID = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
