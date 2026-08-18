///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixClassifierFgMDMRT.cpp
/// \brief Class implementation of Minimum Distance to Mean with geodesic filtering (FgMDM) Classifier RT (adaptation is Real Time Assumed)
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

#include "geometry/classifier/CMatrixClassifierFgMDMRT.hpp"
#include "geometry/Mean.hpp"
#include "geometry/Basics.hpp"
#include "geometry/Featurization.hpp"
#include "geometry/Classification.hpp"
#include <iostream>

namespace Geometry {

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDMRT::Train(const std::vector<std::vector<Eigen::MatrixXd>>& dataset)
{
    if (dataset.empty()) { return false; }
    if (!Mean(Vector2DTo1D(dataset), m_ref, EMetric::Riemann)) { return false; }    // Compute Reference matrix

    // Transform to the Tangent Space
    const size_t nbClass = dataset.size();
    std::vector<std::vector<Eigen::RowVectorXd>> tsSample(nbClass);
    for (size_t k = 0; k < nbClass; ++k) {
        const size_t nbTrials = dataset[k].size();
        tsSample[k].resize(nbTrials);
        for (size_t i = 0; i < nbTrials; ++i) { if (!TangentSpace(dataset[k][i], tsSample[k][i], m_ref)) { return false; } }
    }

    // Compute FgDA Weight
    if (!FgDACompute(tsSample, m_weight)) { return false; }

    // Convert dataset
    std::vector<std::vector<Eigen::MatrixXd>> newDataset(nbClass);
    std::vector<std::vector<Eigen::RowVectorXd>> filtered(nbClass);
    for (size_t k = 0; k < nbClass; ++k) {
        const size_t nbTrials = dataset[k].size();
        newDataset[k].resize(nbTrials);
        filtered[k].resize(nbTrials);
        for (size_t i = 0; i < nbTrials; ++i) {
            if (!FgDAApply(tsSample[k][i], filtered[k][i], m_weight)) { return false; }     // Apply Filter
            if (!UnTangentSpace(filtered[k][i], newDataset[k][i], m_ref)) { return false; } // Return to Matrix Space
        }
    }

    return CMatrixClassifierMDM::Train(newDataset);                                         // Train MDM
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDMRT::Classify(const Eigen::MatrixXd& sample, size_t& classId, std::vector<double>& distance,
                                        std::vector<double>& probability, const EAdaptations adaptation, const size_t& realClassId)
{
    Eigen::RowVectorXd tsSample, filtered;
    Eigen::MatrixXd newSample;

    if (!TangentSpace(sample, tsSample, m_ref)) { return false; }       // Transform to the Tangent Space
    if (!FgDAApply(tsSample, filtered, m_weight)) { return false; }     // Apply Filter
    if (!UnTangentSpace(filtered, newSample, m_ref)) { return false; }  // Return to Matrix Space
    return CMatrixClassifierMDM::Classify(newSample, classId, distance, probability, adaptation, realClassId);
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDMRT::IsEqual(const CMatrixClassifierFgMDMRT& obj, const double precision) const
{
    if (!CMatrixClassifierMDM::IsEqual(obj, precision)) { return false; }   // Compare base members
    if (!AreEquals(m_ref, obj.m_ref, precision)) { return false; }          // Compare Reference
    if (!AreEquals(m_weight, obj.m_weight, precision)) { return false; }    // Compare Weight
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CMatrixClassifierFgMDMRT::Copy(const CMatrixClassifierFgMDMRT& obj)
{
    CMatrixClassifierMDM::Copy(obj);
    m_ref    = obj.m_ref;
    m_weight = obj.m_weight;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDMRT::saveAdditional(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* data) const
{
    // Save Reference
    tinyxml2::XMLElement* reference = doc.NewElement("Reference");  // Create Reference node
    if (!SaveMatrix(reference, m_ref)) { return false; }            // Save class
    data->InsertEndChild(reference);                                // Add class node to data node

    // Save Weight
    tinyxml2::XMLElement* weight = doc.NewElement("Weight");        // Create LDA Weight node
    if (!SaveMatrix(weight, m_weight)) { return false; }            // Save class
    data->InsertEndChild(weight);                                   // Add class node to data node

    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDMRT::loadAdditional(tinyxml2::XMLElement* data)
{
    // Load Reference
    const tinyxml2::XMLElement* ref = data->FirstChildElement("Reference"); // Get Reference Node
    if (!LoadMatrix(ref, m_ref)) { return false; }                          // Load Reference Matrix

    // Load Weight
    const tinyxml2::XMLElement* weight = data->FirstChildElement("Weight"); // Get LDA Weight Node
    return LoadMatrix(weight, m_weight);                                    // Load LDA Weight Matrix
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::stringstream CMatrixClassifierFgMDMRT::printAdditional() const
{
    std::stringstream ss;
    ss << "Reference matrix : " << std::endl << m_ref.format(MATRIX_FORMAT) << std::endl;   // Reference 
    ss << "Weight matrix : " << std::endl << m_weight.format(MATRIX_FORMAT) << std::endl;   // Print Weight
    return ss;
}
///-------------------------------------------------------------------------------------------------

}  // namespace Geometry
