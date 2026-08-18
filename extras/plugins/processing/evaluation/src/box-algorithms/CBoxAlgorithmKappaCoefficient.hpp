///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmKappaCoefficient.hpp
/// \author Serrière Guillaume (Inria)
/// \version 1.0.
/// \date 05/05/2015.
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

//You may have to change this path to match your folder organisation
#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <sstream>

//#include <visualization-toolkit/ovviz_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {
/// <summary> The class CBoxAlgorithmKappaCoef describes the box Kappa coefficient. </summary>
class CBoxAlgorithmKappaCoef final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_KappaCoef)

protected:
    void updateKappaValue();

    Toolkit::TStimulationDecoder<CBoxAlgorithmKappaCoef> m_targetStimDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmKappaCoef> m_classifierStimDecoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmKappaCoef> m_encoder;

    Kernel::TParameterHandler<CMatrix*> op_confusionMatrix;

    Kernel::IAlgorithmProxy* m_algorithm = nullptr;

    size_t m_amountClass                  = 0;
    uint64_t m_currentProcessingTimeLimit = 0;
    double m_kappaCoef                    = 0;
};


// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
// Please uncomment below the callbacks you want to use.
class CBoxAlgorithmKappaCoefListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) {
            CString nClass;
            box.getSettingValue(index, nClass);

            if (nClass.length() == 0) { return true; }

            size_t nSetting;
            std::stringstream ss(nClass.toASCIIString());
            ss >> nSetting;

            //First of all we prevent for the value to goes under 1.
            if (nSetting < 1) {
                box.setSettingValue(index, "1");
                nSetting = 1;
            }
            size_t nCurrent = box.getSettingCount() - 1;
            //We have two choice 1/We need to add class, 2/We need to remove some
            if (nCurrent < nSetting) {
                while (nCurrent < nSetting) {
                    box.addSetting(("Stimulation of class " + std::to_string(nCurrent + 1)).c_str(), OVTK_TypeId_Stimulation, "");
                    ++nCurrent;
                }
            }
            else {
                while (nCurrent > nSetting) {
                    box.removeSetting(box.getSettingCount() - 1);
                    --nCurrent;
                }
            }
        }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Kappa coefficient. </summary>
class CBoxAlgorithmKappaCoefDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Kappa coefficient"; }
    CString getAuthorName() const override { return "Serrière Guillaume"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Compute the kappa coefficient for the classifier."; }
    CString getDetailedDescription() const override { return "The box computes kappa coefficient for a classifier."; }
    CString getCategory() const override { return "Evaluation/Classification"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-yes"; }

    CIdentifier getCreatedClass() const override { return Box_KappaCoef; }
    IPluginObject* create() override { return new CBoxAlgorithmKappaCoef; }


    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmKappaCoefListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Expected stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Found stimulations", OV_TypeId_Stimulations);

        prototype.addOutput("Confusion Matrix", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Number of classes", OV_TypeId_Integer, "2");
        prototype.addSetting("Stimulation of class 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("Stimulation of class 2", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_02");

        prototype.addFlag(Kernel::BoxFlag_CanModifySetting);
        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_KappaCoefDesc)
};
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE