///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmLDADiscriminantFunction.cpp
/// \brief Classes implementation for the Algorithm LDA Discriminant Function.
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

#include "CAlgorithmLDADiscriminantFunction.hpp"
#include <Eigen/Eigenvalues>

#include <sstream>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const BASE_NODE_NAME   = "Class-config";
static const char* const WEIGHT_NODE_NAME = "Weights";
static const char* const BIAS_NODE_NAME   = "Bias";

bool CAlgorithmLDADiscriminantFunction::LoadConfig(const XML::IXMLNode* configuration)
{
    std::stringstream bias(configuration->getChildByName(BIAS_NODE_NAME)->getPCData());
    bias >> m_bias;

    std::stringstream data(configuration->getChildByName(WEIGHT_NODE_NAME)->getPCData());
    std::vector<double> coefficients;
    while (!data.eof()) {
        double value;
        data >> value;
        coefficients.push_back(value);
    }

    m_weight.resize(coefficients.size());
    for (size_t i = 0; i < coefficients.size(); ++i) { m_weight(i, 0) = coefficients[i]; }
    return true;
}

XML::IXMLNode* CAlgorithmLDADiscriminantFunction::GetConfiguration()
{
    XML::IXMLNode* rootNode = XML::createNode(BASE_NODE_NAME);

    std::stringstream weigths, bias;

    weigths << std::scientific;
    for (int i = 0; i < m_weight.size(); ++i) { weigths << " " << m_weight(i, 0); }

    bias << m_bias;

    XML::IXMLNode* tempNode = XML::createNode(WEIGHT_NODE_NAME);
    tempNode->setPCData(weigths.str().c_str());
    rootNode->addChild(tempNode);

    tempNode = XML::createNode(BIAS_NODE_NAME);
    tempNode->setPCData(bias.str().c_str());
    rootNode->addChild(tempNode);

    return rootNode;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
