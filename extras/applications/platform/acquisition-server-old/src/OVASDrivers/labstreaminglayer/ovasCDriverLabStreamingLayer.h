#pragma once

#if defined(TARGET_HAS_ThirdPartyLSL)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include <lsl_cpp.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverLabStreamingLayer
 * \author Jussi T. Lindgren / Inria
 * \date Wed Oct 15 09:41:18 2014
 * \brief The CDriverLabStreamingLayer allows the acquisition server to acquire data from a LabStreamingLayer (LSL) device.
 *
 * \sa CConfigurationLabStreamingLayer
 */
class CDriverLabStreamingLayer final : public IDriver
{
public:
    explicit CDriverLabStreamingLayer(IDriverContext& ctx);
    ~CDriverLabStreamingLayer() override;
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

    CHeader m_header;

private:
    uint32_t m_nSamplePerSentBlock = 0;
    uint32_t m_fallbackSampling    = 0;
    float* m_sample                = nullptr;
    float* m_buffer                = nullptr;

    uint64_t m_startTime = 0;
    uint64_t m_nSample   = 0;

    lsl::stream_info m_oSignalStream;
    lsl::stream_inlet* m_pSignalInlet = nullptr;

    lsl::stream_info m_oMarkerStream;
    lsl::stream_inlet* m_pMarkerInlet = nullptr;

    bool m_limitSpeed = false;
    CString m_sSignalStream;
    CString m_sSignalStreamID;
    CString m_sMarkerStream;
    CString m_sMarkerStreamID;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
