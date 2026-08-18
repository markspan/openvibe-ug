///-------------------------------------------------------------------------------------------------
/// 
/// \file CTestCodecToolkit.hpp
/// \author Laurent Bonnet (Inria)
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

namespace OpenViBE {
namespace Plugins {
namespace Tests {
class CTestCodecToolkit final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_TestCodecToolkit)

protected:
    Toolkit::TStreamedMatrixDecoder<CTestCodecToolkit> m_matrixDecoder;
    Toolkit::TStreamedMatrixEncoder<CTestCodecToolkit> m_streamedMatrixEncoder;

    Toolkit::TChannelLocalisationDecoder<CTestCodecToolkit> m_channelLocalisationDecoder;
    Toolkit::TChannelLocalisationEncoder<CTestCodecToolkit> m_channelLocalisationEncoder;

    Toolkit::TFeatureVectorDecoder<CTestCodecToolkit> m_featureVectorDecoder;
    Toolkit::TFeatureVectorEncoder<CTestCodecToolkit> m_featureVectorEncoder;

    Toolkit::TSpectrumDecoder<CTestCodecToolkit> m_spectrumDecoder;
    Toolkit::TSpectrumEncoder<CTestCodecToolkit> m_spectrumEncoder;

    Toolkit::TSignalDecoder<CTestCodecToolkit> m_signalDecoder;
    Toolkit::TSignalEncoder<CTestCodecToolkit> m_signalEncoder;

    Toolkit::TStimulationDecoder<CTestCodecToolkit> m_stimDecoder;
    Toolkit::TStimulationEncoder<CTestCodecToolkit> m_stimEncoder;

    Toolkit::TExperimentInfoDecoder<CTestCodecToolkit> m_experimentInfoDecoder;
    Toolkit::TExperimentInfoEncoder<CTestCodecToolkit> m_experimentInfoEncoder;

    /* One decoder per input. This vector makes easy the decoding in one iteration over the inputs. */
    std::vector<Toolkit::TDecoder<CTestCodecToolkit>*> m_decoders;

    /* One encoder per output This vector makes easy the encoding in one iteration over the outputs. */
    std::vector<Toolkit::TEncoder<CTestCodecToolkit>*> m_encoders;
};

class CTestCodecToolkitDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Codec Toolkit testbox"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Sample box to test the codec toolkit. Identity (input = output)."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Tests/Algorithms"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_TestCodecToolkit; }
    IPluginObject* create() override { return new CTestCodecToolkit; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Streamed Matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Streamed Matrix", OV_TypeId_StreamedMatrix);

        prototype.addInput("Channel Localisation", OV_TypeId_ChannelLocalisation);
        prototype.addOutput("Channel Localisation", OV_TypeId_ChannelLocalisation);

        prototype.addInput("Feature Vector", OV_TypeId_FeatureVector);
        prototype.addOutput("Feature Vector", OV_TypeId_FeatureVector);

        prototype.addInput("Spectrum", OV_TypeId_Spectrum);
        prototype.addOutput("Spectrum", OV_TypeId_Spectrum);

        prototype.addInput("Signal", OV_TypeId_Signal);
        prototype.addOutput("Signal", OV_TypeId_Signal);

        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);

        prototype.addInput("XP info", OV_TypeId_ExperimentInfo);
        prototype.addOutput("XP info", OV_TypeId_ExperimentInfo);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_TestCodecToolkitDesc)
};
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
