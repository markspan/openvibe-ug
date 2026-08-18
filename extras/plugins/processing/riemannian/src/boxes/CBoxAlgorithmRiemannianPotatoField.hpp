///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRiemannianPotatoField.hpp
/// \brief Classes of the box riemannian potato field
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Thu Dec 05 10:00:00 2024.
/// \Copyright (C) 2024 Inria
///
/// This classes originate from the riemannian potato box developped in openvibe in 2012 by Andreev A., Barachant A., Congedo M., Ionescu,Gelu (Gipsa-Lab)
/// And implement the algorithms described in "The Riemannian Potato Field: a tool for online Signal Quality Index of EEG" by Quentin Barthelemy, Louis Mayaud, David Ojeda and Marco Congedo (2019)
/// The equations labelled with (n) correspond to the ones described in this paper
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

# pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "RiemannianPotato.hpp"


namespace OpenViBE {
namespace Plugins {
namespace Riemannian {

/// <summary>    The class CBoxAlgorithmRiemannianPotatoField describes the box Riemannian Potato Field. </summary>
class CBoxAlgorithmRiemannianPotatoField final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

protected:
    // Parameters
    uint64_t m_nbPotatoes;
    uint64_t m_artifactBeginningStim;
    uint64_t m_artifactEndStim;
    double m_adaptationSpeed;
    double m_zscoreThreshold;

    // Other atributes
    std::vector<RiemannianPotato> m_potatoes;
    size_t m_potatoIdx = 0;
    std::vector<double> m_zScores;
    bool m_currentlyInArtifact = false;

    std::string m_inputFile;
    std::string m_outputFile;


    // Signal inputs
    std::vector<Toolkit::TSignalDecoder<CBoxAlgorithmRiemannianPotatoField>> m_inputSignalsDecoders;

    // Stimulation output
    const CStimulationSet* m_stimSet = nullptr;
    Toolkit::TStimulationEncoder<CBoxAlgorithmRiemannianPotatoField> m_outputStimEncoder;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_RiemannianPotatoField)

private:
    /**
     * @brief Computes the combined z-score thanks to all the potatoes own z-scores
     * @return The combined z-score
     */
    double combineZScores();

};


/// <summary> Listener of the box Riemannian Potato Field. </summary>
class CBoxAlgorithmRiemannianPotatoFieldListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) {
            CString nbPotatoesStr;
            box.getSettingValue(index, nbPotatoesStr);

            int wantedAmount = atoi(nbPotatoesStr);
            int currentAmount = box.getInputCount();

            if(wantedAmount < 0) {
                box.setSettingValue(index, CString("0"));
                wantedAmount = 0;
            }
            if(wantedAmount > 10) {
                box.setSettingValue(index, CString("10"));
                wantedAmount = 10;
            }

            if (wantedAmount > currentAmount) {
                for (int i = currentAmount; i < wantedAmount; ++i) {
                    char name[50];
                    std::sprintf(name, "Filtered and epoched signal %d", i+1);

                    box.addInput(name, OV_TypeId_Signal);
                }
            // If wantedAmount is less than currentAmount, remove inputs
            } else if (wantedAmount < currentAmount) {
                for (int i = currentAmount; i > wantedAmount && i > 0; --i) {
                    box.removeInput(box.getInputCount() - 1);
                }
            }
        }

        return true;
    }
private:
    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Riemannian Potato Field. </summary>
class CBoxAlgorithmRiemannianPotatoFieldDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    virtual CString getName(void) const                { return CString("Riemannian Potato Field"); }
    virtual CString getAuthorName(void) const          { return CString("Axel Bouneau"); }
    virtual CString getAuthorCompanyName(void) const   { return CString("INRIA"); }
    virtual CString getShortDescription(void) const    { return CString(""); }
    virtual CString getDetailedDescription(void) const { return CString("This box allows to detect artifacts on a signal, based on the Riemannian Potato Field algorithm."); }
    virtual CString getCategory(void) const            { return CString("Riemannian Geometry"); }
    virtual CString getVersion(void) const             { return CString("1.0"); }
    virtual CString getStockItemName(void) const       { return CString("gtk-jump-to"); }

    CIdentifier getCreatedClass() const override { return Box_RiemannianPotatoField; }
    IPluginObject* create() override { return new CBoxAlgorithmRiemannianPotatoField; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmRiemannianPotatoFieldListener; }

    virtual bool getBoxPrototype(
        Kernel::IBoxProto& rBoxAlgorithmPrototype) const
    {
        //Input
        rBoxAlgorithmPrototype.addInput  ("Filtered and epoched signal 1",                 OV_TypeId_Signal     );

        //Output
        rBoxAlgorithmPrototype.addOutput ("Output stimulations channel",   OV_TypeId_Stimulations   ); //when artifact starts and when it ends

        //Parameters
        rBoxAlgorithmPrototype.addSetting("Number of potatoes", OV_TypeId_Integer, "1");
        rBoxAlgorithmPrototype.addSetting("Start artifact stimulation",    OV_TypeId_Stimulation, "OVTK_StimulationId_SegmentStart" );
        rBoxAlgorithmPrototype.addSetting("Stop artifact stimulation",     OV_TypeId_Stimulation, "OVTK_StimulationId_SegmentStop"  );

        rBoxAlgorithmPrototype.addSetting("Speed of adaptation:",               OV_TypeId_Float,    "0.5");
        rBoxAlgorithmPrototype.addSetting("Threshold parameter:",                OV_TypeId_Float,      "0.01");

        rBoxAlgorithmPrototype.addSetting("Import trained potato field", OV_TypeId_Filename, "");
        rBoxAlgorithmPrototype.addSetting("Save potato field", OV_TypeId_Filename, "");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_RiemannianPotatoFieldDesc)
};




    
} // namespace Riemannian 
} // namespace Plugins   
} // namespace OpenViBE 
