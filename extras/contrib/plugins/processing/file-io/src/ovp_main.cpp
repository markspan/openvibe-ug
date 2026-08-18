#include <vector>
#include <openvibe/ov_all.h>

// @BEGIN CICIT-GARCHES
#include "box-algorithms/ovpCBoxAlgorithmEDFFileWriter.h"
// @END CICIT-GARCHES

// @BEGIN GIPSA
#include "box-algorithms/ovpCBoxAlgorithmBrainampFileWriterGipsa.h"
// @END GIPSA

#include "box-algorithms/ovpCBoxAlgorithmEDFFileReader.hpp"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

OVP_Declare_Begin()
    // @BEGIN CICIT-GARCHES
    OVP_Declare_New(CBoxAlgorithmEDFFileWriterDesc)
    // @END CICIT_GARCHES

    // @BEGIN GIPSA
    //Register dropdowns
    context.getTypeManager().registerEnumerationType(OVP_TypeId_BinaryFormat, "Binary format select");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_BinaryFormat, "INT_16", OVP_TypeId_BinaryFormat_int16_t.id());
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_BinaryFormat, "UINT_16", OVP_TypeId_BinaryFormat_uint16_t.id());
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_BinaryFormat, "IEEE_FLOAT_32", OVP_TypeId_BinaryFormat_float.id());

    OVP_Declare_New(CBoxAlgorithmBrainampFileWriterGipsaDesc)
    // @END GIPSA

    OVP_Declare_New(CBoxAlgorithmEDFFileReaderDesc)

OVP_Declare_End()

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
