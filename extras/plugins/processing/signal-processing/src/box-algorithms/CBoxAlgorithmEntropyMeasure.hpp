///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmEntropyMeasure.hpp
/// \brief Classes of the Box Entropy Measures.
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Mon Jan 29 10:00:00 2024.
/// \Copyright (C) 2024 Inria
/// 
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
/// 
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "../defines.hpp"
#include <toolkit/ovtk_all.h>

#include "../algorithms/entropy/SampleEntropyCalculator.hpp"


// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OV_AttributeId_Box_FlagIsUnstable                   OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


/// <summary> The class CBoxAlgorithmEntropyMeasure describes the box Entropy Measure. </summary>
class CBoxAlgorithmEntropyMeasure final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;


    // As we do with any class in openvibe, we use the macro below to associate this box to an unique identifier. 
    // The inheritance information is also made available, as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_EntropyMeasure)

protected:
    // Input decoder:
    Toolkit::TSignalDecoder<CBoxAlgorithmEntropyMeasure> m_inputDecoder;
    // Output decoder:
    std::vector<Toolkit::TSignalEncoder<CBoxAlgorithmEntropyMeasure>> m_outputEncoders;
    std::vector<CMatrix*> m_outputMatrices;

private:
    double m_samplingRate = 0.0;
    
    
    // box parameters
    double m_timeWindow = 0.0;
    double m_timeInterval = 0.0;

    // m is the length of the pattern vectors
    int m_m = 0;

    // r is the range below which two vectors are considered similar
    double m_r = 0;

    // m_scale is the scale wanted for entropy calculation. 1 for normal entropy ; >1 for multiscale entropy
    int m_scale = 1;

    bool m_isMultivariate = false;

    EntropyCalculator* m_entropyCalculator;
};

/// <summary> The class CBoxAlgorithmEntropyMeasureListener allows to change the amount of outputs of the entropy measure box, depending on the scale parameter chosen by the user. </summary>
class CBoxAlgorithmEntropyMeasureListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 4) {
            CString scaleStr;
            box.getSettingValue(index, scaleStr);

            int wantedScale = atoi(scaleStr);
            int currentScale = box.getOutputCount();

            if(wantedScale < 1) wantedScale = 1;
            if(wantedScale > 20) wantedScale = 20;

            if (wantedScale > currentScale) {
                for (int i = currentScale; i < wantedScale; ++i) {
                    char name[50];
                    std::sprintf(name, "Entropy scale %d", i+1);

                    box.addOutput(name, OV_TypeId_Signal);
                }
            // If wantedScale is less than currentScale, remove outputs
            } else if (wantedScale < currentScale) {
                for (int i = currentScale; i > wantedScale && i > 1; --i) {
                    box.removeOutput(box.getOutputCount() - 1);
                }
            }
        }

        return true;
    }
private:
    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Entropy Measure. </summary>
class CBoxAlgorithmEntropyMeasureDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return CString("Entropy Measure"); }
    CString getAuthorName() const override { return CString("Axel Bouneau"); }
    CString getAuthorCompanyName() const override { return CString("Inria"); }
    CString getShortDescription() const override { return CString("Computes the Entropy of an EEG signal."); }
    CString getDetailedDescription() const override { return CString("Computes the sample entropy of the input signal. Can be set to compute multivariate and/or multiscale sample entropy."); }
    CString getCategory() const override { return CString("Measures"); }
    CString getVersion() const override { return CString("1.0"); }
    CString getStockItemName() const override { return CString("gtk-remove"); }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_EntropyMeasure; }
    IPluginObject* create() override { return new CBoxAlgorithmEntropyMeasure; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmEntropyMeasureListener; }


    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Signal", OV_TypeId_Signal);

        prototype.addOutput("Entropy", OV_TypeId_Signal);

        prototype.addSetting("Time window (s)", OV_TypeId_Float, "10.0");
        prototype.addSetting("Time interval (s)", OV_TypeId_Float, "0.0");
        prototype.addSetting("m (pattern length)", OV_TypeId_Integer, "2");
        prototype.addSetting("r (tolerance)", OV_TypeId_Float, "0.2");

        prototype.addSetting("Scale", OV_TypeId_Integer, "1");
        prototype.addSetting("Multivariate", OV_TypeId_Boolean, "false");
        

        return true;
    }
    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EntropyMeasureDesc)
};


}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE