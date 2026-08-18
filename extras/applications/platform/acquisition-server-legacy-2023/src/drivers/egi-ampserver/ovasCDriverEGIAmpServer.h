#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <socket/IConnectionClient.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CCommandConnectionHandler;

/**
 * \class CDriverEGIAmpServer
 * \author Yann Renard (Inria)
 */
class CDriverEGIAmpServer final : public IDriver
{
public:
    friend class CCommandConnectionHandler;

    explicit CDriverEGIAmpServer(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "EGI Net Amps 300 (through AmpServer)"; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:
    bool execute(const char* sScriptFilename);

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    Socket::IConnectionClient* m_command = nullptr;
    Socket::IConnectionClient* m_stream  = nullptr;
    uint32_t m_nSamplePerSentBlock       = 0;
    uint32_t m_sampleIdx                 = 0;
    uint32_t m_nChannel                  = 0;
    float* m_buffer                      = nullptr;

    CString m_serverHostName = "localhost";
    uint32_t m_commandPort   = 9877;
    uint32_t m_streamPort    = 9879;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
