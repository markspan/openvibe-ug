#include "ovasCDriverGenericRawTelnetReader.h"
#include "ovasCConfigurationGenericRawReader.h"

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericRawTelnetReader::CDriverGenericRawTelnetReader(IDriverContext& ctx)
    : CDriverGenericRawReader(ctx), m_settings("AcquisitionServer_Driver_GenericRawTelnetReader", m_driverCtx.getConfigurationManager())
{
    m_hostName = "localhost";
    m_hostPort = 1337;

    // Relay configuration properties to the configuration manager
    m_settings.add("Header", &m_header);
    m_settings.add("LimitSpeed", &m_limitSpeed);
    m_settings.add("SampleFormat", &m_sampleFormat);
    m_settings.add("SampleEndian", &m_sampleEndian);
    m_settings.add("StartSkip", &m_startSkip);
    m_settings.add("HeaderSkip", &m_headerSkip);
    m_settings.add("FooterSkip", &m_footerSkip);
    m_settings.add("HostName", &m_hostName);
    m_settings.add("HostPort", &m_hostPort);
    m_settings.load();
}

bool CDriverGenericRawTelnetReader::configure()
{
    CString filename;
    CConfigurationGenericRawReader config(Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-RawTelnetReader.ui",
                                          m_limitSpeed, m_sampleFormat, m_sampleEndian, m_startSkip, m_headerSkip, m_footerSkip, filename);

    config.setHostName(m_hostName);
    config.setHostPort(m_hostPort);

    if (!config.configure(m_header)) { return false; }

    m_hostName = config.getHostName();
    m_hostPort = config.getHostPort();

    m_settings.save();

    return true;
}

bool CDriverGenericRawTelnetReader::open()
{
    m_connection = Socket::createConnectionClient();
    if (!m_connection) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not create client connection\n";
        return false;
    }
    if (!m_connection->connect(m_hostName.toASCIIString(), m_hostPort)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not connect to server [" << m_hostName << ":" << m_hostPort << "]\n";
        return false;
    }
    char* buffer = new char[m_startSkip];
    if (m_startSkip > 0 && !m_connection->receiveBufferBlocking(buffer, m_startSkip)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to skip " << m_startSkip << " bytes at the beginning\n";
        delete[] buffer;
        return false;
    }
    delete[] buffer;

    return true;
}

bool CDriverGenericRawTelnetReader::close()
{
    if (m_connection) {
        m_connection->close();
        m_connection->release();
        m_connection = nullptr;
    }
    return true;
}

bool CDriverGenericRawTelnetReader::read()
{
    if (!m_connection) { return false; }
    return m_connection->receiveBufferBlocking(m_dataFrame, m_dataFrameSize);
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
