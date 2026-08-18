#pragma once

#include "ovasCConfigurationBuilder.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationNetworkBuilder : public CConfigurationBuilder
{
public:
    explicit CConfigurationNetworkBuilder(const char* gtkBuilderFileName) : CConfigurationBuilder(gtkBuilderFileName) {}
    ~CConfigurationNetworkBuilder() override {}


    virtual bool setHostName(const CString& hostName);
    virtual bool setHostPort(uint32_t hostPort);

    virtual CString getHostName() const { return m_hostName; }
    virtual uint32_t getHostPort() const { return m_hostPort; }

protected:
    bool preConfigure() override;
    bool postConfigure() override;

    GtkWidget* m_pHostName = nullptr;
    GtkWidget* m_pHostPort = nullptr;

    CString m_hostName  = "localhost";
    uint32_t m_hostPort = 4000;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
