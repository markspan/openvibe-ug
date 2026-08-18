///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for riemannian plugin, registering the boxes to OpenViBE
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
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
#include "boxes/CBoxAlgorithmCovarianceMatrixCalculator.hpp"
#include "boxes/CBoxAlgorithmCovarianceMatrixToFeatureVector.hpp"
#include "boxes/CBoxAlgorithmFeatureVectorToCovarianceMatrix.hpp"
#include "boxes/CBoxAlgorithmCovarianceMeanCalculator.hpp"
#include "boxes/CBoxAlgorithmMatrixClassifierTrainer.hpp"
#include "boxes/CBoxAlgorithmMatrixClassifierProcessor.hpp"
#include "boxes/CBoxAlgorithmMatrixAffineTransformation.hpp"
#include "boxes/CBoxAlgorithmRiemannianPotatoField.hpp"
#include "boxes/CBoxAlgorithmRiemannianPotatoFieldTrainer.hpp"

namespace OpenViBE {
namespace Plugins {

template <typename T>
static void setEnumeration(const Kernel::IPluginModuleContext& context, const CIdentifier& typeID, const std::string& name, const std::vector<T>& enumeration)
{
    context.getTypeManager().registerEnumerationType(typeID, name.c_str());
    for (const auto& e : enumeration) { context.getTypeManager().registerEnumerationEntry(typeID, toString(e).c_str(), size_t(e)); }
}


OVP_Declare_Begin()
    // Register boxes
    OVP_Declare_New(Riemannian::CBoxAlgorithmCovarianceMatrixCalculatorDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmCovarianceMatrixToFeatureVectorDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmFeatureVectorToCovarianceMatrixDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmCovarianceMeanCalculatorDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmMatrixClassifierTrainerDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmMatrixClassifierProcessorDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmMatrixAffineTransformationDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmRiemannianPotatoFieldDesc)
    OVP_Declare_New(Riemannian::CBoxAlgorithmRiemannianPotatoFieldTrainerDesc)

    // Enumeration Estimator
    const std::vector<Geometry::EEstimator> estimators = {
        Geometry::EEstimator::COV, Geometry::EEstimator::COR, Geometry::EEstimator::LWF,
        Geometry::EEstimator::SCM, Geometry::EEstimator::OAS, Geometry::EEstimator::IDE
    };
    setEnumeration(context, TypeId_Estimator, "Estimator", estimators);

    // Enumeration Metric
    const std::vector<Geometry::EMetric> metrics = {
        Geometry::EMetric::Riemann, Geometry::EMetric::Euclidian, Geometry::EMetric::LogEuclidian, Geometry::EMetric::LogDet,
        Geometry::EMetric::Kullback, Geometry::EMetric::Harmonic, Geometry::EMetric::Identity
    };
    setEnumeration(context, TypeId_Metric, "Metric", metrics);

    // Enumeration Classifier
    const std::vector<Geometry::EMatrixClassifiers> classifiers = {
        Geometry::EMatrixClassifiers::MDM, Geometry::EMatrixClassifiers::MDM_Rebias,
        Geometry::EMatrixClassifiers::FgMDM_RT, Geometry::EMatrixClassifiers::FgMDM_RT_Rebias
    };
    setEnumeration(context, TypeId_Matrix_Classifier, "Matrix Classifier", classifiers);

    // Enumeration Classifier Adaptater
    const std::vector<Geometry::EAdaptations> adaptations = {
        Geometry::EAdaptations::None, Geometry::EAdaptations::Supervised, Geometry::EAdaptations::Unsupervised
    };
    setEnumeration(context, TypeId_Classifier_Adaptation, "Classifier Adaptation", adaptations);

OVP_Declare_End()

}  // namespace Plugins
}  // namespace OpenViBE
