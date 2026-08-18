#pragma once

#if defined(TARGET_HAS_ThirdPartyLSL)

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"
#include "ovasIHeader.h"

#include <lsl_cpp.h>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationLabStreamingLayer
 * \author Jussi T. Lindgren / Inria
 * \date Wed Oct 15 09:41:18 2014
 * \brief The CConfigurationLabStreamingLayer handles the configuration dialog specific to the LabStreamingLayer (LSL) device.
 *
 * \sa CDriverLabStreamingLayer
 */
class CConfigurationLabStreamingLayer final : public CConfigurationBuilder
{
public:
    CConfigurationLabStreamingLayer(IDriverContext& ctx, const char* gtkBuilderFilename, IHeader& header, bool& limitSpeed, CString& signalStream,
                                    CString& signalStreamID, CString& markerStream, CString& markerStreamID, uint32_t& fallbackSampling);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    IDriverContext& m_driverCtx;

private:

    bool& m_limitSpeed;
    CString& m_signalStream;
    CString& m_signalStreamID;
    CString& m_markerStream;
    CString& m_markerStreamID;
    uint32_t& m_fallbackSampling;

    std::vector<lsl::stream_info> m_streams;
    std::vector<int> m_signalIdxs;
    std::vector<int> m_markerIdxs;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
