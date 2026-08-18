#include "ovp_defines.h"
#include "box-algorithms/ovpCGDFFileReader.h"
#include "box-algorithms/ovpCGDFFileWriter.h"
#include "box-algorithms/ovpCBCICompetitionIIIbReader.h"

#include "algorithms/brainamp/ovpCAlgorithmBrainampFileReader.h"

#include "box-algorithms/brainamp/ovpCBoxAlgorithmBrainampFileReader.h"
#include "box-algorithms/brainamp/ovpCBoxAlgorithmBrainampFileWriter.h"

#include "box-algorithms/bci2000reader/ovpCBoxAlgorithmBCI2000Reader.h"

#include "box-algorithms/ovpCBoxAlgorithmSignalConcatenation.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(),
                                                      OV_AttributeId_Box_FlagIsUnstable.id());

    OVP_Declare_New(CGDFFileReaderDesc)
    OVP_Declare_New(CGDFFileWriterDesc)
    OVP_Declare_New(CBCICompetitionIIIbReaderDesc)
    OVP_Declare_New(CAlgorithmBrainampFileReaderDesc)
    OVP_Declare_New(CBoxAlgorithmBrainampFileReaderDesc)
    OVP_Declare_New(CBoxAlgorithmBrainampFileWriterDesc)
    OVP_Declare_New(CBoxAlgorithmBCI2000ReaderDesc)
    OVP_Declare_New(CBoxAlgorithmSignalConcatenationDesc)

OVP_Declare_End()

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
