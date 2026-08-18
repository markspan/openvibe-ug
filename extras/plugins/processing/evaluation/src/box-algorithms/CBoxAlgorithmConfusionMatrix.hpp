///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmConfusionMatrix.hpp
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

#include <iomanip>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {
class CBoxAlgorithmConfusionMatrix final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ConfusionMatrix)

protected:
    Toolkit::TStimulationDecoder<CBoxAlgorithmConfusionMatrix> m_targetStimDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmConfusionMatrix> m_classifierStimDecoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmConfusionMatrix> m_encoder;

    Kernel::IAlgorithmProxy* m_algorithm = nullptr;

    uint64_t m_currentProcessingTimeLimit = 0;
};

class CBoxAlgorithmConfusionMatrixListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingAdded(Kernel::IBox& box, const size_t index) override
    {
        std::stringstream value;
        value << "OVTK_StimulationId_Label_" << std::setfill('0') << std::setw(2) << index - 2;
        box.setSettingName(index, ("Class " + std::to_string(index - 1)).c_str());
        box.setSettingType(index, OV_TypeId_Stimulation);
        box.setSettingValue(index, value.str().c_str());
        return true;
    }

    bool onSettingRemoved(Kernel::IBox& box, const size_t /*index*/) override
    {
        const size_t nSetting = box.getSettingCount();
        const size_t nClass   = nSetting - FIRST_CLASS_SETTING_INDEX;

        for (size_t i = 0; i < nClass; ++i) { box.setSettingName(FIRST_CLASS_SETTING_INDEX + i, ("Class " + std::to_string(i + 1)).c_str()); }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmConfusionMatrixDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Confusion Matrix"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Make a confusion matrix out of classification results coming from one classifier."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Evaluation/Classification"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_ConfusionMatrix; }
    IPluginObject* create() override { return new CBoxAlgorithmConfusionMatrix; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Targets", OV_TypeId_Stimulations);
        prototype.addInput("Classification results", OV_TypeId_Stimulations);
        prototype.addOutput("Confusion Matrix", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Percentages", OV_TypeId_Boolean, "true");
        prototype.addSetting("Sums", OV_TypeId_Boolean, "false");

        prototype.addSetting("Class 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("Class 2", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");

        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);

        return true;
    }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmConfusionMatrixListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ConfusionMatrixDesc)
};
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
