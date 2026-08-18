#include "ovpCFeatureVectorEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CFeatureVectorEncoder::processHeader()
{
    OV_ERROR_UNLESS_KRF(ip_pMatrix->getDimensionCount() == 1, "Invalid feature vector: found " << ip_pMatrix->getDimensionCount() << " dimensions, 1 expected",
                        Kernel::ErrorType::BadInput);

    CStreamedMatrixEncoder::processHeader();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
