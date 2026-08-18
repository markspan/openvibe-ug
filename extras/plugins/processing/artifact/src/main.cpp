///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for artifact plugin, registering the boxes to OpenViBE
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/12/2020.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include <openvibe/ov_all.h>
#include "defines.hpp"

// Boxes Includes
#include "boxes/CBoxAlgorithmAmplitudeArtifactDetector.hpp"
#include "boxes/CBoxAlgorithmASRTrainer.hpp"
#include "boxes/CBoxAlgorithmASRProcessor.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Artifact {

template <typename T>
static void setEnumeration(const Kernel::IPluginModuleContext& context, const CIdentifier& typeID, const std::string& name, const std::vector<T>& enumeration)
{
    context.getTypeManager().registerEnumerationType(typeID, name.c_str());
    for (const auto& e : enumeration) { context.getTypeManager().registerEnumerationEntry(typeID, toString(e).c_str(), size_t(e)); }
}

OVP_Declare_Begin()
    // Register boxes
    OVP_Declare_New(CBoxAlgorithmAmplitudeArtifactDetectorDesc)
    OVP_Declare_New(CBoxAlgorithmASRTrainerDesc)
    OVP_Declare_New(CBoxAlgorithmASRProcessorDesc)

    // Enumeration Metric
    const std::vector<Geometry::EMetric> metrics = {
        Geometry::EMetric::Riemann, Geometry::EMetric::Euclidian, Geometry::EMetric::LogEuclidian, Geometry::EMetric::LogDet,
        Geometry::EMetric::Kullback, Geometry::EMetric::Harmonic, Geometry::EMetric::Identity
    };
    setEnumeration(context, TypeId_Metric, "Metric", metrics);

    // Artifact action enumeration
    const std::vector<EArtifactAction> artifactActions = {
        EArtifactAction::Stop, EArtifactAction::StimulationsOnly, EArtifactAction::Cutoff, EArtifactAction::Zero
    };
    setEnumeration(context, TypeId_ArtifactAction, "Action", artifactActions);


OVP_Declare_End()

}  // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
