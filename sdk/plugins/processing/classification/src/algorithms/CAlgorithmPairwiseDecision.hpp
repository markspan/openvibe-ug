///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmPairwiseDecision.hpp
/// \brief Classes for the Algorithm Pairwise Decision.
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

#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>
#include "CAlgorithmClassifierOneVsOne.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Classification {
/**
 * @brief The CAlgorithmPairwiseDecision class
 * This is the default class for every decision usable with the One Vs One pairwise strategy.
 */
class CAlgorithmPairwiseDecision : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override = 0;
    bool uninitialize() override = 0;

    virtual bool Parameterize() = 0;

    virtual bool Compute(std::vector<classification_info_t>& classifications, CMatrix* probabilities) = 0;
    virtual XML::IXMLNode* SaveConfig() = 0;
    virtual bool LoadConfig(XML::IXMLNode& node) = 0;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Algorithm_PairwiseDecision)
};

class CAlgorithmPairwiseDecisionDesc : virtual public IAlgorithmDesc
{
public:
    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(Classifier_InputParameter_ProbabilityMatrix, "Probability Matrix", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(Classifier_Pairwise_InputParameterId_Config, "Configuration node", Kernel::ParameterType_Pointer);
        prototype.addInputParameter(Classifier_Pairwise_InputParameterId_SetRepartition, "Set repartition", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(Classifier_Pairwise_InputParameterId_AlgorithmIdentifier, "Classification Algorithm", Kernel::ParameterType_Identifier);
        prototype.addInputParameter(Classifier_Pairwise_InputParameter_ClassificationOutputs, "Classification Outputs", Kernel::ParameterType_Pointer);
        prototype.addInputParameter(Classifier_Pairwise_InputParameter_ClassCount, "Class Count", Kernel::ParameterType_UInteger);

        prototype.addOutputParameter(Classifier_OutputParameter_ProbabilityVector, "Probability Vector", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(Classifier_Pairwise_OutputParameterId_Config, "Configuration node", Kernel::ParameterType_Pointer);

        prototype.addInputTrigger(Classifier_Pairwise_InputTriggerId_Compute, "Compute");
        prototype.addInputTrigger(Classifier_Pairwise_InputTriggerId_Parameterize, "Parametrize");
        prototype.addInputTrigger(Classifier_Pairwise_InputTriggerId_SaveConfig, "Save configuration");
        prototype.addInputTrigger(Classifier_Pairwise_InputTriggerId_LoadConfig, "Load configuration");
        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Algorithm_PairwiseDecisionDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
