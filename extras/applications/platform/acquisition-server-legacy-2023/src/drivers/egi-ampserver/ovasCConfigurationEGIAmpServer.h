#pragma once

#include "../ovasCConfigurationBuilder.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationEGIAmpServer final : public CConfigurationBuilder
{
public:
    explicit CConfigurationEGIAmpServer(const char* gtkBuilderFilename) : CConfigurationBuilder(gtkBuilderFilename) { }
    ~CConfigurationEGIAmpServer() override { }

    void setHostName(const CString& hostName) { m_hostName = hostName; }
    void setCommandPort(const uint32_t commandPort) { m_commandPort = commandPort; }
    void setStreamPort(const uint32_t streamPort) { m_streamPort = streamPort; }

    CString getHostName() const { return m_hostName; }
    uint32_t getCommandPort() const { return m_commandPort; }
    uint32_t getStreamPort() const { return m_streamPort; }


protected:
    bool preConfigure() override;
    bool postConfigure() override;

private:
    CConfigurationEGIAmpServer() = delete;

protected:
    GtkWidget* m_pHostName    = nullptr;
    GtkWidget* m_pCommandPort = nullptr;
    GtkWidget* m_pStreamPort  = nullptr;
    CString m_hostName        = "localhost";
    uint32_t m_commandPort    = 9877;
    uint32_t m_streamPort     = 9879;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
