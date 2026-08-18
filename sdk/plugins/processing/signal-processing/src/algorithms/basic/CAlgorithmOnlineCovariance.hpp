///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmOnlineCovariance.hpp
/// \brief Classes for the Algorithm Online Covariance.
/// \author Jussi T. Lindgren (Inria).
/// \version 0.5.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
///<summary>Incremental covariance estimators with shrinkage</summary>
class CAlgorithmOnlineCovariance final : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
    typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;

public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Algorithm_OnlineCovariance)

protected:
    // Debug method. Prints the matrix to the logManager. May be disabled in implementation.
    static void dumpMatrix(Kernel::ILogManager& mgr, const MatrixXdRowMajor& mat, const CString& desc);

    // These are non-normalized estimates for the corresp. statistics
    Eigen::MatrixXd m_cov;
    Eigen::MatrixXd m_mean;

    // The divisor for the above estimates to do the normalization
    uint64_t m_n = 0;
};

class CAlgorithmOnlineCovarianceDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Online Covariance"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Incrementally computes covariance with shrinkage."; }
    CString getDetailedDescription() const override { return "Regularized covariance output is computed as (diag*shrink + cov)"; }
    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "0.5"; }

    CIdentifier getCreatedClass() const override { return Algorithm_OnlineCovariance; }
    IPluginObject* create() override { return new CAlgorithmOnlineCovariance; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OnlineCovariance_InputParameterId_Shrinkage, "Shrinkage", Kernel::ParameterType_Float);
        prototype.addInputParameter(OnlineCovariance_InputParameterId_InputVectors, "Input vectors", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OnlineCovariance_InputParameterId_UpdateMethod, "Covariance update method", Kernel::ParameterType_Enumeration,
                                    TypeId_OnlineCovariance_UpdateMethod);
        prototype.addInputParameter(OnlineCovariance_InputParameterId_TraceNormalization, "Trace normalization", Kernel::ParameterType_Boolean);

        // The algorithm returns these outputs
        prototype.addOutputParameter(OnlineCovariance_OutputParameterId_Mean, "Mean vector", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OnlineCovariance_OutputParameterId_CovarianceMatrix, "Covariance matrix", Kernel::ParameterType_Matrix);

        prototype.addInputTrigger(OnlineCovariance_Process_Reset, "Reset the algorithm");
        prototype.addInputTrigger(OnlineCovariance_Process_Update, "Append a chunk of data");
        prototype.addInputTrigger(OnlineCovariance_Process_GetCov, "Get the current regularized covariance matrix & mean");
        prototype.addInputTrigger(OnlineCovariance_Process_GetCovRaw, "Get the current covariance matrix & mean");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Algorithm_OnlineCovarianceDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
