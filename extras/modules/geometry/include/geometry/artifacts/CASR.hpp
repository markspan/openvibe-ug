///-------------------------------------------------------------------------------------------------
/// 
/// \file CASR.hpp
/// \brief Class used to use Artifact Subspace Reconstruction Algorithm.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 27/08/2020.
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

#include <string>
#include <vector>
#include <Eigen/Dense>

#include "geometry/Basics.hpp"
#include "geometry/Metrics.hpp"

namespace Geometry {

/// <summary> Class For Artifact Subspace Reconstruction (ASR) Algorithm. </summary>
class CASR
{
public:
    CASR() = default;   ///< Initializes a new instance of the <see cref="CASR"/> class.

    /// <summary> Initializes a new instance of the <see cref="CASR"/> class with specified <c>metric</c>. </summary>
    /// <remarks> Only Euclidian and Riemmann metrics are implemented If other is selected, Euclidian is used. </remarks>
    explicit CASR(const EMetric& metric) { SetMetric(metric); }

    /// <summary> Initializes a new instance of the <see cref="CASR"/> class with specified <c>metric</c> and train with the specified <c>dataset</c>. </summary>
    /// <remarks> Only Euclidian and Riemmann metrics are implemented If other is selected, Euclidian is used. </remarks>
    explicit CASR(const EMetric& metric, const std::vector<Eigen::MatrixXd>& dataset)
    {
        SetMetric(metric);
        Train(dataset);
    }

    ~CASR() = default;  ///< Finalizes an instance of the <see cref="CASR"/> class.

    /// <summary>   Trains the specified dataset. </summary>
    /// <param name="dataset">  The dataset (Vector of signal window). </param>
    /// <param name="rejectionLimit">   The rejection limit. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool Train(const std::vector<Eigen::MatrixXd>& dataset, const double rejectionLimit = 5);

    /// <summary>   Apply the ASR algorithm to the input signal. </summary>
    /// <param name="in">   The input signal. </param>
    /// <param name="out">  The corrected signal. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool Process(const Eigen::MatrixXd& in, Eigen::MatrixXd& out);

    //***************************
    //***** Getter / Setter *****
    //***************************

    /// <summary> Set the metric to use (only Riemann and euclidian is used). </summary>
    /// <param name="metric">The metric. </param>
    /// <remarks> If invalid metric is used Euclidian is selected. </remarks>
    void SetMetric(const EMetric& metric) { m_metric = (metric == EMetric::Riemann) ? EMetric::Riemann : EMetric::Euclidian; }

    /// <summary> Sets the number of channel (dimension) to reconstruct in fraction, 0 for nothing 1 for all. </summary>
    /// <param name="max">  The maximum ratio. </param>
    /// <remarks>   If value isn't in [0;1], this function does nothing. </remarks>
    void SetMaxChannel(const double max) { if (InRange(max, 0.0, 1.0)) { m_maxChannel = max; } }

    /// <summary> Sets the differents matrices : median matrix, trheshold matrix, reconstruction matrix and covariance matrix. </summary>
    /// <param name="median">       The median matrix. </param>
    /// <param name="threshold">    The threshold matrix. </param>
    /// <param name="reconstruct">  (Optional) The reconstruct matrix. </param>
    /// <param name="covariance">   (Optional) The covariance matrix. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    /// <remarks>   All matrices must be square with same size (or empty for reconstruct and covariance matrix).
    /// Trivial trigger is set to true. </remarks>
    bool SetMatrices(const Eigen::MatrixXd& median, const Eigen::MatrixXd& threshold,
                     const Eigen::MatrixXd& reconstruct = Eigen::MatrixXd(), const Eigen::MatrixXd& covariance = Eigen::MatrixXd());

    EMetric GetMetric() const { return m_metric; }                      ///< Get the metric.
    size_t GetChannelNumber() const { return m_nChannel; }              ///< Get the matrices number of channel.
    double GetMaxChannel() const { return m_maxChannel; }               ///< Get the number of channel (dimension) to reconstruct in fraction.
    bool GetTrivial() const { return m_trivial; }                       ///< Get is last reconstruct was trivial (first time or if previous doesn't need reconstruct).
    Eigen::MatrixXd GetMedian() const { return m_median; }              ///< Get the median matrix.
    Eigen::MatrixXd GetThresholdMatrix() const { return m_threshold; }  ///< Get the threshold matrix.

    //***********************
    //***** XML Manager *****
    //***********************
    /// <summary>   Saves the ASR information in an XML file. </summary>
    /// <param name="filename"> Filename. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool SaveXML(const std::string& filename) const;

    /// <summary>   Loads the ASR information from an XML file. </summary>
    /// <param name="filename"> Filename. </param>
    /// <returns>   <c>True</c> if it succeeds, <c>False</c> otherwise. </returns>
    bool LoadXML(const std::string& filename);

    //*****************************
    //***** Override Operator *****
    //*****************************
    /// <summary>   Check if object are equals (with a precision tolerance). </summary>
    /// <param name="obj">          The second object. </param>
    /// <param name="precision">    Precision for matrix comparison. </param>
    /// <returns>   <c>True</c> if the two elements are equals (with a precision tolerance), <c>False</c> otherwise. </returns>
    bool IsEqual(const CASR& obj, const double precision = 1e-6) const;

    /// <summary>   Copy object value. </summary>
    /// <param name="obj">  The object to copy. </param>
    void Copy(const CASR& obj);

    /// <summary>   Get the ASR information for output. </summary>
    /// <returns>   The ASR print in stringstream. </returns>
    std::stringstream Print() const;

    /// <summary>   Override the affectation operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   The copied object. </returns>
    CASR& operator=(const CASR& obj)
    {
        Copy(obj);
        return *this;
    }

    /// <summary>   Override the equal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   <c>True</c> if the two <see cref="CASR"/> are equals. </returns>
    bool operator==(const CASR& obj) const { return IsEqual(obj); }

    /// <summary>   Override the not equal operator. </summary>
    /// <param name="obj">  The second object. </param>
    /// <returns>   <c>True</c> if the two <see cref="CASR"/> are diffrents. </returns>
    bool operator!=(const CASR& obj) const { return !IsEqual(obj); }

    /// <summary>   Override the ostream operator. </summary>
    /// <param name="os">   The ostream. </param>
    /// <param name="obj">  The object. </param>
    /// <returns>   Return the modified ostream. </returns>
    friend std::ostream& operator <<(std::ostream& os, const CASR& obj)
    {
        os << obj.Print().str();
        return os;
    }

protected:
    //*********************
    //***** Variables *****
    //*********************
    EMetric m_metric        = EMetric::Euclidian;   ///< Metric Used to compute (only euclidian and Riemann are implemented
    Eigen::Index m_nChannel = 0;                    ///< Number of channels (dimension)
    double m_maxChannel     = 1;                    ///< Maximum number of channels (dimension) to reconstruct if needed (in fraction, 0 for nothing 1 for all).
    bool m_trivial          = true;                 ///< Define if previous sample was trivial to reconstruct
    Eigen::MatrixXd m_median;                       ///< Median computed with train dataset
    Eigen::MatrixXd m_threshold;                    ///< Threshold matrix computed with train dataset
    Eigen::MatrixXd m_r;                            ///< Last Reconstruction matrix
    Eigen::MatrixXd m_cov;                          ///< Last Covariance matrix
};

}  // namespace Geometry
