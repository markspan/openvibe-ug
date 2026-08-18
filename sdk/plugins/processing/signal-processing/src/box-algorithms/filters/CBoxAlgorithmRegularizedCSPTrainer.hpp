///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRegularizedCSPTrainer.hpp
/// \brief Classes for the Box Regularized CSP Trainer.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "../../algorithms/basic/CAlgorithmOnlineCovariance.hpp"

#include <Eigen/Eigenvalues>

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmRegularizedCSPTrainer final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

protected:
    bool updateCov(size_t index);
    bool outclassCovAverage(size_t skipIndex, const std::vector<Eigen::MatrixXd>& cov, Eigen::MatrixXd& covAvg);
    bool computeCSP(const std::vector<Eigen::MatrixXd>& cov, std::vector<Eigen::MatrixXd>& sortedEigenVectors, std::vector<Eigen::VectorXd>& sortedEigenValues);

    Toolkit::TStimulationDecoder<CBoxAlgorithmRegularizedCSPTrainer> m_stimDecoder;
    std::vector<Toolkit::TSignalDecoder<CBoxAlgorithmRegularizedCSPTrainer>> m_signalDecoders;
    Toolkit::TStimulationEncoder<CBoxAlgorithmRegularizedCSPTrainer> m_encoder;

    uint64_t m_stimID = 0;
    CString m_configFilename;
    size_t m_filtersPerClass  = 0;
    bool m_saveAsBoxConf      = false;
    bool m_hasBeenInitialized = false;

    double m_tikhonov = 0.0;

    struct SIncrementalCovarianceProxy
    {
        Kernel::IAlgorithmProxy* cov = nullptr;
        size_t nBuffers              = 0;
        size_t nSamples              = 0;
    };

    std::vector<SIncrementalCovarianceProxy> m_covProxies;

    size_t m_nClasses = 0;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_RegularizedCSPTrainer)
};

class CBoxAlgorithmRegularizedCSPTrainerListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputName(index, ("Signal condition " + std::to_string(index)).c_str());
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmRegularizedCSPTrainerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Regularized CSP Trainer"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Computes Common Spatial Pattern filters with regularization"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Filtering"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_RegularizedCSPTrainer; }
    IPluginObject* create() override { return new CBoxAlgorithmRegularizedCSPTrainer; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmRegularizedCSPTrainerListener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Signal condition 1", OV_TypeId_Signal);
        prototype.addInput("Signal condition 2", OV_TypeId_Signal);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);

        prototype.addSetting("Train Trigger", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Session");
        prototype.addSetting("Spatial filter configuration", OV_TypeId_Filename, "");
        prototype.addSetting("Filters per condition", OV_TypeId_Integer, "2");
        prototype.addSetting("Save filters as box config", OV_TypeId_Boolean, "false");

        // Params of the cov algorithm; would be better to poll the params from the algorithm, however this is not straightforward to do
        prototype.addSetting("Covariance update", TypeId_OnlineCovariance_UpdateMethod, "Chunk average");
        prototype.addSetting("Trace normalization", OV_TypeId_Boolean, "false");
        prototype.addSetting("Shrinkage coefficient", OV_TypeId_Float, "0.0");
        prototype.addSetting("Tikhonov coefficient", OV_TypeId_Float, "0.0");

        prototype.addOutput("Train-completed Flag", OV_TypeId_Stimulations);

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_RegularizedCSPTrainerDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
