///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClassifierProcessor.hpp
/// \brief Classes for the Box Classifier processor.
/// \author Yann Renard (Inria) / Guillaume Serrière (Inria).
/// \version 2.1.
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
namespace Classification {
class CBoxAlgorithmClassifierProcessor final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ClassifierProcessor)

protected:
    bool loadClassifier(const char* filename);

private:
    Toolkit::TFeatureVectorDecoder<CBoxAlgorithmClassifierProcessor> m_sampleDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmClassifierProcessor> m_stimDecoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmClassifierProcessor> m_labelsEncoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmClassifierProcessor> m_hyperplanesEncoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmClassifierProcessor> m_probabilitiesEncoder;

    Kernel::IAlgorithmProxy* m_classifier = nullptr;

    std::map<double, uint64_t> m_stimulations;
};

class CBoxAlgorithmClassifierProcessorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Classifier processor"; }
    CString getAuthorName() const override { return "Yann Renard, Guillaume Serriere"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Generic classification, relying on several box algorithms"; }
    CString getDetailedDescription() const override { return "Classifies incoming feature vectors using a previously learned classifier."; }
    CString getCategory() const override { return "Classification"; }
    CString getVersion() const override { return "2.1"; }

    CIdentifier getCreatedClass() const override { return Box_ClassifierProcessor; }
    IPluginObject* create() override { return new CBoxAlgorithmClassifierProcessor; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Features", OV_TypeId_FeatureVector);
        prototype.addInput("Commands", OV_TypeId_Stimulations);
        prototype.addOutput("Labels", OV_TypeId_Stimulations);
        prototype.addOutput("Hyperplane distance", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Probability values", OV_TypeId_StreamedMatrix);

        //We load everything in the save filed
        prototype.addSetting("Filename to load configuration from", OV_TypeId_Filename, "");
        return true;
    }

    // virtual IBoxListener* createBoxListener() const { return new CBoxAlgorithmCommonClassifierListener(5); }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ClassifierProcessorDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
