///-------------------------------------------------------------------------------------------------
/// 
/// \file CBias.hpp
/// \brief Class used to add Rebias to Other Classifier.
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

#pragma once

#include <Eigen/Dense>
#include <vector>
#include "geometry/Metrics.hpp"
#include "tinyxml2.h"

namespace Geometry {

/// <summary> Class For Bias Algorithm for covariance matrices. </summary>
class CBias
{
public:
    /// <summary> Initializes a new instance of the <see cref="CBias"/> class. </summary>
    CBias() = default;
    /// <summary> Finalizes an instance of the <see cref="CBias"/> class. </summary>
    ~CBias() = default;

    /// <summary> Computes the Bias matrix and reset the number of classification. </summary>
    /// <param name="dataset">  The dataset (first dimension is the classes, second dimension is the trials as matrix). </param>
    /// <param name="metric">   The metric. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool ComputeBias(const std::vector<std::vector<Eigen::MatrixXd>>& dataset, const EMetric metric = EMetric::Riemann);

    /// <summary> Computes the Bias matrix and reset the number of classification. </summary>
    /// <param name="dataset">  The dataset is a vector of trial as matrix. </param>
    /// <param name="metric">   The metric. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool ComputeBias(const std::vector<Eigen::MatrixXd>& dataset, const EMetric metric = EMetric::Riemann);

    /// <summary> Applies the Bias on 2D vector of Matrix. </summary>
    /// <param name="in">   The input 2D vector of matrix. </param>
    /// <param name="out">  The output 2D vector of matrix. </param>
    void ApplyBias(const std::vector<std::vector<Eigen::MatrixXd>>& in, std::vector<std::vector<Eigen::MatrixXd>>& out);
    /// <summary> Applies the Bias on vector of Matrix. </summary>
    /// <param name="in">   The input vector of matrix. </param>
    /// <param name="out">  The output vector of matrix. </param>
    void ApplyBias(const std::vector<Eigen::MatrixXd>& in, std::vector<Eigen::MatrixXd>& out);
    /// <summary> Applies the Bias on Matrix. </summary>
    /// <param name="in">   The input matrix. </param>
    /// <param name="out">  The output matrix. </param>
    void ApplyBias(const Eigen::MatrixXd& in, Eigen::MatrixXd& out);

    /// <summary> Updates the Bias. </summary>
    /// <param name="sample"> The sample. </param>
    /// <param name="metric"> The metric. </param>
    void UpdateBias(const Eigen::MatrixXd& sample, const EMetric metric = EMetric::Riemann);

    const Eigen::MatrixXd& GetBias() const { return m_bias; }   ///< Get the bias matrix.
    void SetBias(const Eigen::MatrixXd& bias);                  ///< Set the bias matrix and the inverse square root of biais.

    size_t GetClassificationNumber() const { return m_n; }      ///< Get the Number of classification (used for update).
    void SetClassificationNumber(const size_t& n) { m_n = n; }  ///< Set the Number of classification (used for update).

    //***********************
    //***** XML Manager *****
    //***********************
    /// <summary>   Saves the Bias information in an XML file. </summary>
    /// <param name="filename"> Filename. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool SaveXML(const std::string& filename) const;

    /// <summary>   Loads the Bias information from an XML file. </summary>
    /// <param name="filename"> Filename. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool LoadXML(const std::string& filename);

    /// <summary>   Save informations in xml element (Bias and number of classification). </summary>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool SaveAdditional(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* data) const;

    /// <summary>   Load informations in xml element (Bias and number of classification). </summary>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool LoadAdditional(tinyxml2::XMLElement* data);

    //*****************************
    //***** Override Operator *****
    //*****************************
    /// <summary>   Check if object are equals (with a precision tolerance). </summary>
    /// <param name="obj">          The second object. </param>
    /// <param name="precision">    Precision for matrix comparison. </param>
    /// <returns>   <c>True</c> if the two elements are equals (with a precision tolerance), <c>False</c> otherwise. </returns>
    bool IsEqual(const CBias& obj, const double precision = 1e-6) const;

    /// <summary>   Copy object value. </summary>
    /// <param name="obj">  The object to copy. </param>
    void Copy(const CBias& obj);

    /// <summary>   Get the Classifier information for output. </summary>
    /// <returns>   The Classifier print in stringstream. </returns>
    std::stringstream Print() const;

    /// <summary>   Override the affectation operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   The copied object. </returns>
    CBias& operator=(const CBias& obj)
    {
        Copy(obj);
        return *this;
    }

    /// <summary>   Override the equal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   <c>True</c> if the two <see cref="CBias"/> are equals. </returns>
    bool operator==(const CBias& obj) const { return IsEqual(obj); }

    /// <summary>   Override the not equal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   <c>True</c> if the two <see cref="CBias"/> are diffrents. </returns>
    bool operator!=(const CBias& obj) const { return !IsEqual(obj); }

    /// <summary>   Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns>   Return the modified ostream. </returns>
    friend std::ostream& operator <<(std::ostream& os, const CBias& obj)
    {
        os << obj.Print().str();
        return os;
    }

protected:
    //*********************
    //***** Variables *****
    //*********************
    size_t m_n = 0;             ///< Number of classification launched (used for update).
    Eigen::MatrixXd m_bias;     ///< Bias Matrix.
    Eigen::MatrixXd m_biasIS;   ///< Inverse squared root bias matrix (stored and pre-computed for application of bias).
};

}  // namespace Geometry
