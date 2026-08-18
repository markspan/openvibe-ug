///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClassifierTrainer.hpp
/// \brief Classes for the Box Classifier trainer.
/// \author Yann Renard (Inria) / Guillaume Serrière (Inria).
/// \version 2.0.
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

#include "CBoxAlgorithmCommonClassifierListener.inl"

#include <map>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

const char* const TRAIN_TRIGGER_SETTING_NAME       = "Train trigger";
const char* const FILENAME_SETTING_NAME            = "Filename to save configuration to";
const char* const MULTICLASS_STRATEGY_SETTING_NAME = "Multiclass strategy to apply";
const char* const ALGORITHM_SETTING_NAME           = "Algorithm to use";
const char* const FOLD_SETTING_NAME                = "Number of partitions for k-fold cross-validation test";
const char* const BALANCE_SETTING_NAME             = "Balance classes";
const char* const RANDOMIZE_SETTING_NAME           = "Randomize k-fold cross-validation test data";

class CBoxAlgorithmClassifierTrainer final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ClassifierTrainer)

protected:
    typedef struct SSample
    {
        CMatrix* sampleMatrix;
        uint64_t startTime;
        uint64_t endTime;
        size_t inputIdx;
    } sample_t;

    bool train(const std::vector<sample_t>& dataset, const std::vector<size_t>& permutation, size_t startIdx, size_t stopIdx);
    double getAccuracy(const std::vector<sample_t>& dataset, const std::vector<size_t>& permutation, size_t startIdx, size_t stopIdx, CMatrix& confusionMatrix);
    bool printConfusionMatrix(const CMatrix& oMatrix);
    bool balanceDataset();

private:
    bool saveConfig();

protected:
    std::map<size_t, size_t> m_nFeatures;

    Kernel::IAlgorithmProxy* m_classifier = nullptr;
    uint64_t m_trainStimulation           = 0;
    size_t m_nPartition                   = 0;

    Toolkit::TStimulationDecoder<CBoxAlgorithmClassifierTrainer> m_stimDecoder;
    std::vector<Toolkit::TFeatureVectorDecoder<CBoxAlgorithmClassifierTrainer>*> m_sampleDecoder;

    Toolkit::TStimulationEncoder<CBoxAlgorithmClassifierTrainer> m_encoder;

    std::map<CString, CString>* m_parameter = nullptr;

    std::vector<sample_t> m_datasets;
    std::vector<sample_t> m_balancedDatasets;

};

class CBoxAlgorithmClassifierTrainerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Classifier trainer"; }
    CString getAuthorName() const override { return "Yann Renard, Guillaume Serriere"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Generic classifier trainer, relying on several box algorithms"; }
    CString getDetailedDescription() const override { return "Performs classifier training with cross-validation -based error estimation"; }
    CString getCategory() const override { return "Classification"; }
    CString getVersion() const override { return "2.0"; }

    CIdentifier getCreatedClass() const override { return Box_ClassifierTrainer; }
    IPluginObject* create() override { return new CBoxAlgorithmClassifierTrainer; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Features for class 1", OV_TypeId_FeatureVector);
        prototype.addInput("Features for class 2", OV_TypeId_FeatureVector);

        prototype.addOutput("Train-completed Flag", OV_TypeId_Stimulations);

        prototype.addSetting(TRAIN_TRIGGER_SETTING_NAME, OV_TypeId_Stimulation, "OVTK_StimulationId_Train");
        prototype.addSetting(FILENAME_SETTING_NAME, OV_TypeId_Filename, "${Path_UserData}/my-classifier.xml");

        prototype.addSetting(MULTICLASS_STRATEGY_SETTING_NAME, OVTK_TypeId_ClassificationStrategy, "Native");
        //Pairing startegy argument
        //Class label

        prototype.addSetting(ALGORITHM_SETTING_NAME, OVTK_TypeId_ClassificationAlgorithm, "Linear Discrimimant Analysis (LDA)");
        //Argument of algorithm

        prototype.addSetting(FOLD_SETTING_NAME, OV_TypeId_Integer, "10");
        prototype.addSetting(RANDOMIZE_SETTING_NAME, OV_TypeId_Boolean, "false");
        prototype.addSetting(BALANCE_SETTING_NAME, OV_TypeId_Boolean, "false");

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        //              prototype.addFlag(Kernel::BoxFlag_ManualUpdate);
        return true;
    }

    IBoxListener* createBoxListener() const override
    {
        const size_t nCommonSetting = 7;
        return new CBoxAlgorithmCommonClassifierListener(nCommonSetting);
    }

    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ClassifierTrainerDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
