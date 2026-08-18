#include "ovasCDriverGenericRawTelnetReader.h"
#include "ovasParameterHelper.h"

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericRawTelnetReader::CDriverGenericRawTelnetReader(IDriverContext& ctx)
    : CDriverGenericRawReader(ctx),
    m_hostName("HostName", "The name of the host to connect to", "localhost"),
    m_hostPort("HostPort", "The port of the host to connect to", 1337, 0, 32767)
{
    m_parameters.push_back(&m_hostName);
    m_parameters.push_back(&m_hostPort);

    loadParametersFromConfig(m_driverCtx.getConfigurationManager(),
                             m_parameters,
                             "AcquisitionServer_Driver_GenericRawTelnetReader",
                             &m_header);

}

bool CDriverGenericRawTelnetReader::configure()
{
    CString filename;

    return saveParametersToConfig(m_driverCtx.getConfigurationManager(),
                                  m_parameters,
                                  "AcquisitionServer_Driver_GenericRawTelnetReader",
                                  &m_header);
}

bool CDriverGenericRawTelnetReader::open()
{
    m_connection = Socket::createConnectionClient();
    if (!m_connection) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not create client connection\n";
        return false;
    }
    if (!m_connection->connect(m_hostName.value().c_str(), m_hostPort)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not connect to server [" << m_hostName.value() << ":" << m_hostPort.value() << "]\n";
        return false;
    }
    char* buffer = new char[m_startSkip];
    if (m_startSkip > 0 && !m_connection->receiveBufferBlocking(buffer, m_startSkip)) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to skip " << m_startSkip.value() << " bytes at the beginning\n";
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
