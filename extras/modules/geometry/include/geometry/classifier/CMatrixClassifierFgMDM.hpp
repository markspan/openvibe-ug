///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixClassifierFgMDM.hpp
/// \brief Class of Minimum Distance to Mean with geodesic filtering (FgMDM) Classifier.
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

#pragma once

#include "geometry/classifier/CMatrixClassifierFgMDMRT.hpp"

namespace Geometry {

/// <summary>   Class of Minimum Distance to Mean with geodesic filtering (FgMDM) Classifier. </summary>
/// <seealso cref="CMatrixClassifierMDM" />
class CMatrixClassifierFgMDM final : public CMatrixClassifierFgMDMRT
{
public:
    //***********************   
    //***** Constructor *****
    //***********************   
    /// <summary>   Initializes a new instance of the <see cref="CMatrixClassifierFgMDM"/> class. </summary>
    CMatrixClassifierFgMDM() = default;

    /// <summary>   Default Copy constructor. Initializes a new instance of the <see cref="CMatrixClassifierFgMDM"/> class. </summary>
    /// <param name="obj">  Initial object. </param>
    CMatrixClassifierFgMDM(const CMatrixClassifierFgMDM& obj) : CMatrixClassifierFgMDMRT(obj) { *this = obj; }

    /// <summary>   Copy constructor with parent class. Initializes a new instance of the <see cref="CMatrixClassifierFgMDM"/> class. </summary>
    /// <param name="obj">  Initial object. </param>
    explicit CMatrixClassifierFgMDM(const CMatrixClassifierFgMDMRT& obj) { Copy(obj); }

    /// <summary>   Initializes a new instance of the <see cref="CMatrixClassifierFgMDM"/> class and set base members. </summary>
    /// <param name="nbClass">  The number of classes. </param>
    /// <param name="metric">   Metric to use to calculate means (see also <see cref="EMetric" />). </param>
    explicit CMatrixClassifierFgMDM(const size_t nbClass, const EMetric metric) : CMatrixClassifierFgMDMRT(nbClass, metric) { }

    /// <summary>   Finalizes an instance of the <see cref="CMatrixClassifierFgMDM"/> class. </summary>
    /// <remarks>   clear the <see cref="m_means"/> vector of Matrix and the <see cref="m_dataset"/> member. </remarks>
    ~CMatrixClassifierFgMDM() override;

    //***************************
    //***** Getter / Setter *****
    //***************************
    void SetDataset(const std::vector<std::vector<Eigen::MatrixXd>>& dataset) { m_dataset = dataset; }  ///< Set dataset.
    const std::vector<std::vector<Eigen::MatrixXd>>& GetDataset() const { return m_dataset; }           ///< Get dataset.

    //**********************
    //***** Classifier *****
    //**********************
    /// <summary>   Train the classifier with the dataset.
    /// -# Compute the Riemann mean of all trials as reference and store this in <see cref="m_ref"/> member.
    /// -# Set the good number of classes
    /// -# Trasnform data to the Tangent Space with the reference
    /// -# Compute the FgDA Weight (<see cref="FgDACompute" />).
    /// -# Apply the FgDA Weight and return to Original Manifold.
    /// -# Apply the train function of MDM Classifier (see <see cref="CMatrixClassifierMDM::train"/>)
    /// </summary>
    /// <param name="dataset">  The dataset (first dimension is the classes, second dimension is the trials as covariance matrix). </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    /// <remarks>   the dataset is saved. </remarks>
    bool Train(const std::vector<std::vector<Eigen::MatrixXd>>& dataset) override;

    /// <summary>   Classify the matrix and return the class id, the distance and the probability of each class.\n
    /// -# Transform the sample to the Tangent Space.\n
    /// -# Apply the FgDA weight.\n
    /// -# Return to the original Manifold.\n
    /// -# Apply the classify function of MDM Classifier (see <see cref="CMatrixClassifierMDM::classify"/>)
    /// </summary>
    /// <remarks>   The classifier is train with the new sample. </remarks>
    /// \copydetails IMatrixClassifier::Classify(const Eigen::MatrixXd&, size_t&, std::vector<double>&, std::vector<double>&, const EAdaptations, const size_t&)
    bool Classify(const Eigen::MatrixXd& sample, size_t& classId, std::vector<double>& distance, std::vector<double>& probability,
                  EAdaptations adaptation = EAdaptations::None, const size_t& realClassId = std::numeric_limits<size_t>::max()) override;


    //*****************************
    //***** Override Operator *****
    //*****************************

    /// <summary>   Get the type of the classifier. </summary>
    /// <returns>   Minimum Distance to Mean with geodesic filtering (FgMDM). </returns>
    std::string GetType() const override { return toString(EMatrixClassifiers::FgMDM); }

    /// <summary>   Override the affectation operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   The copied object. </returns>
    CMatrixClassifierFgMDM& operator=(const CMatrixClassifierFgMDM& obj)
    {
        Copy(obj);
        return *this;
    }

    /// <summary>   Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns>   Return the modified ostream. </returns>
    friend std::ostream& operator <<(std::ostream& os, const CMatrixClassifierFgMDM& obj)
    {
        os << obj.Print().str();
        return os;
    }

protected:
    ///<summary> train with the actual dataset (<see cref="m_dataset"/>). </summary>
    bool train() { return CMatrixClassifierFgMDMRT::Train(m_dataset); }

    //*********************
    //***** Variables *****
    //*********************
    std::vector<std::vector<Eigen::MatrixXd>> m_dataset;    ///< Data set for train and adaptation (it can quickly rise).
};

}  // namespace Geometry
