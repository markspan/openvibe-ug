#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverCognionics
 * \author Mike Chi (Cognionics, Inc.)
 * \copyright AGPL3
 * \date Thu Apr 18 21:19:49 2013
 
 * \brief The CDriverCognionics allows the acquisition server to acquire data from a Cognionics device.
 *
 * TODO: details
 *
 * \sa CConfigurationCognionics
 */
class CDriverCognionics final : public IDriver
{
public:

    explicit CDriverCognionics(IDriverContext& ctx);
    ~CDriverCognionics() override {}
    const char* getName() override { return "Cognionics"; }

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

    // Replace this generic Header with any specific header you might have written
    CHeader m_header;
    uint32_t m_nSamplePerSentBlock = 0;
    float* m_sample                = nullptr;

private:

    /*
     * Insert here all specific attributes, such as USB port number or device ID.
     * Example :
     */
    uint32_t m_comPort = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
