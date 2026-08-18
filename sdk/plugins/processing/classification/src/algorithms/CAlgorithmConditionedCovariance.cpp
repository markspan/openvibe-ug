///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmConditionedCovariance.cpp
/// \brief Classes implementation for the Algorithm Conditioned Covariance.
/// \author Jussi T. Lindgren (Inria).
/// \version 1.0.
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

#include "CAlgorithmConditionedCovariance.hpp"

/*
 * This implementation is based on the matlab code corresponding to
 *
 * Ledoit & Wolf: "A Well-Conditioned Estimator for Large-Dimensional Covariance Matrices", 2004.
 *
 */
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

#define COV_DEBUG 0
#if COV_DEBUG
void CAlgorithmConditionedCovariance::dumpMatrix(Kernel::ILogManager &mgr, const MatrixXdRowMajor &mat, const CString &desc)
{
    mgr << Kernel::LogLevel_Info << desc << "\n";
    for (int i = 0 ; i < mat.rows() ; i++)
    {
        mgr << Kernel::LogLevel_Info << "Row " << i << ": ";
        for (int j = 0 ; j < mat.cols() ; j++) { mgr << mat(i,j) << " "; }
        mgr << "\n";
    }
}
#else
void CAlgorithmConditionedCovariance::dumpMatrix(Kernel::ILogManager& /* mgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { }
#endif

bool CAlgorithmConditionedCovariance::initialize()
{
    // Default value setting
    Kernel::TParameterHandler<double> ip_shrinkage(getInputParameter(ConditionedCovariance_InputParameterId_Shrinkage));
    ip_shrinkage = -1.0;

    return true;
}

bool CAlgorithmConditionedCovariance::process()
{
    // Set up the IO
    const Kernel::TParameterHandler<double> ip_shrinkage(getInputParameter(ConditionedCovariance_InputParameterId_Shrinkage));
    const Kernel::TParameterHandler<CMatrix*> ip_sample(getInputParameter(ConditionedCovariance_InputParameterId_FeatureVectorSet));
    Kernel::TParameterHandler<CMatrix*> op_mean(getOutputParameter(ConditionedCovariance_OutputParameterId_Mean));
    Kernel::TParameterHandler<CMatrix*> op_covMatrix(getOutputParameter(ConditionedCovariance_OutputParameterId_CovarianceMatrix));
    double shrinkage = ip_shrinkage;

    OV_ERROR_UNLESS_KRF(shrinkage <= 1.0, "Invalid shrinkage value " << shrinkage << "(expected value <= 1.0)", Kernel::ErrorType::BadConfig);


    OV_ERROR_UNLESS_KRF(ip_sample->getDimensionCount() == 2,
                        "Invalid dimension count for vector set " << ip_sample->getDimensionCount() << "(expected value = 2)", Kernel::ErrorType::BadInput);

    const size_t nRows = ip_sample->getDimensionSize(0);
    const size_t nCols = ip_sample->getDimensionSize(1);

    OV_ERROR_UNLESS_KRF(nRows >= 1 && nCols >= 1, "Invalid input matrix [" << nRows << "x" << nCols << "] (expected at least 1x1 size)",
                        Kernel::ErrorType::BadInput);

    const double* buffer = ip_sample->getBuffer();


    OV_ERROR_UNLESS_KRF(buffer, "Invalid NULL feature set buffer", Kernel::ErrorType::BadInput);

    // Set the output buffers so we can write the results to them without copy
    op_mean->resize(1, nCols);
    op_covMatrix->resize(nCols, nCols);

    // Insert our data into an Eigen matrix. As Eigen doesn't have const double* constructor, we cast away the const.
    const Eigen::Map<MatrixXdRowMajor> dataMatrix(const_cast<double*>(buffer), nRows, nCols);

    // Estimate the data center and center the data
    Eigen::Map<MatrixXdRowMajor> dataMean(op_mean->getBuffer(), 1, nCols);
    dataMean                            = dataMatrix.colwise().mean();
    const MatrixXdRowMajor dataCentered = dataMatrix.rowwise() - dataMean.row(0);

    // Compute the sample cov matrix
    const Eigen::MatrixXd sampleCov = (dataCentered.transpose() * dataCentered) * (1 / double(nRows));

    // Compute the prior cov matrix
    Eigen::MatrixXd priorCov = Eigen::MatrixXd::Zero(nCols, nCols);
    priorCov.diagonal().setConstant(sampleCov.diagonal().mean());

    // Compute shrinkage coefficient if its not given
    if (shrinkage < 0) {
        const Eigen::MatrixXd dataSquared = dataCentered.cwiseProduct(dataCentered);
        const Eigen::MatrixXd phiMat      = (dataSquared.transpose() * dataSquared) / double(nRows) - sampleCov.cwiseAbs2();

        const double phi   = phiMat.sum();
        const double gamma = (sampleCov - priorCov).squaredNorm();  // Frobenius norm
        const double kappa = phi / gamma;

        shrinkage = std::max<double>(0, std::min<double>(1, kappa / double(nRows)));

        this->getLogManager() << Kernel::LogLevel_Debug << "Phi " << phi << " Gamma " << gamma << " kappa " << kappa << "\n";
        this->getLogManager() << Kernel::LogLevel_Debug << "Estimated shrinkage weight to be " << shrinkage << "\n";

        dumpMatrix(this->getLogManager(), phiMat, "PhiMat");
    }
    else { this->getLogManager() << Kernel::LogLevel_Debug << "Using user-provided shrinkage weight " << shrinkage << "\n"; }

    // Use the output as a buffer to avoid copying
    Eigen::Map<MatrixXdRowMajor> oCov(op_covMatrix->getBuffer(), nCols, nCols);

    // Mix the prior and the sample estimates according to the shrinkage parameter
    oCov = shrinkage * priorCov + (1.0 - shrinkage) * sampleCov;

    // Debug block
    dumpMatrix(this->getLogManager(), dataMean, "DataMean");
    dumpMatrix(this->getLogManager(), sampleCov, "Sample cov");
    dumpMatrix(this->getLogManager(), priorCov, "Prior cov");
    dumpMatrix(this->getLogManager(), oCov, "Output cov");

    return true;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
