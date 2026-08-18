#include "ovasCDriverGenericRawFileReader.h"
#include "ovasParameterHelper.h"

namespace OpenViBE {
namespace AcquisitionServer {

CDriverGenericRawFileReader::CDriverGenericRawFileReader(IDriverContext& ctx)
    : CDriverGenericRawReader(ctx), 
    m_filename("Filename", "The name of the file to read", "", OpenViBE::Parameters::Read, {"All files (*.*)"})
{
    m_parameters.push_back(&m_filename);

    loadParametersFromConfig(m_driverCtx.getConfigurationManager(),
                             m_parameters,
                             "AcquisitionServer_Driver_GenericRawFileReader",
                             &m_header);
}

bool CDriverGenericRawFileReader::configure()
{
    saveParametersToConfig(m_driverCtx.getConfigurationManager(),
                           m_parameters,
                           "AcquisitionServer_Driver_GenericRawFileReader",
                           &m_header);
    return true;
}

bool CDriverGenericRawFileReader::open()
{
    m_file = fopen(m_filename.value().c_str(), "rb");
    if (!m_file) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open file [" << m_filename.value() << "]\n";
        return false;
    }
    if (fseek(m_file, m_startSkip, SEEK_SET) != 0) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not seek to " << m_startSkip.value() << " bytes from the file beginning\n";
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
