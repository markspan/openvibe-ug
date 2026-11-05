// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
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
                        "the matrix file [" << *ip_sFilename << "] could not be written. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
