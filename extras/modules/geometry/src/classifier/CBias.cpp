///-------------------------------------------------------------------------------------------------
/// 
/// \file CBias.cpp
/// \brief Class implementation used to add Rebias to Other Classifier.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 27/08/2019.
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

#include "geometry/classifier/CBias.hpp"
#include "geometry/classifier/IMatrixClassifier.hpp"
#include "geometry/Mean.hpp"
#include "geometry/Basics.hpp"
#include "geometry/Geodesic.hpp"
#include <unsupported/Eigen/MatrixFunctions> // SQRT of Matrix
#include <iostream>

namespace Geometry {

///-------------------------------------------------------------------------------------------------
bool CBias::ComputeBias(const std::vector<std::vector<Eigen::MatrixXd>>& dataset, const EMetric metric) { return ComputeBias(Vector2DTo1D(dataset), metric); }
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBias::ComputeBias(const std::vector<Eigen::MatrixXd>& dataset, const EMetric metric)
{
    if (!Mean(dataset, m_bias, metric)) { return false; }   // Compute Bias reference
    m_biasIS = m_bias.sqrt().inverse();                     // Inverse Square root of Bias matrix => isR
    m_n      = 0;
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CBias::ApplyBias(const std::vector<std::vector<Eigen::MatrixXd>>& in, std::vector<std::vector<Eigen::MatrixXd>>& out)
{
    const size_t n = in.size();
    out.resize(n);
    for (size_t i = 0; i < n; ++i) { ApplyBias(in[i], out[i]); }
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CBias::ApplyBias(const std::vector<Eigen::MatrixXd>& in, std::vector<Eigen::MatrixXd>& out)
{
    const size_t n = in.size();
    out.resize(n);
    for (size_t i = 0; i < n; ++i) { ApplyBias(in[i], out[i]); }
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CBias::ApplyBias(const Eigen::MatrixXd& in, Eigen::MatrixXd& out) { out = m_biasIS * in * m_biasIS.transpose(); }
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CBias::UpdateBias(const Eigen::MatrixXd& sample, const EMetric metric)
{
    m_n++;                                                  // Update number of classify
    if (m_n == 1) { m_bias = sample; }                      // At the first pass we reinitialize the Bias
    else { Geodesic(m_bias, sample, m_bias, metric, 1.0 / double(m_n)); }
    m_biasIS = m_bias.sqrt().inverse();                     // Inverse Square root of Bias matrix => isR
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CBias::SetBias(const Eigen::MatrixXd& bias)
{
    m_bias   = bias;
    m_biasIS = m_bias.sqrt().inverse();
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBias::SaveXML(const std::string& filename) const
{
    tinyxml2::XMLDocument xmlDoc;
    // Create Root
    tinyxml2::XMLNode* root = xmlDoc.NewElement("Bias");    // Create root node
    xmlDoc.InsertFirstChild(root);                          // Add root to XML

    tinyxml2::XMLElement* data = xmlDoc.NewElement("Bias-data");    // Create data node
    if (!SaveAdditional(xmlDoc, data)) { return false; }    // Save Optionnal Informations

    root->InsertEndChild(data);                             // Add data to root
    return xmlDoc.SaveFile(filename.c_str()) == 0;          // save XML (if != 0 it means error)
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBias::LoadXML(const std::string& filename)
{
    // Load File
    tinyxml2::XMLDocument xmlDoc;
    if (xmlDoc.LoadFile(filename.c_str()) != 0) { return false; }   // Check File Exist and Loading

    // Load Root
    tinyxml2::XMLNode* root = xmlDoc.FirstChild();          // Get Root Node
    if (root == nullptr) { return false; }                  // Check Root Node Exist

    // Load Data
    tinyxml2::XMLElement* data = root->FirstChildElement("Bias-data");  // Get Data Node
    if (!LoadAdditional(data)) { return false; }            // Load Optionnal Informations
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBias::SaveAdditional(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* data) const
{
    tinyxml2::XMLElement* bias = doc.NewElement("Bias");    // Create Bias node
    bias->SetAttribute("n", int(m_n));                      // Set attribute class number of trials
    if (!IMatrixClassifier::SaveMatrix(bias, m_bias)) { return false; } // Save class
    data->InsertEndChild(bias);                             // Add class node to data node
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBias::LoadAdditional(tinyxml2::XMLElement* data)
{
    const tinyxml2::XMLElement* bias = data->FirstChildElement("Bias"); // Get LDA Weight Node
    m_n                              = bias->IntAttribute("n");         // Get the number of Trials for this class
    if (!IMatrixClassifier::LoadMatrix(bias, m_bias)) { return false; } // Load Reference Matrix
    m_biasIS = m_bias.sqrt().inverse();
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBias::IsEqual(const CBias& obj, const double precision) const { return AreEquals(m_bias, obj.m_bias, precision) && m_n == obj.m_n; }
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void CBias::Copy(const CBias& obj)
{
    m_bias   = obj.m_bias;
    m_biasIS = obj.m_biasIS;
    m_n      = obj.m_n;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
std::stringstream CBias::Print() const
{
    std::stringstream ss;
    ss << "Number of Classification : " << m_n << std::endl;
    ss << "Bias Matrix : ";
    if (m_bias.size() != 0) { ss << std::endl << m_bias.format(MATRIX_FORMAT) << std::endl; }
    else { ss << "Not Computed" << std::endl; }
    return ss;
}
///-------------------------------------------------------------------------------------------------

}  // namespace Geometry
