///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmOpenALSoundPlayer.cpp
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

#if defined TARGET_HAS_ThirdPartyOpenAL

#include "CBoxAlgorithmOpenALSoundPlayer.hpp"
#include <tcptagging/IStimulusSender.h>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {


#define BUFFER_SIZE 32768
#define UNIQUE_SOURCE 1

bool CBoxAlgorithmOpenALSoundPlayer::initialize()
{
    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);

    m_playTrigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_stopTrigger = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_filename    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_loop        = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    m_lastOutputChunkDate = -1;
    m_startOfSoundSent    = false;
    m_endOfSoundSent      = false;

    if (alutInit(nullptr, nullptr) != AL_TRUE) {
        if (alutGetError() == ALUT_ERROR_INVALID_OPERATION) { this->getLogManager() << Kernel::LogLevel_Trace << "ALUT already initialized.\n"; }
        else {
            this->getLogManager() << Kernel::LogLevel_Error << "ALUT initialization returned a bad status.\n";
            this->getLogManager() << Kernel::LogLevel_Error << "ALUT ERROR:\n" << alutGetErrorString(alutGetError()) << "\n";
            return false;
        }
    }

    m_fileFormat = Unsupported;

    const std::string file(m_filename.toASCIIString());
    if (file.find(".wav") != std::string::npos) { m_fileFormat = Wav; }
    if (file.find(".ogg") != std::string::npos) { m_fileFormat = Ogg; }

    m_stimulusSender = TCPTagging::CreateStimulusSender();

    if (!m_stimulusSender->connect("localhost", "15361")) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
    }

    return OpenSoundFile();
}

bool CBoxAlgorithmOpenALSoundPlayer::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();

    const bool status = StopSound(false);

#if UNIQUE_SOURCE
    alDeleteSources(1, &m_sourceHandle);
#endif
    alDeleteBuffers(1, &m_soundBufferHandle);

    if (alutExit() != AL_TRUE) {
        if (alutGetError() == ALUT_ERROR_INVALID_OPERATION) { this->getLogManager() << Kernel::LogLevel_Trace << "ALUT already exited.\n"; }
        else {
            this->getLogManager() << Kernel::LogLevel_Error << "ALUT uninitialization returned a bad status.\n";
            this->getLogManager() << Kernel::LogLevel_Error << "ALUT ERROR:\n" << alutGetErrorString(alutGetError()) << "\n";
            return false;
        }
    }

    if (m_stimulusSender) {
        delete m_stimulusSender;
        m_stimulusSender = nullptr;
    }

    return status;
}

bool CBoxAlgorithmOpenALSoundPlayer::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmOpenALSoundPlayer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmOpenALSoundPlayer::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    if (m_lastOutputChunkDate == uint64_t(-1)) {
        // Send header on initialize
        m_encoder.encodeHeader();
        boxContext.markOutputAsReadyToSend(0, 0, 0);
        m_lastOutputChunkDate = 0;
    }

    // Look for command stimulations
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_decoder.decode(i);

        if (m_decoder.isHeaderReceived()) { }   // NOP
        if (m_decoder.isBufferReceived()) {
            const CStimulationSet* stimSet = m_decoder.getOutputStimulationSet();

            for (size_t j = 0; j < stimSet->size(); ++j) {
                const uint64_t stim = stimSet->getId(j);
                if (stim == m_playTrigger) {
                    PlaySound();
                    m_endOfSoundSent   = false;
                    m_startOfSoundSent = false;
                }
                else if (stim == m_stopTrigger) { StopSound(true); }
                else {
                    // Immediate passthrough
                    m_stimulusSender->sendStimulation(stim);
                }
            }
        }
        if (m_decoder.isEndReceived()) {
            // @fixme potentially bad behavior: the box may send chunks after sending this end
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, m_lastOutputChunkDate, this->getPlayerContext().getCurrentTime());
            m_lastOutputChunkDate = this->getPlayerContext().getCurrentTime();
        }
    }

    // n.b. TCP Tagging should be used instead of this socket output. This code is kept for backwards compatibility.
    const CStimulationSet* oStimSet = m_encoder.getInputStimulationSet();
    oStimSet->clear();

    ALint status;
    alGetSourcei(m_sourceHandle, AL_SOURCE_STATE, &status);
    // CASE : the sound has stopped, and we need to send the stimulation
    if (status == AL_STOPPED && !m_endOfSoundSent) {
        oStimSet->push_back(m_stopTrigger, this->getPlayerContext().getCurrentTime(), 0);
        m_endOfSoundSent = true;
    }
    // CASE : the sound has started playing, and we need to send the stimulation
    if (status == AL_PLAYING && !m_startOfSoundSent) {
        oStimSet->push_back(m_playTrigger, this->getPlayerContext().getCurrentTime(), 0);
        m_startOfSoundSent = true;
    }

    m_encoder.encodeBuffer();
    boxContext.markOutputAsReadyToSend(0, m_lastOutputChunkDate, this->getPlayerContext().getCurrentTime());

    m_lastOutputChunkDate = this->getPlayerContext().getCurrentTime();
    return true;
}

