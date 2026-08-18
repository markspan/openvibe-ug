#include <openvibe/ov_all.h>
#include "ovp_defines.h"

// Boxes Includes
#include "boxes/CBoxAlgorithmFeaturesSelection.hpp"
#include "boxes/CBoxAlgorithmFeaturesSelector.hpp"

namespace OpenViBE {
namespace Plugins {
namespace FeaturesSelection {

OVP_Declare_Begin()
    // Register boxes
    OVP_Declare_New(CBoxAlgorithmFeaturesSelectionDesc)
    OVP_Declare_New(CBoxAlgorithmFeaturesSelectorDesc)

    // Enumeration Feature Selection Method
    context.getTypeManager().registerEnumerationType(OVP_TypeId_Features_Selection_Method, "Features Selection Method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_Features_Selection_Method, toString(EFeatureSelection::MRMR).c_str(),
                                                      size_t(EFeatureSelection::MRMR));

    // Enumeration mRMR Method
    context.getTypeManager().registerEnumerationType(OVP_TypeId_mRMR_Method, "mRMR Method");
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_mRMR_Method, "MID", size_t(EMRMRMethod::MID));
    context.getTypeManager().registerEnumerationEntry(OVP_TypeId_mRMR_Method, "MIQ", size_t(EMRMRMethod::MIQ));

OVP_Declare_End()
}  // namespace FeaturesSelection
}  // namespace Plugins
}  // namespace OpenViBE
