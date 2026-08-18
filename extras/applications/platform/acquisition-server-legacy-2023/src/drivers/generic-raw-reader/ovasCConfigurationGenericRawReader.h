#pragma once

#include "../ovasCConfigurationNetworkBuilder.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationGenericRawReader final : public CConfigurationNetworkBuilder
{
public:
    CConfigurationGenericRawReader(const char* gtkBuilderFilename, bool& limitSpeed, uint32_t& sampleFormat, uint32_t& sampleEndian,
                                   uint32_t& startSkip, uint32_t& headerSkip, uint32_t& footerSkip, CString& filename);

protected:
    bool preConfigure() override;
    bool postConfigure() override;

    bool& m_limitSpeed;
    uint32_t& m_sampleFormat;
    uint32_t& m_sampleEndian;
    uint32_t& m_startSkip;
    uint32_t& m_headerSkip;
    uint32_t& m_footerSkip;
    CString& m_filename;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
