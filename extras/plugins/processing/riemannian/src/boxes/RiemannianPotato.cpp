///-------------------------------------------------------------------------------------------------
/// 
/// \file RiemannianPotato.cpp
/// \brief Class riemannian potato
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Thu Dec 05 10:00:00 2024.
/// \Copyright (C) 2024 Inria
///
/// This classes originate from the riemannian potato box developped in openvibe in 2012 by Andreev A., Barachant A., Congedo M., Ionescu,Gelu (Gipsa-Lab)
/// And implement the algorithms described in "The Riemannian Potato Field: a tool for online Signal Quality Index of EEG" by Quentin Barthelemy, Louis Mayaud, David Ojeda and Marco Congedo (2019)
/// The equations labelled with (n) correspond to the ones described in this paper
/// 
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
/// 
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

# pragma once

#include "RiemannianPotato.hpp"

#include "geometry/Distance.hpp"
#include "geometry/Geodesic.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {

RiemannianPotato::RiemannianPotato() {
    m_distanceMean = 0.0;
    m_distanceStd = 0.0;
    m_distance = 0.0;
};

RiemannianPotato::RiemannianPotato(Eigen::MatrixXd cov, double distMean, double distStd) {
    m_refCov = cov;
    m_distanceMean = distMean;
    m_distanceStd = m_distanceStd;
}

double RiemannianPotato::computeZScore(Eigen::MatrixXd cov) {
    m_cov = cov;
    if(m_refCov.rows() == 0 || m_refCov.cols() == 0) {
        m_refCov = cov;
        m_distanceMean = 0;
        m_distanceStd = 0;
        
        return 0;
    }
    m_distance = Geometry::DistanceRiemann(cov, m_refCov);

    if (m_distanceStd == 0) {
        return 0;
    }
    double zScore = (m_distance - m_distanceMean) / m_distanceStd;
    return zScore;
}

void RiemannianPotato::update(double alpha) {

    Eigen::MatrixXd tmp;
    
    Geometry::GeodesicRiemann(m_refCov, m_cov, tmp); // (5)
    m_refCov = tmp;

    m_distanceMean = (1.0 - alpha) * m_distanceMean + alpha * m_distance; // (6)

    m_distanceStd = sqrt((1.0 - alpha) * m_distanceStd + alpha * pow(m_distance - m_distanceMean, 2)); // (7)

}

// Serialization to XML
void RiemannianPotato::serializeToXML(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent) const {
    tinyxml2::XMLElement* potatoElement = doc.NewElement("RiemannianPotato");
    parent->InsertEndChild(potatoElement);

    // Serialize m_refCov
    tinyxml2::XMLElement* refCovElement = doc.NewElement("ReferenceCovariance");
    for (int i = 0; i < m_refCov.rows(); ++i) {
        for (int j = 0; j < m_refCov.cols(); ++j) {
            tinyxml2::XMLElement* entry = doc.NewElement("Entry");
            entry->SetAttribute("value", m_refCov(i, j));
            refCovElement->InsertEndChild(entry);
        }
    }
    potatoElement->InsertEndChild(refCovElement);

    // Serialize other members
    potatoElement->SetAttribute("DistanceMean", m_distanceMean);
    potatoElement->SetAttribute("DistanceStd", m_distanceStd);
}

// Deserialization from XML
RiemannianPotato RiemannianPotato::deserializeFromXML(const tinyxml2::XMLElement* element) {
    // Deserialize m_refCov
    Eigen::MatrixXd refCov;
    const tinyxml2::XMLElement* refCovElement = element->FirstChildElement("ReferenceCovariance");
    std::vector<double> refCovValues;
    for (const tinyxml2::XMLElement* entry = refCovElement->FirstChildElement("Entry"); entry != nullptr; entry = entry->NextSiblingElement("Entry")) {
        double value = entry->DoubleAttribute("value");
        refCovValues.push_back(value);
    }
    int refCovRows = static_cast<int>(sqrt(refCovValues.size())); // Assuming square matrix
    refCov.resize(refCovRows, refCovRows);
    for (int i = 0; i < refCovRows; ++i) {
        for (int j = 0; j < refCovRows; ++j) {
            refCov(i, j) = refCovValues[i * refCovRows + j];
        }
    }

    double distMean = element->DoubleAttribute("DistanceMean");
    double distStd = element->DoubleAttribute("DistanceStd");

    // Create and return the object
    RiemannianPotato potato(refCov, distMean, distStd);
    return potato;
}

// Save vector of RiemannianPotato objects to XML
void saveToXML(const std::vector<RiemannianPotato>& potatoes, const std::string& filename) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement* root = doc.NewElement("RiemannianPotatoes");
    doc.InsertFirstChild(root);
    
    for (const auto& potato : potatoes) {
        potato.serializeToXML(doc, root);
    }

    if (doc.SaveFile(filename.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error saving to XML file: " << filename << std::endl;
    }
}

// Load vector of RiemannianPotato objects from XML
std::vector<RiemannianPotato> loadFromXML(const std::string& filename) {
    tinyxml2::XMLDocument doc;
    std::vector<RiemannianPotato> potatoes;

    if (doc.LoadFile(filename.c_str()) == tinyxml2::XML_SUCCESS) {
        const tinyxml2::XMLElement* root = doc.FirstChildElement("RiemannianPotatoes");
        for (const tinyxml2::XMLElement* potatoElement = root->FirstChildElement("RiemannianPotato"); potatoElement != nullptr; 
            potatoElement = potatoElement->NextSiblingElement("RiemannianPotato")) {
                potatoes.push_back(RiemannianPotato::deserializeFromXML(potatoElement));
            }
        } else {
        std::cerr << "Error loading XML file: " << filename << std::endl;
    }

    return potatoes;
}

}
}
}

