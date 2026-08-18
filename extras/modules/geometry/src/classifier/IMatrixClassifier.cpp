///-------------------------------------------------------------------------------------------------
/// 
/// \file IMatrixClassifier.cpp
/// \brief Abstract class implementation of Matrix Classifier
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

#include "geometry/classifier/IMatrixClassifier.hpp"
#include <iostream>

namespace Geometry {

//***********************   
//***** Constructor *****   
//***********************
///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::Classify(const Eigen::MatrixXd& sample, size_t& classId, const EAdaptations adaptation, const size_t& realClassId)
{
    std::vector<double> distance, probability;
    return Classify(sample, classId, distance, probability, adaptation, realClassId);
}
///-------------------------------------------------------------------------------------------------

//***********************
//***** XML Manager *****
//***********************
///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::SaveXML(const std::string& filename) const
{
    tinyxml2::XMLDocument xmlDoc;
    // Create Root
    tinyxml2::XMLNode* root = xmlDoc.NewElement("Classifier");      // Create root node
    xmlDoc.InsertFirstChild(root);                                  // Add root to XML

    tinyxml2::XMLElement* data = xmlDoc.NewElement("Classifier-data");  // Create data node
    if (!saveHeader(data)) { return false; }                        // Save Header attribute
    if (!saveAdditional(xmlDoc, data)) { return false; }            // Save Optionnal Informations
    if (!saveClasses(xmlDoc, data)) { return false; }               // Save Classes

    root->InsertEndChild(data);                                     // Add data to root
    return xmlDoc.SaveFile(filename.c_str()) == 0;                  // save XML (if != 0 it means error)
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::LoadXML(const std::string& filename)
{
    // Load File
    tinyxml2::XMLDocument xmlDoc;
    if (xmlDoc.LoadFile(filename.c_str()) != 0) { return false; }   // Check File Exist and Loading

    // Load Root
    tinyxml2::XMLNode* root = xmlDoc.FirstChild();                  // Get Root Node
    if (root == nullptr) { return false; }                          // Check Root Node Exist

    // Load Data
    tinyxml2::XMLElement* data = root->FirstChildElement("Classifier-data");    // Get Data Node
    if (data == nullptr) { return false; }                          // Check Root Node Exist
    if (!loadHeader(data)) { return false; }                        // Load Header attribute
    if (!loadAdditional(data)) { return false; }                    // Load Optionnal Informations
    if (!loadClasses(data)) { return false; }                       // Load Classes

    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::ConvertMatrixToXMLFormat(const Eigen::MatrixXd& in, std::stringstream& out)
{
    out << in.format(MATRIX_FORMAT);
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::ConvertXMLFormatToMatrix(std::stringstream& in, Eigen::MatrixXd& out, const Eigen::Index rows, const Eigen::Index cols)
{
    out = Eigen::MatrixXd::Identity(rows, cols);        // Init With Identity Matrix (in case of)
    for (Eigen::Index i = 0; i < rows; ++i) {           // Fill Matrix
        for (Eigen::Index j = 0; j < cols; ++j) { in >> out(i, j); }
    }
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::SaveMatrix(tinyxml2::XMLElement* element, const Eigen::MatrixXd& matrix)
{
    element->SetAttribute("size", int(matrix.rows()));  // Set Matrix size NxN
    std::stringstream ss;
    ConvertMatrixToXMLFormat(matrix, ss);
    element->SetText(ss.str().c_str());                 // Write Means Value
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::LoadMatrix(const tinyxml2::XMLElement* element, Eigen::MatrixXd& matrix)
{
    const Eigen::Index size = element->IntAttribute("size");    // Get number of row/col
    if (size == 0) { return true; }
    std::stringstream ss(element->GetText());           // String stream to parse Matrix value
    ConvertXMLFormatToMatrix(ss, matrix, size, size);
    return true;
}
///-------------------------------------------------------------------------------------------------

//*****************************
//***** Override Operator *****
//*****************************
///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::IsEqual(const IMatrixClassifier& obj, const double /*precision*/) const
{
    return m_metric == obj.m_metric && m_nbClass == obj.GetClassCount();
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
void IMatrixClassifier::Copy(const IMatrixClassifier& obj)
{
    m_metric = obj.m_metric;
    SetClassCount(obj.GetClassCount());
}
/// -------------------------------------------------------------------------------------------------

/// -------------------------------------------------------------------------------------------------
std::stringstream IMatrixClassifier::Print() const { return std::stringstream(printHeader().str() + printAdditional().str() + printClasses().str()); }
/// -------------------------------------------------------------------------------------------------

/// -------------------------------------------------------------------------------------------------
std::stringstream IMatrixClassifier::printHeader() const
{
    std::stringstream ss;
    ss << GetType() << " Classifier" << std::endl;
    ss << "Metric : " << toString(m_metric) << std::endl;
    ss << "Number of Classes : " << m_nbClass << std::endl;
    return ss;
}
///-------------------------------------------------------------------------------------------------

//*******************************************
//***** XML Manager (Private Functions) *****
//*******************************************
///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::saveHeader(tinyxml2::XMLElement* data) const
{
    data->SetAttribute("type", GetType().c_str());              // Set attribute classifier type
    data->SetAttribute("class-count", int(m_nbClass));          // Set attribute class count
    data->SetAttribute("metric", toString(m_metric).c_str());   // Set attribute metric
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool IMatrixClassifier::loadHeader(tinyxml2::XMLElement* data)
{
    if (data == nullptr) { return false; }                      // Check if Node Exist
    const std::string classifierType = data->Attribute("type"); // Get type
    if (classifierType != GetType()) { return false; }          // Check Type
    SetClassCount(data->IntAttribute("class-count"));           // Update Number of classes
    m_metric = StringToMetric(data->Attribute("metric"));       // Update Metric
    return true;
}
///-------------------------------------------------------------------------------------------------

}  // namespace Geometry
