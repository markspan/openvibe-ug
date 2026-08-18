///-------------------------------------------------------------------------------------------------
/// 
/// \file RiemannianPotato.hpp
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

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <tinyxml2.h>
#include <Eigen/Dense>


namespace OpenViBE {
namespace Plugins {
namespace Riemannian {

class RiemannianPotato
{
public:
    RiemannianPotato();
    RiemannianPotato(Eigen::MatrixXd cov, double distMean, double distStd);

    /**
     * @brief Computes the z-score, given the current state of the potato and the new covariance matrix
     * 
     * @param cov The new covariance matrix
     * @return The computed z-score 
     */
    double computeZScore(Eigen::MatrixXd cov); 
    /**
     * @brief Updates the values of Σ-bar, µ, and σ. Should be called if the computed z-score was below the RPF threshold
     * 
     * @param alpha The speed of adaptation
     */
    void update(double alpha);

    void serializeToXML(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent) const;
    static RiemannianPotato deserializeFromXML(const tinyxml2::XMLElement* element);

private:
    Eigen::MatrixXd m_refCov; // the reference covariance matrix (Σ-bar), computed from the artifact-free parts of the signal
    double m_distanceMean; // the mean (µ) of all the distances previously computed between the received clean (= non artifact) cov matrices and the refCov
    double m_distanceStd; // the standard deviation (σ) of all the distances previously computed between the received clean (=non artifact) cov matrices and the refCov

    Eigen::MatrixXd m_cov; // the latest cov
    double m_distance; // the distance between the lastest cov and m_refCov
};

// 
void saveToXML(const std::vector<RiemannianPotato>& potatoes, const std::string& filename);
std::vector<RiemannianPotato> loadFromXML(const std::string& filename);

}
}
}