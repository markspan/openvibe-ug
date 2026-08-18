///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmPairwiseStrategyPKPD.cpp
/// \brief Classes implementation for the Algorithm Pairwise Strategy PKPD.
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

#define PKPD_DEBUG 0
#include "CAlgorithmPairwiseStrategyPKPD.hpp"
#include <xml/IXMLNode.h>

#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const TYPE_NODE_NAME = "PairwiseDecision_PKDP";

bool CAlgorithmPairwiseStrategyPKPD::Parameterize()
{
    Kernel::TParameterHandler<uint64_t> ip_nClass(this->getInputParameter(Classifier_Pairwise_InputParameter_ClassCount));
    m_nClass = size_t(ip_nClass);

    OV_ERROR_UNLESS_KRF(m_nClass >= 2, "Pairwise decision PKPD algorithm needs at least 2 classes [" << m_nClass << "] found", Kernel::ErrorType::BadInput);

    return true;
}

bool CAlgorithmPairwiseStrategyPKPD::Compute(std::vector<classification_info_t>& classifications, CMatrix* probabilities)
{
    OV_ERROR_UNLESS_KRF(m_nClass >= 2, "Pairwise decision PKPD algorithm needs at least 2 classes [" << m_nClass << "] found", Kernel::ErrorType::BadInput);

    std::vector<double> matrix(m_nClass * m_nClass);

    //First we set the diagonal to 0
    for (size_t i = 0; i < m_nClass; ++i) { matrix[i * m_nClass + i] = 0.; }

    for (size_t i = 0; i < classifications.size(); ++i) {
        const classification_info_t& temp       = classifications[i];
        const size_t firstIdx                   = size_t(temp.firstClass);
        const size_t secondIdx                  = size_t(temp.secondClass);
        const double* values                    = temp.classificationValue->getBuffer();
        matrix[firstIdx * m_nClass + secondIdx] = values[0];
        matrix[secondIdx * m_nClass + firstIdx] = 1 - values[0];
    }

#if PKPD_DEBUG
    for (size_t i = 0 ; i < m_nClass ; ++i)
    {
        for (size_t j = 0 ; j < m_nClass ; ++j) { std::cout << matrix[i * m_nClass + j] << " "; }
        std::cout << std::endl;
    }
#endif

    std::vector<double> probVector(m_nClass);
    double sum = 0;
    for (size_t classIdx = 0; classIdx < m_nClass; ++classIdx) {
        double tmpSum = 0;
        for (size_t secondClass = 0; secondClass < m_nClass; ++secondClass) {
            if (secondClass != classIdx) { tmpSum += 1 / matrix[m_nClass * classIdx + secondClass]; }
        }
        probVector[classIdx] = 1 / (tmpSum - (m_nClass - 2));
        sum += probVector[classIdx];
    }

    for (size_t i = 0; i < m_nClass; ++i) { probVector[i] /= sum; }

#if PKPD_DEBUG
    for (size_t i = 0; i < m_nClass ; ++i) { std::cout << probVector[i] << " "; }
    std::cout << std::endl;
#endif

    probabilities->resize(m_nClass);

    for (size_t i = 0; i < m_nClass; ++i) { probabilities->getBuffer()[i] = probVector[i]; }

    return true;
}

XML::IXMLNode* CAlgorithmPairwiseStrategyPKPD::SaveConfig()
{
    XML::IXMLNode* node = XML::createNode(TYPE_NODE_NAME);
    return node;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
