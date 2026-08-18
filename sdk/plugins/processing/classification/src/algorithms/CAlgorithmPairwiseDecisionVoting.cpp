///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmPairwiseDecisionVoting.cpp
/// \brief Classes implementation for the Algorithm Pairwise Decision Voting.
/// \author Guillaume Serrière (Inria).
/// \version 0.1.
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

#define VOTING_DEBUG 0
#include "CAlgorithmPairwiseDecisionVoting.hpp"

#include <iostream>

#include <xml/IXMLNode.h>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const TYPE_NODE_NAME = "PairwiseDecision_Voting";

bool CAlgorithmPairwiseDecisionVoting::Parameterize()
{
    Kernel::TParameterHandler<uint64_t> ip_nClass(this->getInputParameter(Classifier_Pairwise_InputParameter_ClassCount));
    m_nClass = size_t(ip_nClass);

    OV_ERROR_UNLESS_KRF(m_nClass >= 2, "Pairwise decision Voting algorithm needs at least 2 classes [" << m_nClass << "] found", Kernel::ErrorType::BadInput);

    return true;
}

bool CAlgorithmPairwiseDecisionVoting::Compute(std::vector<classification_info_t>& classifications, CMatrix* probabilities)
{
    OV_ERROR_UNLESS_KRF(m_nClass >= 2, "Pairwise decision Voting algorithm needs at least 2 classes [" << m_nClass << "] found", Kernel::ErrorType::BadInput);

#if VOTING_DEBUG
    std::cout << classifications.size() << std::endl;

    for (size_t i = 0 ; i < classifications.size() ; ++i)
    {
        std::cout << classifications[i].firstClass << " " << classifications[i].secondClass << std::endl;
        std::cout << classifications[i].classLabel << std::endl;
    }
#endif

    std::vector<size_t> win(m_nClass);
    for (size_t i = 0; i < m_nClass; ++i) { win[i] = 0; }

    for (size_t i = 0; i < classifications.size(); ++i) {
        const classification_info_t& temp = classifications[i];
        if (temp.classLabel == 0) { ++(win[size_t(temp.firstClass)]); }
        else { ++(win[size_t(temp.secondClass)]); }
    }

#if VOTING_DEBUG
    for (size_t i = 0; i < m_nClass ;  ++i) { std::cout << (double(win[i])/ classifications.size() <<  " "; }
    std::cout << std::endl;
#endif

    probabilities->resize(m_nClass);

    for (size_t i = 0; i < m_nClass; ++i) { probabilities->getBuffer()[i] = double(win[i]) / classifications.size(); }

    return true;
}

XML::IXMLNode* CAlgorithmPairwiseDecisionVoting::SaveConfig()
{
    XML::IXMLNode* node = XML::createNode(TYPE_NODE_NAME);
    return node;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
