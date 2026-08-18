///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmMatrixAverage.hpp
/// \brief Classes for the Algorithm Matrix average.
/// \author Yann Renard (Inria/IRISA).
/// \version 1.1.
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

#include <vector>
#include <deque>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CAlgorithmMatrixAverage final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Algorithm_MatrixAverage)

protected:
    Kernel::TParameterHandler<uint64_t> ip_averagingMethod;
    Kernel::TParameterHandler<uint64_t> ip_matrixCount;
    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<CMatrix*> op_averagedMatrix;

    std::deque<CMatrix*> m_history;
    std::vector<double> m_averageMatrices;
    size_t m_nAverageSamples = 0;
};

class CAlgorithmMatrixAverageDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix average"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria/IRISA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Averaging"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Algorithm_MatrixAverage; }
    IPluginObject* create() override { return new CAlgorithmMatrixAverage(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(MatrixAverage_InputParameterId_Matrix, "Matrix", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(MatrixAverage_InputParameterId_MatrixCount, "Matrix count", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(MatrixAverage_InputParameterId_AveragingMethod, "Averaging Method", Kernel::ParameterType_UInteger);

        prototype.addOutputParameter(MatrixAverage_OutputParameterId_AveragedMatrix, "Averaged matrix", Kernel::ParameterType_Matrix);

        prototype.addInputTrigger(MatrixAverage_InputTriggerId_Reset, "Reset");
        prototype.addInputTrigger(MatrixAverage_InputTriggerId_FeedMatrix, "Feed matrix");
        prototype.addInputTrigger(MatrixAverage_InputTriggerId_ForceAverage, "Force average");

        prototype.addOutputTrigger(MatrixAverage_OutputTriggerId_AveragePerformed, "Average performed");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Algorithm_MatrixAverageDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
