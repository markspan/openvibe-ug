#pragma once

#include "ovasCDriverGenericRawReader.h"

#include <socket/IConnectionClient.h>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGenericRawTelnetReader
 * \author Yann Renard (INRIA)
 */
class CDriverGenericRawTelnetReader final : public CDriverGenericRawReader
{
public:
    explicit CDriverGenericRawTelnetReader(IDriverContext& ctx);

    const char* getName() override { return "Generic Raw Telnet Reader"; }
    bool isConfigurable() override { return true; }
    bool configure() override;

protected:
    bool open() override;
    bool close() override;
    bool read() override;

    Socket::IConnectionClient* m_connection = nullptr;
    OpenViBE::Parameters::ParameterString m_hostName;
    OpenViBE::Parameters::ParameterInt m_hostPort;
    //CString m_hostName                      = "localhost";
    
    //uint32_t m_hostPort                     = 1337;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
