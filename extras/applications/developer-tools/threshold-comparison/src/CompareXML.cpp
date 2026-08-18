///-------------------------------------------------------------------------------------------------
/// 
/// \file CompareXML.cpp
/// \author Thibaut Monseigne / Inria.
/// \version 2.0.
/// \date 19/04/2022.
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

#include <cmath> // std::fabs
#include <iostream>
#include <string>
#include <vector>

#include "CompareXML.hpp"

///-------------------------------------------------------------------------------------------------
bool CompareXML::Test(const std::string& file1, const std::string& file2)
{
    // Load File and Root
    tinyxml2::XMLDocument xmlDoc1, xmlDoc2;
    // Check if File Exist and Loading
    if (xmlDoc1.LoadFile(file1.c_str()) != 0) {
        std::cout << "Error opening file [" << file1 << "] for reading" << std::endl;
        return false;
    }
    if (xmlDoc2.LoadFile(file2.c_str()) != 0) {
        std::cout << "Error opening file [" << file2 << "] for reading" << std::endl;
        return false;
    }

    // Load Root
    tinyxml2::XMLElement* root1 = xmlDoc1.FirstChildElement();          // Get Root Node
    tinyxml2::XMLElement* root2 = xmlDoc2.FirstChildElement();          // Get Root Node

    if (root1 == nullptr) {
        std::cout << "No root node in file [" << file1 << "]" << std::endl;
        return false;
    }
    if (root2 == nullptr) {
        std::cout << "No root node in file [" << file2 << "]" << std::endl;
        return false;
    }
    return compareElement(root1, root2);
}

///-------------------------------------------------------------------------------------------------
bool CompareXML::compareElement(tinyxml2::XMLElement* node1, tinyxml2::XMLElement* node2)
{
    tinyxml2::XMLElement *child1 = node1->FirstChildElement(), *child2 = node2->FirstChildElement();
    // there is no child so we compare values
    if (child1 == nullptr && child2 == nullptr) {
        const char* s1 = node1->GetText();
        const char* s2 = node2->GetText();
        return compareValue(s1 ? s1 : "", s2 ? s2 : "");    // Check s1 and s2 validity as tinyxml2 doesn't return "" if there is no text
    }
    // We have Child, so we check all node recursively
    for (; ; child1 = child1->NextSiblingElement(), child2 = child2->NextSiblingElement()) {
        if (child1 == nullptr && child2 == nullptr) { break; }  // Finish reading children
        if (child1 != nullptr && child2 != nullptr) { if (!compareElement(child1, child2)) { return false; } }
        else {
            std::cout << "Error : XML structur is different." << std::endl;
            return false;
        }
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CompareXML::compareValue(const std::string& value1, const std::string& value2) const
{
    // Replace \r\n, \n and \t by space.
    std::string tmp1 = value1, tmp2 = value2;
    RemoveSpecial(tmp1);
    RemoveSpecial(tmp2);
    if (tmp1.empty() && tmp2.empty()) { return true; }

    // Possible values : int, string, float, matrix, 
    if (IsNumber(tmp1) && IsNumber(tmp2)) {
        //Fill Buffer
        const std::vector<std::string> buffer1 = Split(tmp1, " "), buffer2 = Split(tmp2, " ");
        for (size_t i = 0; i < buffer1.size(); ++i) {
            const bool hasValue1 = !buffer1[i].empty(), hasValue2 = !buffer2[i].empty();
            if (hasValue1 != hasValue2) {
                std::cout << "Error : One Value is " << (hasValue1 ? "not" : "") << " empty in first file"
                        << " and is " << (hasValue2 ? "not" : "") << " empty in second file." << std::endl;
                return false;
            }
            if (hasValue1) {
                const double err = std::fabs(std::stod(buffer1[i]) - std::stod(buffer2[i]));
                if (err > m_threshold) {
                    std::cout << "Difference too big between \"" << buffer1[i] << "\" and \"" << buffer2[i] << "\"" << std::endl;
                    return false;
                }
            }
        }
    }
    else if (tmp1 != tmp2) {
        std::cout << "Difference between \"" << tmp1 << "\" and \"" << tmp2 << "\"" << std::endl;
        return false;
    }
    return true;
}
