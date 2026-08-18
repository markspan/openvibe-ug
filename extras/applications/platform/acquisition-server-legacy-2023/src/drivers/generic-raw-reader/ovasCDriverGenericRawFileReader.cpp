#include "ovasCDriverGenericRawFileReader.h"
#include "ovasCConfigurationGenericRawReader.h"

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericRawFileReader::CDriverGenericRawFileReader(IDriverContext& ctx)
    : CDriverGenericRawReader(ctx), m_settings("AcquisitionServer_Driver_GenericRawFileReader", m_driverCtx.getConfigurationManager())
{
    m_filename = "/tmp/some_raw_file";

    // Relay configuration properties to the configuration manager

    m_settings.add("Header", &m_header);
    m_settings.add("LimitSpeed", &m_limitSpeed);
    m_settings.add("SampleFormat", &m_sampleFormat);
    m_settings.add("SampleEndian", &m_sampleEndian);
    m_settings.add("StartSkip", &m_startSkip);
    m_settings.add("HeaderSkip", &m_headerSkip);
    m_settings.add("FooterSkip", &m_footerSkip);
    m_settings.add("FileName", &m_filename);
    m_settings.load();
}

bool CDriverGenericRawFileReader::configure()
{
    CConfigurationGenericRawReader config(Directories::getDataDir() + "/applications/acquisition-server/interface-Generic-RawFileReader.ui", m_limitSpeed,
                                          m_sampleFormat, m_sampleEndian, m_startSkip, m_headerSkip, m_footerSkip, m_filename);

    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    return true;
}

bool CDriverGenericRawFileReader::open()
{
    m_file = fopen(m_filename.toASCIIString(), "rb");
    if (!m_file) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open file [" << m_filename << "]\n";
        return false;
    }
    if (fseek(m_file, m_startSkip, SEEK_SET) != 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not seek to " << m_startSkip << " bytes from the file beginning\n";
        fclose(m_file);
        return false;
    }
    return true;
}

bool CDriverGenericRawFileReader::close()
{
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
    return true;
}

bool CDriverGenericRawFileReader::read()
{
    if (!m_file) { return false; }
    const bool res = (fread(m_dataFrame, 1, m_dataFrameSize, m_file) == m_dataFrameSize);
    if (!res && feof(m_file)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "End of file reached.\n"; }
    return res;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
