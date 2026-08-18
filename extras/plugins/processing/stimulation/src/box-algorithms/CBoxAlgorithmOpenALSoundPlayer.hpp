///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmOpenALSoundPlayer.hpp
/// \author Laurent Bonnet (Inria).
/// \version 1.1.
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

#if defined TARGET_HAS_ThirdPartyOpenAL

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <AL/alut.h>
#include <vorbis/vorbisfile.h>
#include <iostream>
#include <vector>

namespace TCPTagging {
class IStimulusSender; // fwd declare
}

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
class CBoxAlgorithmOpenALSoundPlayer final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    struct SOggVorbisStream
    {
        OggVorbis_File Stream;
        FILE* File;
        ALenum Format;
        ALsizei SampleRate;
    };

    enum ESupportedFileFormat { Wav = 0, Ogg, Unsupported };

    CBoxAlgorithmOpenALSoundPlayer() { }

    void release() override { delete this; }

    uint64_t getClockFrequency() override { return (128LL << 32); }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    bool process() override;

    bool OpenSoundFile();
    bool PlaySound();
    bool StopSound(bool forwardStim);

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_OpenALSoundPlayer)

protected:
    Toolkit::TStimulationDecoder<CBoxAlgorithmOpenALSoundPlayer> m_decoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmOpenALSoundPlayer> m_encoder;

    uint64_t m_lastOutputChunkDate = 0;
    bool m_startOfSoundSent        = false;
    bool m_endOfSoundSent          = false;
    bool m_loop                    = false;
    uint64_t m_playTrigger         = 0;
    uint64_t m_stopTrigger         = 0;
    CString m_filename;

    std::vector<ALuint> m_sources;
    ESupportedFileFormat m_fileFormat = Unsupported;

    //The handles
    ALuint m_soundBufferHandle;
    ALuint m_sourceHandle;
    //OGG
    SOggVorbisStream m_oggVorbisStream;
    std::vector<char> m_rawOggBufferFromFile;

    // For sending stimulations to the TCP Tagging
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;
};

class CBoxAlgorithmOpenALSoundPlayerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Sound Player"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Play/Stop a sound, with or without loop."; }
    CString getDetailedDescription() const override { return "Available format : WAV / OGG. Play and stop with input stimulations. Box based on OpenAL."; }
    CString getCategory() const override { return "Stimulation"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Box_OpenALSoundPlayer; }
    IPluginObject* create() override { return new CBoxAlgorithmOpenALSoundPlayer; }
    CString getStockItemName() const override { return "gtk-media-play"; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input triggers", OV_TypeId_Stimulations);
        prototype.addOutput("Resync triggers (deprecated)", OV_TypeId_Stimulations);
        prototype.addSetting("PLAY trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_00");
        prototype.addSetting("STOP trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("File to play", OV_TypeId_Filename, "${Path_Data}/plugins/stimulation/ov_beep.wav");
        prototype.addSetting("Loop", OV_TypeId_Boolean, "false");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_OpenALSoundPlayerDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE

#endif //TARGET_HAS_ThirdPartyOpenAL
