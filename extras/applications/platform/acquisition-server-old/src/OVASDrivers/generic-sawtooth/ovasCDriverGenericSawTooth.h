#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGenericSawTooth 
 * \author Yann Renard (INRIA)
 */
class CDriverGenericSawTooth final : public IDriver
{
public:
    explicit CDriverGenericSawTooth(IDriverContext& ctx);
    void release();
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override;
    bool configure() override;

protected:
    IDriverCallback* m_callback = nullptr;

    uint32_t m_externalBlockSize = 0;
    std::vector<float> m_samples;

    uint64_t m_nTotalSample = 0;
    uint64_t m_startTime    = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
