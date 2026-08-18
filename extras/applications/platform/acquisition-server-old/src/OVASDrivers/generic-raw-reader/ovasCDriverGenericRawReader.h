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

protected:
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool read() = 0;

    IDriverCallback* m_callback = nullptr;
    uint32_t m_nSamplePerSentBlock = 0;
    uint32_t m_sampleSize          = 0;

    OpenViBE::Parameters::ParameterBool m_limitSpeed;
    OpenViBE::Parameters::ParameterListString m_sampleFormat;
    OpenViBE::Parameters::ParameterListString m_sampleEndian;
    OpenViBE::Parameters::ParameterInt m_startSkip;
    OpenViBE::Parameters::ParameterInt m_headerSkip;
    OpenViBE::Parameters::ParameterInt m_footerSkip;

    unsigned long m_dataFrameSize  = 0;
    uint8_t* m_dataFrame           = nullptr;
    float* m_sample                = nullptr;
    uint64_t m_startTime           = 0;
    uint64_t m_nTotalSample        = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
