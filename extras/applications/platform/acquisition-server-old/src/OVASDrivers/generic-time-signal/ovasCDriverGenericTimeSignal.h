#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGenericTimeSignal 
 * \author Jussi Lindgren (Inria)
 *
 * This driver may have some utility in debugging. For each sample, it returns the 
 * current time as obtained from openvibe's System::Time:zgettime() converted to float seconds.
 *
 */
class CDriverGenericTimeSignal final : public IDriver
{
public:
    explicit CDriverGenericTimeSignal(IDriverContext& ctx);
    void release();
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool configure() override;

protected:
    IDriverCallback* m_callback = nullptr;
    
    uint64_t m_nTotalSample = 0;
    uint64_t m_startTime    = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
