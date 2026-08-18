///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmZeroCrossingDetector.hpp
/// \brief Classes for the Box Zero-Crossing Detector.
/// \author Quentin Barthelemy (Mensia Technologies).
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

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmZeroCrossingDetector final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ZeroCrossingDetector)

protected:
    Toolkit::TGenericDecoder<CBoxAlgorithmZeroCrossingDetector> m_decoder;
    Toolkit::TGenericEncoder<CBoxAlgorithmZeroCrossingDetector> m_encoder0;
    Toolkit::TStimulationEncoder<CBoxAlgorithmZeroCrossingDetector> m_encoder1;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmZeroCrossingDetector> m_encoder2;

    std::vector<double> m_signals;
    std::vector<int> m_states;
    double m_hysteresis = 0;
    uint64_t m_nChunk   = 0;

    size_t m_sampling    = 0;
    double m_windowTimeD = 0;
    size_t m_windowTime  = 0;
    std::vector<std::vector<size_t>> m_chunks;
    std::vector<std::vector<size_t>> m_samples;

    uint64_t m_stimId1 = 0;
    uint64_t m_stimId2 = 0;
};

class CBoxAlgorithmZeroCrossingDetectorListener final : public Toolkit::TBoxListener<IBoxListener>
{
    static bool onConnectorTypeChanged(Kernel::IBox& box, const size_t index, const CIdentifier& typeID, const bool outputChanged)
    {
        if (index == 0) {
            if (typeID == OV_TypeId_Signal) {
                box.setInputType(0, OV_TypeId_Signal);
                box.setOutputType(0, OV_TypeId_Signal);
            }
            else if (typeID == OV_TypeId_StreamedMatrix) {
                box.setInputType(0, OV_TypeId_StreamedMatrix);
                box.setOutputType(0, OV_TypeId_StreamedMatrix);
            }
            else {
                // Invalid i/o type identifier
                CIdentifier originalTypeID = CIdentifier::undefined();
                if (outputChanged) {
                    // Restores output
                    box.getInputType(0, originalTypeID);
                    box.setOutputType(0, originalTypeID);
                }
                else {
                    // Restores input
                    box.getOutputType(0, originalTypeID);
                    box.setInputType(0, originalTypeID);
                }
            }
        }
        if (index == 1) { box.setOutputType(1, OV_TypeId_Stimulations); }
        if (index == 2) { box.setOutputType(2, OV_TypeId_StreamedMatrix); }

        return true;
    }

public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        return onConnectorTypeChanged(box, index, typeID, false);
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        return onConnectorTypeChanged(box, index, typeID, true);
    }
    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmZeroCrossingDetectorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Zero-Crossing Detector"; }
    CString getAuthorName() const override { return "Quentin Barthelemy"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Detects zero-crossings of the signal"; }

    CString getDetailedDescription() const override
    {
        return
                "Detects zero-crossings of the signal for each channel, with 1 for positive zero-crossings (negative-to-positive), -1 for negatives ones (positive-to-negative), 0 otherwise. For all channels, stimulations mark positive and negatives zero-crossings. For each channel, the rythm is computed in events per min.";
    }

    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_ZeroCrossingDetector; }
    IPluginObject* create() override { return new CBoxAlgorithmZeroCrossingDetector; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmZeroCrossingDetectorListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Zero-crossing signal", OV_TypeId_Signal);
        prototype.addOutput("Zero-crossing stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Events rythm (per min)", OV_TypeId_StreamedMatrix);
        prototype.addSetting("Hysteresis threshold", OV_TypeId_Float, "0.01");
        prototype.addSetting("Rythm estimation window (in sec)", OV_TypeId_Float, "10");
        prototype.addSetting("Negative-to-positive stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_ThresholdPassed_Positive");
        prototype.addSetting("Positive-to-negative stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_ThresholdPassed_Negative");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ZeroCrossingDetectorDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
