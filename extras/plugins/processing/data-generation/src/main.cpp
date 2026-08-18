#include "defines.hpp"

#include "box-algorithms/CBoxAlgorithmNoiseGenerator.hpp"
#include "box-algorithms/CSinusSignalGenerator.hpp"
#include "box-algorithms/CBoxAlgorithmChannelUnitsGenerator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {

OVP_Declare_Begin()
    OVP_Declare_New(CNoiseGeneratorDesc)
    context.getTypeManager().registerEnumerationType(TypeId_NoiseType, "Noise type");
    context.getTypeManager().registerEnumerationEntry(TypeId_NoiseType, "Uniform", TypeId_NoiseType_Uniform);
    context.getTypeManager().registerEnumerationEntry(TypeId_NoiseType, "Gaussian", TypeId_NoiseType_Gaussian);

    OVP_Declare_New(CSinusSignalGeneratorDesc)
    OVP_Declare_New(CChannelUnitsGeneratorDesc)


OVP_Declare_End()

}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
