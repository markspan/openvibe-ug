///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixClassifierMDMRebias.cpp
/// \brief Class implementation of Minimum Distance to Mean (MDM) Classifier with Rebias.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 10/12/2018.
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

#include "geometry/classifier/CMatrixClassifierMDMRebias.hpp"
#include "geometry/Mean.hpp"
#include "geometry/Basics.hpp"
#include <unsupported/Eigen/MatrixFunctions> // SQRT of Matrix

namespace Geometry {

//**********************
//***** Classifier *****
//**********************
///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierMDMRebias::Train(const std::vector<std::vector<Eigen::MatrixXd>>& dataset)
{
    if (!m_bias.ComputeBias(dataset, m_metric)) { return false; }
    std::vector<std::vector<Eigen::MatrixXd>> newDataset;
    m_bias.ApplyBias(dataset, newDataset);
    return CMatrixClassifierMDM::Train(newDataset);         // Train MDM
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierMDMRebias::Classify(const Eigen::MatrixXd& sample, size_t& classId, std::vector<double>& distance,
                                          std::vector<double>& probability, const EAdaptations adaptation, const size_t& realClassId)
{
    if (!IsSquare(sample)) { return false; }                    // Verification if it's a square matrix 
    Eigen::MatrixXd newSample;
    m_bias.ApplyBias(sample, newSample);
    m_bias.UpdateBias(sample, m_metric);
    return CMatrixClassifierMDM::Classify(newSample, classId, distance, probability, adaptation, realClassId);
}
///-------------------------------------------------------------------------------------------------

//***********************
//***** XML Manager *****
//***********************
///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierMDMRebias::saveAdditional(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* data) const
{
    if (!CMatrixClassifierMDM::saveAdditional(doc, data)) { return false; }
    if (!m_bias.SaveAdditional(doc, data)) { return false; }
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierMDMRebias::loadAdditional(tinyxml2::XMLElement* data)
{
    if (!CMatrixClassifierMDM::loadAdditional(data)) { return false; }
    if (!m_bias.LoadAdditional(data)) { return false; }
    return true;
}
///-------------------------------------------------------------------------------------------------

//*****************************
//***** Override Operator *****
//*****************************
///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierMDMRebias::IsEqual(const CMatrixClassifierMDMRebias& obj, const double precision) const
{
    return CMatrixClassifierMDM::IsEqual(obj, precision) && m_bias == obj.m_bias;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CMatrixClassifierMDMRebias::Copy(const CMatrixClassifierMDMRebias& obj)
{
    CMatrixClassifierMDM::Copy(obj);
    m_bias = obj.m_bias;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::stringstream CMatrixClassifierMDMRebias::printAdditional() const
{
    std::stringstream ss = CMatrixClassifierMDM::printAdditional();
    ss << m_bias;
    return ss;
}
///-------------------------------------------------------------------------------------------------

}  // namespace Geometry
