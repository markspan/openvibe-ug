///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmVotingClassifier.hpp
/// \brief Classes for the Box Voting Classifier.
/// \author Yann Renard (Inria).
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

#include <vector>
#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CBoxAlgorithmVotingClassifier final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_VotingClassifier)

protected:
    size_t m_nRepetitions         = 0;
    size_t m_targetClassLabel     = 0;
    size_t m_nonTargetClassLabel  = 0;
    size_t m_rejectClassLabel     = 0;
    size_t m_resultClassLabelBase = 0;
    bool m_chooseOneIfExAequo     = false;

private:
    typedef struct SInput
    {
        Toolkit::TDecoder<CBoxAlgorithmVotingClassifier>* decoder = nullptr;
        Kernel::TParameterHandler<CStimulationSet*> op_stimSet;
        Kernel::TParameterHandler<CMatrix*> op_matrix;
        bool twoValueInput;
        std::vector<std::pair<double, uint64_t>> scores;
    } input_t;

    std::map<size_t, input_t> m_results;

    Toolkit::TStimulationEncoder<CBoxAlgorithmVotingClassifier> m_classificationChoiceEncoder;
    Kernel::TParameterHandler<const CStimulationSet*> ip_classificationChoiceStimSet;

    uint64_t m_lastTime = 0;
    bool m_matrixBased  = false;
};

class CBoxAlgorithmVotingClassifierListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    CBoxAlgorithmVotingClassifierListener() : m_inputTypeID(OV_TypeId_Stimulations) { }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier id = CIdentifier::undefined();
        box.getInputType(index, id);
        if (id == OV_TypeId_Stimulations || id == OV_TypeId_StreamedMatrix) {
            m_inputTypeID = id;
            for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputType(i, m_inputTypeID); }
        }
        else { box.setInputType(index, m_inputTypeID); }
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputType(i, m_inputTypeID);
            box.setInputName(i, ("Classification result " + std::to_string(i)).c_str());
        }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())

protected:
    CIdentifier m_inputTypeID = CIdentifier::undefined();
};

class CBoxAlgorithmVotingClassifierDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Voting Classifier"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Majority voting classifier. Returns the chosen class."; }

    CString getDetailedDescription() const override
    {
        return "Each classifier used as input is assumed to have its own two-class output stream. Mainly designed for P300 scenario use.";
    }

    CString getCategory() const override { return "Classification"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_VotingClassifier; }
    IPluginObject* create() override { return new CBoxAlgorithmVotingClassifier; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Classification result 1", OV_TypeId_Stimulations);
        prototype.addInput("Classification result 2", OV_TypeId_Stimulations);
        prototype.addOutput("Classification choice", OV_TypeId_Stimulations);
        prototype.addSetting("Number of repetitions", OV_TypeId_Integer, "12");
        prototype.addSetting("Target class label", OV_TypeId_Stimulation, "OVTK_StimulationId_Target");
        prototype.addSetting("Non target class label", OV_TypeId_Stimulation, "OVTK_StimulationId_NonTarget");
        prototype.addSetting("Reject class label", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Result class label base", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("Choose one if ex-aequo", OV_TypeId_Boolean, "false");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmVotingClassifierListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_VotingClassifierDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
