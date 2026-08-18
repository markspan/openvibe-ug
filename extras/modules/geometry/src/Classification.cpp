///-------------------------------------------------------------------------------------------------
/// 
/// \file Classification.cpp
/// \brief All functions implementation to help Matrix Classifiers.
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
/// - LSQR inspired by <a href="http://scikit-learn.org">sklearn</a> <a href="https://scikit-learn.org/stable/modules/generated/sklearn.discriminant_analysis.LinearDiscriminantAnalysis.html">LinearDiscriminantAnalysis</a> (<a href="https://github.com/scikit-learn/scikit-learn/blob/master/COPYING">License</a>).
/// - FgDA inspired by the work of Alexandre Barachant : <a href="https://github.com/alexandrebarachant/pyRiemann">pyRiemann</a> (<a href="https://github.com/alexandrebarachant/pyRiemann/blob/master/LICENSE">License</a>).
/// 
///-------------------------------------------------------------------------------------------------

#include "geometry/Classification.hpp"
#include "geometry/Covariance.hpp"
#include "geometry/Basics.hpp"

namespace Geometry {

///-------------------------------------------------------------------------------------------------
bool LSQR(const std::vector<std::vector<Eigen::RowVectorXd>>& dataset, Eigen::MatrixXd& weight)
{
    // Precomputation
    if (dataset.empty()) { return false; }
    const Eigen::Index nbClass = Eigen::Index(dataset.size()), nbFeatures = dataset[0][0].size();
    std::vector<Eigen::Index> nbSample(nbClass);
    size_t totalSample = 0;
    for (Eigen::Index k = 0; k < nbClass; ++k) {
        if (dataset[k].empty()) { return false; }
        nbSample[k] = Eigen::Index(dataset[k].size());
        totalSample += nbSample[k];
    }

    // Compute Class Euclidian mean
    Eigen::MatrixXd mean = Eigen::MatrixXd::Zero(nbClass, nbFeatures);
    for (Eigen::Index k = 0; k < nbClass; ++k) {
        for (Eigen::Index i = 0; i < nbSample[k]; ++i) { mean.row(k) += dataset[k][i]; }
        mean.row(k) /= double(nbSample[k]);
    }

    // Compute Class Covariance
    Eigen::MatrixXd cov = Eigen::MatrixXd::Zero(nbFeatures, nbFeatures);
    for (Eigen::Index k = 0; k < nbClass; ++k) {
        //Fit Data to existing covariance matrix method
        Eigen::MatrixXd classData(nbFeatures, nbSample[k]);
        for (Eigen::Index i = 0; i < nbSample[k]; ++i) { classData.col(i) = dataset[k][i]; }

        // Standardize Features
        Eigen::RowVectorXd scale;
        MatrixStandardScaler(classData, scale);

        //Compute Covariance of this class
        Eigen::MatrixXd classCov;
        if (!CovarianceMatrix(classData, classCov, EEstimator::LWF)) { return false; }

        // Rescale
        for (Eigen::Index i = 0; i < nbFeatures; ++i) { for (Eigen::Index j = 0; j < nbFeatures; ++j) { classCov(i, j) *= scale[i] * scale[j]; } }

        //Add to cov with good weight
        cov += (double(nbSample[k]) / double(totalSample)) * classCov;
    }

    // linear least squares systems solver
    // Chosen solver with the performance table of this page : https://eigen.tuxfamily.org/dox/group__TutorialLinearAlgebra.html
    weight = cov.colPivHouseholderQr().solve(mean.transpose()).transpose();
    //weight = cov.completeOrthogonalDecomposition().solve(mean.transpose()).transpose();
    //weight = cov.bdcSvd(ComputeThinU | ComputeThinV).solve(mean.transpose()).transpose();

    // Treat binary case as a special case
    if (nbClass == 2) {
        const Eigen::MatrixXd tmp = weight.row(1) - weight.row(0);  // Need to use a tmp variable otherwise sometimes error
        weight                    = tmp;
    }
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool FgDACompute(const std::vector<std::vector<Eigen::RowVectorXd>>& dataset, Eigen::MatrixXd& weight)
{
    // Compute LSQR Weight
    Eigen::MatrixXd w;
    if (!LSQR(dataset, w)) { return false; }
    const Eigen::Index nbClass = w.rows();

    // Transform to FgDA Weight
    const Eigen::MatrixXd wT = w.transpose();
    weight                   = (wT * (w * wT).colPivHouseholderQr().solve(Eigen::MatrixXd::Identity(nbClass, nbClass))) * w;
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool FgDAApply(const Eigen::RowVectorXd& in, Eigen::RowVectorXd& out, const Eigen::MatrixXd& weight)
{
    if (in.cols() != weight.rows()) { return false; }
    out = in * weight;
    return true;
}
///-------------------------------------------------------------------------------------------------

}  // namespace Geometry
