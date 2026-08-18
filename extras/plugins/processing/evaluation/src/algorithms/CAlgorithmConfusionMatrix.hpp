///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmConfusionMatrix.hpp
/// \author Laurent Bonnet (INRIA/IRISA)
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
#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {
class CAlgorithmConfusionMatrix final : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, Algorithm_ConfusionMatrix)

protected:
    Kernel::TParameterHandler<bool> ip_usePercentages;
    Kernel::TParameterHandler<bool> ip_useSums;

    // input TARGET
    Kernel::TParameterHandler<CStimulationSet*> ip_targetStimSet;
    // deduced timeline:
    std::map<uint64_t, uint64_t> m_targetsTimeLines;

    // input CLASSIFIER
    Kernel::TParameterHandler<CStimulationSet*> ip_classifierStimSet;

    //CONFUSION MATRIX computing
    Kernel::TParameterHandler<CStimulationSet*> ip_classesCodes;
    Kernel::TParameterHandler<CMatrix*> op_confusionMatrix;

    CMatrix m_confusionMatrix; // the values, not percentage
    std::map<uint64_t, size_t> m_nClassificationAttemptPerClass;

private:
    bool isClass(const uint64_t id) const;
    size_t getClassIndex(const uint64_t id) const;
};

class CAlgorithmConfusionMatrixDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Confusion Matrix Algorithm"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Make a confusion matrix out of classification results coming from one classifier."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Classification"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Algorithm_ConfusionMatrix; }
    IPluginObject* create() override { return new CAlgorithmConfusionMatrix; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet, "Targets", Kernel::ParameterType_StimulationSet);
        prototype.addInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet, "Classification results", Kernel::ParameterType_StimulationSet);
        prototype.addInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes, "Class codes", Kernel::ParameterType_StimulationSet);
        prototype.addInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage, "Percentage", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums, "Sums", Kernel::ParameterType_Boolean);

        prototype.addOutputParameter(Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix, "Confusion matrix", Kernel::ParameterType_Matrix);

        prototype.addInputTrigger(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget, "Reset Target");
        prototype.addInputTrigger(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier, "Reset Classifier");
        prototype.addInputTrigger(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget, "Feed Target");
        prototype.addInputTrigger(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier, "Feed Classifier");

        prototype.addOutputTrigger(Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed, "Confusion computing performed");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, Algorithm_ConfusionMatrixDesc)
};
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
