///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmConditionedCovariance.hpp
/// \brief Classes for the Algorithm Conditioned Covariance.
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

#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CAlgorithmConditionedCovariance final : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
    typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;

public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override { return true; }
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Algorithm_ConditionedCovariance)

protected:
    // Debug method. Prints the matrix to the logManager. May be disabled in implementation.
    static void dumpMatrix(Kernel::ILogManager& mgr, const MatrixXdRowMajor& mat, const CString& desc);
};

class CAlgorithmConditionedCovarianceDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Conditioned Covariance"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Computes covariance with shrinkage."; }

    CString getDetailedDescription() const override
    {
        return
                "Shrinkage: {<0 = auto-estimate, [0,1] balance between prior and sample cov}. The conditioned covariance matrix may allow better accuracies with models that rely on inverting the cov matrix, in cases where the regular cov matrix is close to singular.";
    }

    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Algorithm_ConditionedCovariance; }
    IPluginObject* create() override { return new CAlgorithmConditionedCovariance; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(ConditionedCovariance_InputParameterId_Shrinkage, "Shrinkage (-1 == auto)", Kernel::ParameterType_Float);
        prototype.addInputParameter(ConditionedCovariance_InputParameterId_FeatureVectorSet, "Feature vectors", Kernel::ParameterType_Matrix);

        // The algorithm returns these outputs
        prototype.addOutputParameter(ConditionedCovariance_OutputParameterId_Mean, "Mean vector", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(ConditionedCovariance_OutputParameterId_CovarianceMatrix, "Covariance matrix", Kernel::ParameterType_Matrix);

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Algorithm_ConditionedCovarianceDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
