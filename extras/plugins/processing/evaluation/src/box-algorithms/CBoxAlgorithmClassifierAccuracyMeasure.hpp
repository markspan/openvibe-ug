///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClassifierAccuracyMeasure.hpp
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

#include "CClassifierAccuracyMeasureView.hpp"

#include <map>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {
class CBoxAlgorithmClassifierAccuracyMeasure final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ClassifierAccuracyMeasure)

protected:
    ClassifierAccuracyMeasureView* getView();

    //codecs
    // for the TARGET
    Toolkit::TStimulationDecoder<CBoxAlgorithmClassifierAccuracyMeasure> m_targetStimDecoder;
    // For the CLASSIFIERS
    std::vector<Toolkit::TStimulationDecoder<CBoxAlgorithmClassifierAccuracyMeasure>*> m_classifierStimDecoders;

    // deduced timeline:
    std::map<uint64_t, uint64_t> m_targetsTimeLines;
    uint64_t m_currentProcessingTimeLimit = 0;

    //display section
    friend class CBoxAlgorithmClassifierAccuracyMeasureDesc;
    ClassifierAccuracyMeasureView *m_accuracyView = nullptr;
};

class CBoxAlgorithmClassifierAccuracyMeasureListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputNameChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) { box.setInputName(0, "Targets"); } // forced
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_Stimulations); // all inputs must be stimulations
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmClassifierAccuracyMeasureDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override {
        for(auto * view : this->m_accuracyViews) {
            if(view) {
                delete view;
            }
        }
        this->m_accuracyViews.clear();

     }

    CString getName() const override { return "Classifier Accuracy Measure"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Displays real-time classifier accuracies as vertical progress bars"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Evaluation/Classification"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return Box_ClassifierAccuracyMeasure; }
    IPluginObject* create() override { return new CBoxAlgorithmClassifierAccuracyMeasure; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmClassifierAccuracyMeasureListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Targets", OV_TypeId_Stimulations);
        prototype.addInput("Classifier 1", OV_TypeId_Stimulations);

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addInputSupport(OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ClassifierAccuracyMeasureDesc)

protected:
    //Display section
    std::vector<ClassifierAccuracyMeasureView*> m_accuracyViews;
};

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE