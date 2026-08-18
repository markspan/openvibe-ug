///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixClassifierFgMDM.cpp
/// \brief Class implementation of Minimum Distance to Mean with geodesic filtering (FgMDM) Classifier.
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

#include "geometry/classifier/CMatrixClassifierFgMDM.hpp"
#include "geometry/Mean.hpp"

namespace Geometry {

///-------------------------------------------------------------------------------------------------
CMatrixClassifierFgMDM::~CMatrixClassifierFgMDM()
{
    for (auto& v : m_dataset) { v.clear(); }
    m_dataset.clear();
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDM::Train(const std::vector<std::vector<Eigen::MatrixXd>>& dataset)
{
    m_dataset = dataset;
    return train();
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CMatrixClassifierFgMDM::Classify(const Eigen::MatrixXd& sample, size_t& classId, std::vector<double>& distance,
                                      std::vector<double>& probability, const EAdaptations adaptation, const size_t& realClassId)
{
    if (!CMatrixClassifierFgMDMRT::Classify(sample, classId, distance, probability, EAdaptations::None)) { return false; }

    // Adaptation
    if (adaptation == EAdaptations::None) { return true; }
    // Get class id for adaptation and increase number of trials, expected if supervised, predicted if unsupervised
    const size_t id = adaptation == EAdaptations::Supervised ? realClassId : classId;
    if (id >= m_nbClass) { return false; }                  // Check id (if supervised and bad input)
    m_nbTrials[id]++;                                       // Update number of trials for the class id
    m_dataset[id].push_back(sample);                        // Update the dataset

    // Retrain 
    return train();
}
///-------------------------------------------------------------------------------------------------

}  // namespace Geometry
