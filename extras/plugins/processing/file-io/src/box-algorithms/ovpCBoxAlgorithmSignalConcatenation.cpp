#include "ovpCBoxAlgorithmSignalConcatenation.h"

namespace OpenViBE {
namespace Plugins {
namespace FileIO {


bool CBoxAlgorithmSignalConcatenation::initialize()
{
    m_signalChunkBuffers.resize(this->getStaticBoxContext().getInputCount() >> 1);
    m_stimulationChunkBuffers.resize(this->getStaticBoxContext().getInputCount() >> 1);

    m_timeOut = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_timeOut = m_timeOut << 32;
    this->getLogManager() << Kernel::LogLevel_Info << "Timeout set to " << CTime(m_timeOut) << ".\n";
    for (uint32_t i = 0; i < this->getStaticBoxContext().getInputCount(); i += 2)
    {
        m_eofStimulations.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), (i >> 2) + 1));
        m_eofReached.push_back(false);
        m_fileEndTimes.push_back(0);
    }

    for (uint32_t i = 0; i < this->getStaticBoxContext().getInputCount(); i += 2)
    {
        auto* signalDecoder = new Toolkit::TSignalDecoder<CBoxAlgorithmSignalConcatenation>(*this, i);
        auto* stimDecoder   = new Toolkit::TStimulationDecoder<CBoxAlgorithmSignalConcatenation>(*this, i + 1);

        m_signalDecoders.push_back(signalDecoder);
        m_stimulationDecoders.push_back(stimDecoder);
        auto* stimSet = new CStimulationSet();
        m_stimulationSets.push_back(stimSet);
    }

    m_stimulationEncoder.initialize(*this, 1);
    m_stimulationEncoder.getInputStimulationSet().setReferenceTarget(m_stimulationDecoders[0]->getOutputStimulationSet());

    m_signalEncoder.initialize(*this, 0);

    m_triggerEncoder.initialize(*this, 2);
    m_triggerEncoder.getInputStimulationSet().setReferenceTarget(m_stimulationDecoders[0]->getOutputStimulationSet());

    m_headerReceivedCount = 0;
    m_endReceivedCount    = 0;

    m_headerSent     = false;
    m_endSent        = false;
    m_stimHeaderSent = false;
    m_finished       = false;
    m_resynchroDone  = false;
    m_statsPrinted   = false;

    m_state.m_CurrentFileIdx        = 0;
    m_state.m_CurrentChunkIdx       = 0;
    m_state.m_CurrentStimulationIdx = 0;

