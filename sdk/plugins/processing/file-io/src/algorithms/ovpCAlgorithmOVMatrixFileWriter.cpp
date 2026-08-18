#include "ovpCAlgorithmOVMatrixFileWriter.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

bool CAlgorithmOVMatrixFileWriter::initialize()
{
    ip_sFilename.initialize(getInputParameter(OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Filename));
    ip_pMatrix.initialize(getInputParameter(OVP_Algorithm_OVMatrixFileWriter_InputParameterId_Matrix));
    return true;
}

bool CAlgorithmOVMatrixFileWriter::uninitialize()
{
    ip_sFilename.uninitialize();
    ip_pMatrix.uninitialize();
    return true;
}

bool CAlgorithmOVMatrixFileWriter::process()
{
    OV_ERROR_UNLESS_KRF(Toolkit::Matrix::saveToTextFile(*ip_pMatrix, ip_sFilename->toASCIIString()),
                        "Writing matrix file " << *ip_sFilename << " failed", Kernel::ErrorType::BadFileWrite);

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
