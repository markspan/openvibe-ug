///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmPairwiseDecision.cpp
/// \brief Classes implementation for the Algorithm Pairwise Decision.
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

#include "CAlgorithmPairwiseDecision.hpp"

#include <iostream>
namespace OpenViBE {
namespace Plugins {
namespace Classification {


bool CAlgorithmPairwiseDecision::process()
{
    // @note there is essentially no test that these are called in correct order. Caller be careful!
    if (this->isInputTriggerActive(Classifier_Pairwise_InputTriggerId_Compute)) {
        Kernel::TParameterHandler<std::vector<classification_info_t>*> ip_classifications = this->getInputParameter(
            Classifier_Pairwise_InputParameter_ClassificationOutputs);
        Kernel::TParameterHandler<CMatrix*> op_probability = this->getOutputParameter(Classifier_OutputParameter_ProbabilityVector);
        return this->Compute(*static_cast<std::vector<classification_info_t>*>(ip_classifications), static_cast<CMatrix*>(op_probability));
    }
    if (this->isInputTriggerActive(Classifier_Pairwise_InputTriggerId_SaveConfig)) {
        Kernel::TParameterHandler<XML::IXMLNode*> op_configuration(this->getOutputParameter(Classifier_Pairwise_OutputParameterId_Config));
        XML::IXMLNode* tmp = this->SaveConfig();

        OV_ERROR_UNLESS_KRF(tmp != nullptr, "Invalid NULL xml node returned while saving configuration", Kernel::ErrorType::Internal);

        op_configuration = tmp;
        return true;
    }
    if (this->isInputTriggerActive(Classifier_Pairwise_InputTriggerId_LoadConfig)) {
        Kernel::TParameterHandler<XML::IXMLNode*> op_config(this->getInputParameter(Classifier_Pairwise_InputParameterId_Config));
        XML::IXMLNode* tmp = static_cast<XML::IXMLNode*>(op_config);

        OV_ERROR_UNLESS_KRF(tmp != nullptr, "Invalid NULL xml node to load configuration in", Kernel::ErrorType::BadInput);

        return this->LoadConfig(*tmp);
    }
    if (this->isInputTriggerActive(Classifier_Pairwise_InputTriggerId_Parameterize)) { return this->Parameterize(); }

    return true;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