    m_triggerDate        = 0;
    m_lastChunkStartTime = 0;
    m_lastChunkEndTime   = 0;

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmSignalConcatenation::uninitialize()
{
    m_stimulationEncoder.uninitialize();
    m_signalEncoder.uninitialize();
    m_triggerEncoder.uninitialize();

    for (uint32_t i = 0; i < m_signalDecoders.size(); ++i)
    {
        m_signalDecoders[i]->uninitialize();
        m_stimulationDecoders[i]->uninitialize();
        delete m_signalDecoders[i];
        delete m_stimulationDecoders[i];
    }

    for (auto& signalChunkBuffer : m_signalChunkBuffers) { for (auto& signalChunk : signalChunkBuffer) { delete signalChunk.m_Buffer; } }

    for (auto& stimulationChunkBuffer : m_stimulationChunkBuffers)
    {
        for (auto& stimulationChunk : stimulationChunkBuffer) { delete stimulationChunk.m_StimulationSet; }
    }

    for (auto& stimulationSet : m_stimulationSets) { delete stimulationSet; }

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmSignalConcatenation::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmSignalConcatenation::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (!m_headerSent || m_finished) { return true; }

    const uint64_t currentTime = this->getPlayerContext().getCurrentTime();

    for (uint32_t i = 0; i < m_fileEndTimes.size(); ++i)
    {
        if (!m_eofReached[i] && currentTime > m_fileEndTimes[i] + m_timeOut)
        {
            m_eofReached[i] = true;
            this->getLogManager() << Kernel::LogLevel_Info << "File #" << i + 1 << "/" << (this->getStaticBoxContext().getInputCount() / 2) <<
                    " has timed out (effective end time: " << CTime(m_fileEndTimes[i]) << ").\n";
        }
    }

    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmSignalConcatenation::process()
{
    const Kernel::IBox& staticBoxContext = this->getStaticBoxContext();
    Kernel::IBoxIO& boxCtx               = this->getDynamicBoxContext();

    //SIGNAL INPUTS
    for (uint32_t i = 0; i < staticBoxContext.getInputCount(); i += 2)
    {
        const uint32_t idx = i >> 1;

        for (uint32_t j = 0; j < boxCtx.getInputChunkCount(i); ++j)
        {
            m_signalDecoders[idx]->decode(j, true);

            if (m_signalDecoders[idx]->isHeaderReceived())
            {
                // Not received all headers we expect? Decode and test ...
                if (m_headerReceivedCount < staticBoxContext.getInputCount() / 2)
                {
                    const uint64_t samplingFrequency    = m_signalDecoders[idx]->getOutputSamplingRate();
                    const uint32_t nChannel             = m_signalDecoders[idx]->getOutputMatrix()->getDimensionSize(0);
                    const uint32_t sampleCountPerBuffer = m_signalDecoders[idx]->getOutputMatrix()->getDimensionSize(1);

                    // Note that the stream may be decoded in any order, hence e.g. stream  2 header may be received before stream 1     ...
                    if (m_headerReceivedCount == 0)
                    {
                        this->getLogManager() << Kernel::LogLevel_Info << "Common sampling rate is " << samplingFrequency << ", channel count is " << nChannel
                                << " and sample count per buffer is " << sampleCountPerBuffer << ".\n";

                        // Set the encoder to follow the parameters of this first received input
                        m_signalEncoder.getInputSamplingRate().setReferenceTarget(m_signalDecoders[idx]->getOutputSamplingRate());
                        m_signalEncoder.getInputMatrix().setReferenceTarget(m_signalDecoders[idx]->getOutputMatrix());

                        m_signalEncoder.encodeHeader();
                        boxCtx.markOutputAsReadyToSend(0, boxCtx.getInputChunkStartTime(i, j), boxCtx.getInputChunkEndTime(i, j));
                        m_headerSent = true;
                    }
                    else
                    {
                        if (m_signalEncoder.getInputSamplingRate() != samplingFrequency)
                        {
                            this->getLogManager() << Kernel::LogLevel_Error << "File #"
                                    << idx + 1 << "/" << (staticBoxContext.getInputCount() / 2)
                                    << " has a different sampling rate (" << samplingFrequency
                                    << "Hz) than other file(s) (" << m_signalEncoder.getInputSamplingRate() << "Hz).\n";
                            return false;
                        }
                        if (m_signalEncoder.getInputMatrix()->getDimensionSize(0) != nChannel)
                        {
                            this->getLogManager() << Kernel::LogLevel_Error << "File #"
                                    << idx + 1 << "/" << (staticBoxContext.getInputCount() / 2)
                                    << " has a different channel count (" << nChannel
                                    << ") than other file(s) (" << m_signalEncoder.getInputMatrix()->getDimensionSize(0) << ").\n";
                            return false;
                        }
                        if (m_signalEncoder.getInputMatrix()->getDimensionSize(1) != sampleCountPerBuffer)
                        {
                            this->getLogManager() << Kernel::LogLevel_Error << "File #"
                                    << idx + 1 << "/" << (staticBoxContext.getInputCount() / 2)
                                    << " has a different sample count per buffer (" << sampleCountPerBuffer
                                    << ") than other file(s) (" << m_signalEncoder.getInputMatrix()->getDimensionSize(1) << ").\n";
                            return false;
                        }
                    }

                    m_headerReceivedCount++;
                }
            }

            if (m_signalDecoders[idx]->isBufferReceived() && !m_eofReached[idx])
            {
                CMemoryBuffer* buffer = new CMemoryBuffer();
                buffer->setSize(boxCtx.getInputChunk(i, j)->getSize(), true);
                memcpy(buffer->getDirectPointer(), boxCtx.getInputChunk(i, j)->getDirectPointer(), buffer->getSize());
                SChunk val;
                val.m_Buffer    = buffer;
                val.m_StartTime = boxCtx.getInputChunkStartTime(i, j);
                val.m_EndTime   = boxCtx.getInputChunkEndTime(i, j);
                m_signalChunkBuffers[idx].push_back(val);

                if (boxCtx.getInputChunkEndTime(i, j) < m_fileEndTimes[idx])
                {
                    this->getLogManager() << Kernel::LogLevel_Warning << "Oops, added extra chunk  " << CTime(boxCtx.getInputChunkStartTime(i, j))
                            << " to " << CTime(boxCtx.getInputChunkEndTime(i, j)) << "\n";
                }

                m_fileEndTimes[idx] = boxCtx.getInputChunkEndTime(i, j);
            }

            if (m_signalDecoders[idx]->isEndReceived())
            {
                // we assume the signal chunks must be continuous, so the end time is the end of the last buffer, don't set here
                //just discard it (automatic by decoder)
            }
        }
    }

    //STIMULATION INPUTS
    for (uint32_t i = 1; i < staticBoxContext.getInputCount(); i += 2)
    {
        const uint32_t idx = i >> 1;

        for (uint32_t j = 0; j < boxCtx.getInputChunkCount(i); ++j)
        {
            m_stimulationDecoders[idx]->decode(j, true);
            if (m_stimulationDecoders[idx]->isHeaderReceived() && !m_stimHeaderSent)
            {
                m_stimulationEncoder.encodeHeader();
                boxCtx.markOutputAsReadyToSend(1, boxCtx.getInputChunkStartTime(i, j), boxCtx.getInputChunkEndTime(i, j));
                m_triggerEncoder.encodeHeader();
                boxCtx.markOutputAsReadyToSend(2, boxCtx.getInputChunkStartTime(i, j), boxCtx.getInputChunkEndTime(i, j));
                m_stimHeaderSent = true;
            }
            if (m_stimulationDecoders[idx]->isBufferReceived() && !m_eofReached[idx])
            {
                const CStimulationSet* stimSet = m_stimulationDecoders[idx]->getOutputStimulationSet();

                SStimulationChunk val;
                val.m_StartTime = boxCtx.getInputChunkStartTime(i, j);
                val.m_EndTime   = boxCtx.getInputChunkEndTime(i, j);

                if (stimSet->size() > 0) { val.m_StimulationSet = new CStimulationSet(); }
                else { val.m_StimulationSet = nullptr; }

                m_stimulationChunkBuffers[idx].
                        push_back(val); // we store even if empty to be able to retain the chunking structure of the stimulation input stream

                for (size_t stim = 0; stim < stimSet->size(); ++stim)
                {
                    val.m_StimulationSet->push_back(stimSet->getId(stim), stimSet->getDate(stim),
                                                            stimSet->getDuration(stim));

                    this->getLogManager() << Kernel::LogLevel_Trace << "Input " << i << ": Discovered stim " << stimSet->getId(stim)
                            << " at date [" << CTime(stimSet->getDate(stim)) << "] in chunk [" << CTime(val.m_StartTime)
                            << ", " << CTime(val.m_EndTime) << "]\n";

                    if (stimSet->getId(stim) == m_eofStimulations[idx])
                    {
                        m_eofReached[idx]   = true;
                        m_fileEndTimes[idx] = val.m_EndTime;
                        this->getLogManager() << Kernel::LogLevel_Info << "File #" << idx + 1 << "/" << (staticBoxContext.getInputCount() / 2) <<
                                " is finished (end time: " << CTime(m_fileEndTimes[idx]) << "). Later signal chunks will be discarded.\n";

                        break;
                    }
                }
            }
            if (m_stimulationDecoders[idx]->isEndReceived() && !m_endSent) { m_endReceivedCount++; }
            if (m_endReceivedCount == staticBoxContext.getInputCount() / 2 - 1) { m_endSent = true; }
        }
    }

    bool shouldConcatenate = true;
    for (auto&& eof : m_eofReached) { shouldConcatenate &= eof; }

    if (shouldConcatenate && !m_statsPrinted)
    {
        for (uint32_t i = 0; i < m_stimulationChunkBuffers.size(); ++i)
        {
            if (!m_signalChunkBuffers[i].empty())
            {
                this->getLogManager() << Kernel::LogLevel_Trace << "File " << i
                        << " has 1st signal chunk at " << CTime(m_signalChunkBuffers[i][0].m_StartTime)
                        << " last at [" << CTime(m_signalChunkBuffers[i].back().m_EndTime)
                        << ", " << CTime(m_signalChunkBuffers[i].back().m_EndTime) << "].\n";
            }
            if (!m_stimulationChunkBuffers[i].empty())
            {
                this->getLogManager() << Kernel::LogLevel_Trace << "File " << i
                        << " has 1st stim chunk at " << CTime(m_stimulationChunkBuffers[i][0].m_StartTime)
                        << " last at [" << CTime(m_stimulationChunkBuffers[i].back().m_EndTime)
                        << ", " << CTime(m_stimulationChunkBuffers[i].back().m_EndTime)
                        << "].\n";
            }
            this->getLogManager() << Kernel::LogLevel_Trace << "File " << i << " EOF is at " << CTime(m_fileEndTimes[i]) << "\n";
        }
        m_statsPrinted = true;
    }

    if (shouldConcatenate && !m_finished)
    {
        if (!this->concate()) { return true; }
        m_stimulationEncoder.encodeEnd();
        boxCtx.markOutputAsReadyToSend(1, m_lastChunkEndTime, m_lastChunkEndTime);
        m_triggerEncoder.encodeEnd();
        boxCtx.markOutputAsReadyToSend(2, m_lastChunkEndTime, m_lastChunkEndTime);
        m_signalEncoder.encodeEnd();
        boxCtx.markOutputAsReadyToSend(0, m_lastChunkEndTime, m_lastChunkEndTime);

        m_triggerEncoder.getInputStimulationSet()->push_back(OVTK_StimulationId_EndOfFile, this->getPlayerContext().getCurrentTime(), 0);
        m_triggerEncoder.encodeBuffer();
        boxCtx.markOutputAsReadyToSend(2, this->getPlayerContext().getCurrentTime(), this->getPlayerContext().getCurrentTime());
        m_finished = true;
    }

    return true;
}


bool CBoxAlgorithmSignalConcatenation::concate()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    if (!m_resynchroDone)
    {
        this->getLogManager() << Kernel::LogLevel_Info << "Concatenation in progress...\n";
        this->getLogManager() << Kernel::LogLevel_Trace << "Resynchronizing Chunks ...\n";

        // note: m_stimulationSets and m_signalChunkBuffers should have the same size (== number of files)

        uint64_t offset = m_fileEndTimes[0];

        for (uint32_t i = 1; i < m_stimulationChunkBuffers.size(); ++i)
        {
            for (auto& stimulationChunkBuffer : m_stimulationChunkBuffers[i])
            {
                CStimulationSet* stimSet = stimulationChunkBuffer.m_StimulationSet;
                if (stimSet)
                {
                    for (size_t k = 0; k < stimSet->size(); ++k)
                    {
                        const uint64_t synchronizedDate = stimSet->getDate(k) + offset;
                        stimSet->setDate(k, synchronizedDate);
                        //this->getLogManager() << Kernel::LogLevel_Info << "Resynchronizing stim ["<<m_stimulations[i][j].first<<"] from time ["<<m_stimulations[i][j].second<<"] to ["<<synchronizedDate<<"]\n";
                    }
                }
                stimulationChunkBuffer.m_StartTime += offset;
                stimulationChunkBuffer.m_EndTime += offset;
            }

            for (auto& signalChunkBuffer : m_signalChunkBuffers[i])
            {
                signalChunkBuffer.m_StartTime += offset;
                signalChunkBuffer.m_EndTime += offset;
            }

            offset = offset + m_fileEndTimes[i];
        }

        this->getLogManager() << Kernel::LogLevel_Trace << "Resynchronization finished.\n";
        m_resynchroDone = true;
    }

    // When we get here, resynchro has been done

    // note that the iterators are references on purpose...

    for (uint32_t& i = m_state.m_CurrentFileIdx; i < m_signalChunkBuffers.size(); ++i)
    {
        const std::vector<SChunk>& chunkVector                    = m_signalChunkBuffers[i];
        const std::vector<SStimulationChunk>& stimulusChunkVector = m_stimulationChunkBuffers[i];

        // Send a signal chunk
        uint32_t& chunk = m_state.m_CurrentChunkIdx;
        if (chunk < chunkVector.size())
        {
            // we write the signal memory buffer
            const CMemoryBuffer* iBuffer = chunkVector[chunk].m_Buffer;
            CMemoryBuffer* oBuffer       = boxContext.getOutputChunk(0);
            oBuffer->setSize(iBuffer->getSize(), true);
            memcpy(oBuffer->getDirectPointer(), iBuffer->getDirectPointer(), iBuffer->getSize());
            boxContext.markOutputAsReadyToSend(0, chunkVector[chunk].m_StartTime, chunkVector[chunk].m_EndTime);

            /*
            if(CTime(chunkVector[chunk].m_StartTime).toSeconds()>236)

            {
                this->getLogManager() << Kernel::LogLevel_Info << "Adding signalchunk " << i << "," << chunk << " ["
                        << CTime(chunkVector[chunk].m_StartTime) << ", " << CTime(chunkVector[chunk].m_EndTime) << "\n";
            }
            */

            const uint64_t signalChunkEnd = chunkVector[chunk].m_EndTime;

            // Write stimulations up to this point
            for (uint32_t& k = m_state.m_CurrentStimulationIdx; k < stimulusChunkVector.size() && stimulusChunkVector[k].m_EndTime <= signalChunkEnd; ++k)
            {
                const SStimulationChunk& stimChunk     = stimulusChunkVector[k];
                const CStimulationSet* bufferedStimSet = stimChunk.m_StimulationSet;

                CStimulationSet* stimSet = m_stimulationEncoder.getInputStimulationSet();
                stimSet->clear();

                if (bufferedStimSet)
                {
                    for (size_t s = 0; s < bufferedStimSet->size(); ++s)
                    {
                        stimSet->push_back(bufferedStimSet->getId(s), bufferedStimSet->getDate(s),
                                                   bufferedStimSet->getDuration(s));

                        this->getLogManager() << Kernel::LogLevel_Trace << "Adding stimulation " << bufferedStimSet->getId(s)
                                << " at date [" << CTime(stimSet->getDate(s))
                                << "] to chunk [" << CTime(stimChunk.m_StartTime)
                                << ", " << CTime(stimChunk.m_EndTime)
                                << "]\n";
                    }
                }

                // encode the stim memory buffer even if it is empty
                m_stimulationEncoder.encodeBuffer();
                boxContext.markOutputAsReadyToSend(1, stimChunk.m_StartTime, stimChunk.m_EndTime);
                /*
                if(CTime(stimChunk.m_StartTime).toSeconds()>238 &&
                    CTime(stimChunk.m_StartTime).toSeconds()<242)

                {
                    this->getLogManager() << Kernel::LogLevel_Info << "Adding stimchunk " << i << "," << k << " ["
                        << CTime(stimChunk.m_StartTime)
                        << ", " << CTime(stimChunk.m_EndTime)
                        << "\n";
                }
                */
            }

            // Let the kernel send blocks up to now, prevent freezing up sending everything at once
            chunk++;
            return false;
        }

        // For now we don't support stimuli that don't correspond to signal data, these ones are after the last signal chunk
        for (uint32_t& k = m_state.m_CurrentStimulationIdx; k < stimulusChunkVector.size(); ++k)
        {
            const SStimulationChunk& stimChunk     = stimulusChunkVector[k];
            const CStimulationSet* bufferedStimSet = stimChunk.m_StimulationSet;

            if (i == m_signalChunkBuffers.size() - 1)
            {
                // last file, let pass

                CStimulationSet* stimSet = m_stimulationEncoder.getInputStimulationSet();
                stimSet->clear();

                if (bufferedStimSet)
                {
                    for (size_t s = 0; s < bufferedStimSet->size(); ++s)
                    {
                        stimSet->push_back(bufferedStimSet->getId(s), bufferedStimSet->getDate(s),
                                                   bufferedStimSet->getDuration(s));

                        this->getLogManager() << Kernel::LogLevel_Warning << "Stimulation " << bufferedStimSet->getId(s)
                                << " at date [" << CTime(stimSet->getDate(s))
                                << "] in chunk [" << CTime(stimChunk.m_StartTime)
                                << ", " << CTime(stimChunk.m_EndTime)
                                << "] is after signal ended, but last file, so adding.\n";
                    }
                }

                // encode the stim memory buffer even if it is empty
                m_stimulationEncoder.encodeBuffer();
                boxContext.markOutputAsReadyToSend(1, stimChunk.m_StartTime, stimChunk.m_EndTime);
            }
            else
            {
                if (bufferedStimSet)
                {
                    for (size_t s = 0; s < bufferedStimSet->size(); ++s)
                    {
                        if (!chunkVector.empty())
                        {
                            this->getLogManager() << Kernel::LogLevel_Warning
                                    << "Stimulation " << bufferedStimSet->getId(s)
                                    << "'s chunk at [" << CTime(stimChunk.m_StartTime)
                                    << ", " << CTime(stimChunk.m_EndTime)
                                    << "] is after the last signal chunk end time " << CTime(chunkVector.back().m_EndTime)
                                    << ", discarded.\n";
                        }
                    }
                }
            }
        }


        // Finished with the file

        //  if(stimChunk.m_EndTime < chunkVector[m_CurrentChunkIdx].m_EndTime) 
        //  {
        // There is no corresponding signal anymore, skip the rest of the stimulations from this file
        //this->getLogManager() << Kernel::LogLevel_Info << "Stimulus time " << CTime(stimulusChunkVector[j].m_EndTime) 
        //  << " exceeds the last signal buffer end time " << CTime(chunkVector[chunkVector.size()-1].m_EndTime) 
        //  << "\n";
        //break;
        //}
        m_state.m_CurrentChunkIdx       = 0;
        m_state.m_CurrentStimulationIdx = 0;

        this->getLogManager() << Kernel::LogLevel_Info << "File #" << i + 1 << " Finished.\n";
    }

    //We search for the last file with data.
    for (uint32_t lastFile = m_signalChunkBuffers.size(); lastFile > 0; lastFile--)
    {
        const uint32_t lastChunkOfLastFile = m_signalChunkBuffers[lastFile - 1].size();
        if (lastChunkOfLastFile != 0)
        {
            m_lastChunkEndTime = m_signalChunkBuffers[lastFile - 1][lastChunkOfLastFile - 1].m_EndTime;
            break;
        }
    }

    this->getLogManager() << Kernel::LogLevel_Info << "Concatenation finished !\n";


    return true;
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
