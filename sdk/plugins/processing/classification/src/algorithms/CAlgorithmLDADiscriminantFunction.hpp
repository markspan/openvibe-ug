///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmLDADiscriminantFunction.hpp
/// \brief Classes for the Algorithm LDA Discriminant Function.
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

#include "CAlgorithmClassifierLDA.hpp"
#include <xml/IXMLNode.h>
#include <Eigen/Eigenvalues>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
//The purpose of this class is to compute the "membership" of a vector
class CAlgorithmLDADiscriminantFunction
{
public:
    CAlgorithmLDADiscriminantFunction() {}

    void SetWeight(const Eigen::VectorXd& weigth) { m_weight = weigth; }
    void SetBias(const double bias) { m_bias = bias; }

    //Return the class membership of the feature vector
    double GetValue(const Eigen::VectorXd& featureVector) { return (m_weight.transpose() * featureVector)(0) + m_bias; }
    size_t GetNWeight() const { return m_weight.size(); }


    bool LoadConfig(const XML::IXMLNode* configuration);
    XML::IXMLNode* GetConfiguration();

    const Eigen::VectorXd& GetWeight() const { return m_weight; }
    double GetBias() const { return m_bias; }

private:
    double m_bias = 0;
    Eigen::VectorXd m_weight;
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
