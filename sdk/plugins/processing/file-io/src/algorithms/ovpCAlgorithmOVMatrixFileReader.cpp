// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "ovpCAlgorithmOVMatrixFileReader.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

bool CAlgorithmOVMatrixFileReader::initialize()
{
    ip_sFilename.initialize(getInputParameter(OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename));
    op_pMatrix.initialize(getOutputParameter(OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix));
    return true;
}

bool CAlgorithmOVMatrixFileReader::uninitialize()
{
    op_pMatrix.uninitialize();
    ip_sFilename.uninitialize();
    return true;
}

bool CAlgorithmOVMatrixFileReader::process()
{
    OV_ERROR_UNLESS_KRF(Toolkit::Matrix::loadFromTextFile(*op_pMatrix, ip_sFilename->toASCIIString()),
                        "the matrix file [" << *ip_sFilename << "] could not be read. Perhaps it does not exist, or is not a matrix. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadFileRead);

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