bool CBoxAlgorithmOpenALSoundPlayer::OpenSoundFile()
{
    switch (m_fileFormat) {
        case Wav:
        {
            this->getLogManager() << Kernel::LogLevel_Trace << "Buffering WAV file (this step may take some times for long files).\n";
            m_soundBufferHandle = alutCreateBufferFromFile(m_filename);
            this->getLogManager() << Kernel::LogLevel_Trace << "WAV file buffered.\n";
            if (m_soundBufferHandle == AL_NONE) {
                this->getLogManager() << Kernel::LogLevel_Error << "ALUT can't create buffer from file " << m_filename << "\n";
                this->getLogManager() << Kernel::LogLevel_Error << "ALUT ERROR:\n" << alutGetErrorString(alutGetError()) << "\n";
                return false;
            }
            break;
        }
        case Ogg:
        {
            // On windows using fopen+ov_open can lead to failure, as stated in the vorbis official documentation:
            //http://xiph.org/vorbis/doc/vorbisfile/ov_open.html
            // using ov_fopen instead.
            //m_oggVorbisStream.File = fopen((const char *)m_filename, "rb");
            //if (m_oggVorbisStream.File == nullptr)
            //{
            // this->getLogManager() << Kernel::LogLevel_Error << "Can't open file "<<m_filename<<": IO error\n.";
            // return false;
            //}

#if defined TARGET_OS_Windows
            if (ov_fopen(const_cast<char*>(m_filename.toASCIIString()), &m_oggVorbisStream.Stream) < 0)
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            if((m_oggVorbisStream.File = fopen((const char *)m_filename, "rb")) == nullptr)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Can't open file "<<m_filename<<": IO error\n.";
                return false;
            }
            if(ov_open(m_oggVorbisStream.File, &(m_oggVorbisStream.Stream), nullptr, 0) < 0)
#else
#error "Please port this code"
#endif
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Can't open file " << m_filename << ": OGG VORBIS stream error\n";
                return false;
            }

            const vorbis_info* infos     = ov_info(&m_oggVorbisStream.Stream, -1);
            m_oggVorbisStream.Format     = infos->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
            m_oggVorbisStream.SampleRate = infos->rate;

            //Now we fill the raw buffer (good for small piece of sound... use buffering for big files)
            this->getLogManager() << Kernel::LogLevel_Trace << "Buffering OGG file (this step may take some times for long files).\n";
            int bytesRead;
            int bitStream;
            char buffer[BUFFER_SIZE];
            do {
                // Read up to a buffer's worth of decoded sound data
                bytesRead = ov_read(&m_oggVorbisStream.Stream, buffer, BUFFER_SIZE, 0, 2, 1, &bitStream);
                // Append to end of buffer
                m_rawOggBufferFromFile.insert(m_rawOggBufferFromFile.end(), buffer, buffer + bytesRead);
            } while (bytesRead > 0);
            this->getLogManager() << Kernel::LogLevel_Trace << "OGG file buffered.\n";

            //we have decoded all the file. we drop the decoder (file is closed for us).
            ov_clear(&m_oggVorbisStream.Stream);

            //create empty buffer
            alGenBuffers(1, &m_soundBufferHandle);
            //fill it with raw data
            alBufferData(m_soundBufferHandle, m_oggVorbisStream.Format, &m_rawOggBufferFromFile[0], ALsizei(m_rawOggBufferFromFile.size()),
                         m_oggVorbisStream.SampleRate);

            break;
        }
        default:
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unsupported file format. Please use only WAV or OGG files.\n";
            return false;
        }
    }

#if UNIQUE_SOURCE
    alGenSources(1, &m_sourceHandle);
    alSourcei(m_sourceHandle, AL_BUFFER, ALint(m_soundBufferHandle));
    alSourcei(m_sourceHandle, AL_LOOPING, (m_loop ? AL_TRUE : AL_FALSE));
#endif
    return true;
}

bool CBoxAlgorithmOpenALSoundPlayer::PlaySound()
{
    switch (m_fileFormat) {
        case Wav:
        case Ogg:
        {
#if UNIQUE_SOURCE
            ALint status;
            alGetSourcei(m_sourceHandle, AL_SOURCE_STATE, &status);
            if (status == AL_PLAYING) { alSourceStop(m_sourceHandle); } // we start back again
            alSourcePlay(m_sourceHandle);
#else
            ALuint src;
            alGenSources(1, &src);
            m_sources.push_back(src);
            alSourcei (src, AL_BUFFER, m_soundBufferHandle);
            alSourcei (src, AL_LOOPING, (m_loop?AL_TRUE:AL_FALSE));
            alSourcePlay(src);
#endif
            break;
        }
        default:
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unsupported file format. Please use only WAV or OGG files.\n";
            return false;
        }
    }

    m_stimulusSender->sendStimulation(m_playTrigger);
    return true;
}

bool CBoxAlgorithmOpenALSoundPlayer::StopSound(const bool forwardStim)
{
    switch (m_fileFormat) {
        case Wav:
        case Ogg:
        {
#if UNIQUE_SOURCE
            alSourceStop(m_sourceHandle);
#else
            for (size_t i = 0;i<m_sources.size();i++)
            {
                //stop all sources
                alSourceStop(m_sources[i]);
                alDeleteSources(1, &m_sources[i]);
            }
            m_sources.clear();
#endif
            break;
        }
        default:
        {
            this->getLogManager() << Kernel::LogLevel_Error << "Unsupported file format. Please use only WAV or OGG files.\n";
            return false;
        }
    }

    if (forwardStim) { m_stimulusSender->sendStimulation(m_stopTrigger); }
    return true;
}


}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE

#endif //TARGET_HAS_ThirdPartyOpenAL
