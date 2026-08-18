///-------------------------------------------------------------------------------------------------
/// 
/// \file CMatrixClassifierFgMDMRTRebias.hpp
/// \brief Class of Minimum Distance to Mean with geodesic filtering (FgMDM) Classifier RT (adaptation is Real Time Assumed)
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
#include "geometry/classifier/CBias.hpp"

namespace Geometry {

/// <summary>   Class of Minimum Distance to Mean with geodesic filtering (FgMDM) Classifier RT (adaptation is Real Time Assumed). </summary>
/// <seealso cref="CMatrixClassifierFgMDMRT" />
class CMatrixClassifierFgMDMRTRebias final : public CMatrixClassifierFgMDMRT
{
public:
    //***********************   
    //***** Constructor *****
    //***********************   
    /// <summary>   Initializes a new instance of the <see cref="CMatrixClassifierFgMDMRTRebias"/> class. </summary>
    CMatrixClassifierFgMDMRTRebias() = default;

    /// <summary>   Default Copy constructor. Initializes a new instance of the <see cref="CMatrixClassifierFgMDMRTRebias"/> class. </summary>
    /// <param name="obj">  Initial object. </param>
    CMatrixClassifierFgMDMRTRebias(const CMatrixClassifierFgMDMRTRebias& obj) : CMatrixClassifierFgMDMRT(obj) { *this = obj; }

    /// <summary>   Initializes a new instance of the <see cref="CMatrixClassifierFgMDMRTRebias"/> class and set base members. </summary>
    /// <param name="nbClass">  The number of classes. </param>
    /// <param name="metric">   Metric to use to calculate means (see also <see cref="EMetric" />). </param>
    explicit CMatrixClassifierFgMDMRTRebias(const size_t nbClass, const EMetric metric) : CMatrixClassifierFgMDMRT(nbClass, metric) { }

    /// <summary>   Finalizes an instance of the <see cref="CMatrixClassifierFgMDMRTRebias"/> class. </summary>
    /// <remarks>   clear the <see cref="m_means"/> vector of Matrix. </remarks>
    ~CMatrixClassifierFgMDMRTRebias() override = default;

    //***************************
    //***** Getter / Setter *****
    //***************************
    const CBias& GetBias() const { return m_bias; }      ///< Get Rebias Method. 
    void SetBias(const CBias& bias) { m_bias = bias; }   ///< Set Rebias Method. 

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
    bool Train(const std::vector<std::vector<Eigen::MatrixXd>>& dataset) override;

    /// <summary>   Classify the matrix and return the class id, the distance and the probability of each class.\n
    /// -# Transform the sample to the Tangent Space.\n
    /// -# Apply the FgDA weight.\n
    /// -# Return to the original Manifold.\n
    /// -# Apply the classify function of MDM Classifier (see <see cref="CMatrixClassifierMDM::classify"/>)
    /// </summary>
    /// <remarks>
    /// <b>Remark</b> : We use the MDM classification whatever the adaptation method chosen. 
    /// Thus the MDM part evolves but the geodesic filtering does not evolve to keep an execution online. 
    /// A version allowing the adaptation of the Filter will be implemented for offline execution.
    /// </remarks>
    /// \copydetails IMatrixClassifier::Classify(const Eigen::MatrixXd&, size_t&, std::vector<double>&, std::vector<double>&, const EAdaptations, const size_t&)
    bool Classify(const Eigen::MatrixXd& sample, size_t& classId, std::vector<double>& distance, std::vector<double>& probability,
                  EAdaptations adaptation = EAdaptations::None, const size_t& realClassId = std::numeric_limits<size_t>::max()) override;

    //*****************************
    //***** Override Operator *****
    //*****************************

    /// <summary>   Check if object are equals (with a precision tolerance). </summary>
    /// <param name="obj">          The second object. </param>
    /// <param name="precision">    Precision for matrix comparison. </param>
    /// <returns>   <c>True</c> if the two elements are equals (with a precision tolerance). </returns>
    bool IsEqual(const CMatrixClassifierFgMDMRTRebias& obj, double precision = 1e-6) const;

    /// <summary>   Copy object value. </summary>
    /// <param name="obj">  The object to copy. </param>
    void Copy(const CMatrixClassifierFgMDMRTRebias& obj);

    /// <summary>   Get the type of the classifier. </summary>
    /// <returns>   Minimum Distance to Mean with geodesic filtering (FgMDM). </returns>
    std::string GetType() const override { return toString(EMatrixClassifiers::FgMDM_RT_Rebias); }

    /// <summary>   Override the affectation operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   The copied object. </returns>
    CMatrixClassifierFgMDMRTRebias& operator=(const CMatrixClassifierFgMDMRTRebias& obj)
    {
        Copy(obj);
        return *this;
    }

    /// <summary>   Override the equal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   <c>True</c> if the two <see cref="CMatrixClassifierFgMDMRTRebias"/> are equals. </returns>
    bool operator==(const CMatrixClassifierFgMDMRTRebias& obj) const { return IsEqual(obj); }

    /// <summary>   Override the not equal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   <c>True</c> if the two <see cref="CMatrixClassifierFgMDMRTRebias"/> are diffrents. </returns>
    bool operator!=(const CMatrixClassifierFgMDMRTRebias& obj) const { return !IsEqual(obj); }

    /// <summary>   Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns>   Return the modified ostream. </returns>
    friend std::ostream& operator <<(std::ostream& os, const CMatrixClassifierFgMDMRTRebias& obj)
    {
        os << obj.Print().str();
        return os;
    }

protected:
    //***********************
    //***** XML Manager *****
    //***********************
    /// <summary>   Save Additionnal informations (Reference and LDA Weight). </summary>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool saveAdditional(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* data) const override;

    /// <summary>   Load Additionnal informations (Reference and LDA Weight). </summary>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool loadAdditional(tinyxml2::XMLElement* data) override;

    /// <summary>   Prints the Additional informations (Reference and LDA Weight). </summary>
    /// <returns>   Additional informations in stringstream. </returns>
    std::stringstream printAdditional() const override;

    //*********************
    //***** Variables *****
    //*********************
    CBias m_bias;   ///< Rebias Method. 
};

}  // namespace Geometry
