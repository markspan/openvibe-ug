#pragma once


#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

//___________________________________________________________________//
//                                                                   //
// Basic includes                                                    //
//___________________________________________________________________//
//                                                                   //

#include "ovtk_defines.h"

//___________________________________________________________________//
//                                                                   //
// Tools                                                             //
//___________________________________________________________________//
//                                                                   //

#include "tools/ovtkMatrix.h"
#include "tools/ovtkStimulationSet.h"
#include "tools/ovtkString.h"

//___________________________________________________________________//
//                                                                   //
// Codecs                                                            //
//___________________________________________________________________//
//                                                                   //

#include "codecs/ovtkTCodec.h"
#include "codecs/ovtkTGenericCodec.h"
#include "codecs/encoders/ovtkTEncoder.h"
#include "codecs/decoders/ovtkTDecoder.h"

#include "codecs/decoders/ovtkTAcquisitionDecoder.h"

#include "codecs/decoders/ovtkTStreamStructureDecoder.h"

#include "codecs/encoders/ovtkTStimulationStreamEncoder.h"
#include "codecs/decoders/ovtkTStimulationStreamDecoder.h"

#include "codecs/encoders/ovtkTStreamedMatrixEncoder.h"
#include "codecs/decoders/ovtkTStreamedMatrixDecoder.h"

#include "codecs/encoders/ovtkTSignalEncoder.h"
#include "codecs/decoders/ovtkTSignalDecoder.h"

#include "codecs/encoders/ovtkTChannelLocalisationEncoder.h"
#include "codecs/decoders/ovtkTChannelLocalisationDecoder.h"

#include "codecs/encoders/ovtkTChannelUnitsEncoder.h"
#include "codecs/decoders/ovtkTChannelUnitsDecoder.h"

#include "codecs/encoders/ovtkTExperimentInfoEncoder.h"
#include "codecs/decoders/ovtkTExperimentInfoDecoder.h"

#include "codecs/encoders/ovtkTFeatureVectorEncoder.h"
#include "codecs/decoders/ovtkTFeatureVectorDecoder.h"

#include "codecs/encoders/ovtkTSpectrumEncoder.h"
#include "codecs/decoders/ovtkTSpectrumDecoder.h"

//___________________________________________________________________//
//                                                                   //
//                                                                   //
//___________________________________________________________________//
//                                                                   //

#include "ovtkIVector.h"
#include "ovtkIFeatureVector.h"
#include "ovtkIFeatureVectorSet.h"

#include "box-algorithms/ovtkTBoxAlgorithm.h"

#include "algorithms/ovtkTAlgorithm.h"
#include "algorithms/classification/ovtkCAlgorithmClassifier.h"
#include "algorithms/classification/ovtkCAlgorithmPairingStrategy.h"

#include "algorithms/scenario-io/ovtkCAlgorithmScenarioImporter.h"
#include "algorithms/scenario-io/ovtkCAlgorithmScenarioExporter.h"

//___________________________________________________________________//
//                                                                   //
//                                                                   //
//___________________________________________________________________//
//                                                                   //

namespace OpenViBE {
namespace Toolkit {
OVTK_API bool initialize(const Kernel::IKernelContext& ctx);
OVTK_API bool uninitialize(const Kernel::IKernelContext& ctx);

bool initializeStimulationList(const Kernel::IKernelContext& ctx);
}  // namespace Toolkit
}  // namespace OpenViBE
