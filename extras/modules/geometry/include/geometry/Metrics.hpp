///-------------------------------------------------------------------------------------------------
/// 
/// \file Metrics.hpp
/// \brief All Metrics.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 26/10/2018.
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
/// \remarks 
/// - List of Metrics inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a> (<a href="https://github.com/alexandrebarachant/pyRiemann/blob/master/LICENSE">License</a>).
/// 
///-------------------------------------------------------------------------------------------------

#pragma once
#include <string>

namespace Geometry {

/// <summary>   Enumeration of metrics. Inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a>. </summary>
enum class EMetric
{
    Riemann,        ///< The Riemannian Metric.
    Euclidian,      ///< The Euclidian Metric.
    LogEuclidian,   ///< The Log Euclidian Metric.
    LogDet,         ///< The Log Determinant Metric.
    Kullback,       ///< The Kullback Metric.
    ALE,            ///< The AJD-based log-Euclidean (ALE) Metric.
    Harmonic,       ///< The Harmonic Metric.
    Wasserstein,    ///< The Wasserstein Metric.
    Identity        ///< The Identity Metric.
};

/// <summary>   Convert metric to string. </summary>
/// <param name="metric">   The metric. </param>
/// <returns>   <c>std::string</c> </returns>
inline std::string toString(const EMetric metric)
{
    switch (metric) {
        case EMetric::Riemann: return "Riemann";
        case EMetric::Euclidian: return "Euclidian";
        case EMetric::LogEuclidian: return "Log Euclidian";
        case EMetric::LogDet: return "Log Determinant";
        case EMetric::Kullback: return "Kullback";
        case EMetric::ALE: return "AJD-based log-Euclidean";
        case EMetric::Harmonic: return "Harmonic";
        case EMetric::Wasserstein: return "Wasserstein";
        case EMetric::Identity: return "Identity";
    }
    return "Invalid Metric";
}

/// <summary>   Convert string to metric. </summary>
/// <param name="metric">   The metric. </param>
/// <returns>   <see cref="EMetric"/> </returns>
inline EMetric StringToMetric(const std::string& metric)
{
    if (metric == "Riemann") { return EMetric::Riemann; }
    if (metric == "Euclidian") { return EMetric::Euclidian; }
    if (metric == "Log Euclidian") { return EMetric::LogEuclidian; }
    if (metric == "Log Determinant") { return EMetric::LogDet; }
    if (metric == "Kullback") { return EMetric::Kullback; }
    if (metric == "AJD-based log-Euclidean") { return EMetric::ALE; }
    if (metric == "Harmonic") { return EMetric::Harmonic; }
    if (metric == "Wasserstein") { return EMetric::Wasserstein; }
    return EMetric::Identity;
}

}  // namespace Geometry
