#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGenericRawReader
 * \author Yann Renard (INRIA)
 */
class CDriverGenericRawReader : public IDriver
{
public:
    enum EParameter
    {
        Endian_Little = 0,
        Endian_Big = 1,

        Format_UnsignedInteger8 = 0,
        Format_UnsignedInteger16 = 1,
        Format_UnsignedInteger32 = 2,
        Format_SignedInteger8 = 3,
        Format_SignedInteger16 = 4,
        Format_SignedInteger32 = 5,
        Format_Float32 = 6,
        Format_Float64 = 7,
    };

    explicit CDriverGenericRawReader(IDriverContext& ctx);
    virtual void release() { delete this; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    const IHeader* getHeader() override { return &m_header; }

protected:
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool read() = 0;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;
    uint32_t m_nSamplePerSentBlock = 0;
    uint32_t m_sampleSize          = 0;
    uint32_t m_sampleFormat        = Format_SignedInteger32;
    uint32_t m_sampleEndian        = 0;
    uint32_t m_startSkip           = 0;
    uint32_t m_headerSkip          = 0;
    uint32_t m_footerSkip          = 20;
    uint32_t m_dataFrameSize       = 0;
    bool m_limitSpeed              = false;
    uint8_t* m_dataFrame           = nullptr;
    float* m_sample                = nullptr;
    uint64_t m_startTime           = 0;
    uint64_t m_nTotalSample        = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
