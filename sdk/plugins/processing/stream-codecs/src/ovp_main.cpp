#include "algorithms/decoders/ovpCAcquisitionDecoder.h"
#include "algorithms/decoders/ovpCStreamStructureDecoder.h"
#include "algorithms/decoders/ovpCExperimentInfoDecoder.h"
#include "algorithms/decoders/ovpCChannelLocalisationDecoder.h"
#include "algorithms/decoders/ovpCChannelUnitsDecoder.h"
#include "algorithms/decoders/ovpCFeatureVectorDecoder.h"
#include "algorithms/decoders/ovpCSignalDecoder.h"
#include "algorithms/decoders/ovpCSpectrumDecoder.h"
#include "algorithms/decoders/ovpCStimulationDecoder.h"
#include "algorithms/decoders/ovpCStreamedMatrixDecoder.h"
// #include "algorithms/decoders/ovpCMasterAcquisitionDecoder.h"

#include "algorithms/encoders/ovpCAcquisitionEncoder.h"
#include "algorithms/encoders/ovpCExperimentInfoEncoder.h"
#include "algorithms/encoders/ovpCChannelLocalisationEncoder.h"
#include "algorithms/encoders/ovpCChannelUnitsEncoder.h"
#include "algorithms/encoders/ovpCFeatureVectorEncoder.h"
#include "algorithms/encoders/ovpCSignalEncoder.h"
#include "algorithms/encoders/ovpCSpectrumEncoder.h"
#include "algorithms/encoders/ovpCStimulationEncoder.h"
#include "algorithms/encoders/ovpCStreamedMatrixEncoder.h"
#include "algorithms/encoders/ovpCMasterAcquisitionEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

OVP_Declare_Begin()
    OVP_Declare_New(CAcquisitionDecoderDesc)
    OVP_Declare_New(CStreamStructureDecoderDesc)
    OVP_Declare_New(CExperimentInfoDecoderDesc)
    OVP_Declare_New(CChannelLocalisationDecoderDesc)
    OVP_Declare_New(CChannelUnitsDecoderDesc)
    OVP_Declare_New(CFeatureVectorDecoderDesc)
    OVP_Declare_New(CSignalDecoderDesc)
    OVP_Declare_New(CSpectrumDecoderDesc)
    OVP_Declare_New(CStimulationDecoderDesc)
    OVP_Declare_New(CStreamedMatrixDecoderDesc)
    // OVP_Declare_New(CMasterAcquisitionDecoderDesc)

    OVP_Declare_New(CAcquisitionEncoderDesc)
    OVP_Declare_New(CExperimentInfoEncoderDesc)
    OVP_Declare_New(CChannelLocalisationEncoderDesc)
    OVP_Declare_New(CChannelUnitsEncoderDesc)
    OVP_Declare_New(CFeatureVectorEncoderDesc)
    OVP_Declare_New(CSignalEncoderDesc)
    OVP_Declare_New(CSpectrumEncoderDesc)
    OVP_Declare_New(CStimulationEncoderDesc)
    OVP_Declare_New(CStreamedMatrixEncoderDesc)
    OVP_Declare_New(CMasterAcquisitionEncoderDesc)

OVP_Declare_End()

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
