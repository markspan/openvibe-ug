///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300IdentifierStimulator.cpp
/// \author Baptiste Payan (Inria).
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

#include "CBoxAlgorithmP300IdentifierStimulator.hpp"

#include <cstdlib>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

bool CBoxAlgorithmP300IdentifierStimulator::initialize()
{
    //get values of the configure windows for all settings

    m_startStimulation = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_stimulationBase  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_nImages = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    //the number of images must be different to 0
    if (m_nImages == 0) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "This stimulator should at least have 1 Image (got " << m_nImages << ")\n";
        return false;
    }

    m_percentRepetitionTarget = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    //the percent of Repetition contains the target must be between 0% and 100%
    if (m_percentRepetitionTarget > 100.) {
        this->getLogManager() << Kernel::LogLevel_Warning << "The percent of repetition contains Target, should not be more than 100% \n";
        m_percentRepetitionTarget = 100.;
    }

    if (m_percentRepetitionTarget < 0.) {
        this->getLogManager() << Kernel::LogLevel_Warning << "The percent of repetition contains Target, should not be less than 0% \n";
        m_percentRepetitionTarget = 0.;
    }

    m_repetitionCountInTrial  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_nTrial                  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    m_flashDuration           = uint64_t(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6)) * double(1LL << 32));
    m_noFlashDuration         = uint64_t(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7)) * double(1LL << 32));
    m_interRepetitionDuration = uint64_t(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8)) * double(1LL << 32));
    m_interTrialDuration      = uint64_t(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9)) * double(1LL << 32));

    //-----------------------------------------------------------------------------------------------------------------------------------------

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
    m_encoder->initialize();

    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_decoder->initialize();

    m_targetDecoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_targetDecoder->initialize();

    //initialized all variables
    m_headerSent = false;
    m_lastState  = EStates::None;

    m_nFlashInRepet              = m_nImages;
    m_nFlashInRepetWithoutTarget = m_nFlashInRepet - 1;
    m_repetDuration              = m_nFlashInRepet * (m_flashDuration + m_noFlashDuration);
    m_repetDurationWithoutTarget = m_nFlashInRepetWithoutTarget * (m_flashDuration + m_noFlashDuration);

    m_repetTarget = new uint64_t[m_repetitionCountInTrial];

    Reset();

    return true;
}

bool CBoxAlgorithmP300IdentifierStimulator::Reset()
{
    m_lastTime       = 0;
    m_startReceived  = false;
    m_trialStartTime = m_interTrialDuration;

    m_targetNum = 0;
    m_trialIdx  = 1;
    //generate the first trial variables
    this->generateTrialVars();
    return true;
}

bool CBoxAlgorithmP300IdentifierStimulator::uninitialize()
{
    m_decoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_decoder);

    m_targetDecoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_targetDecoder);

    m_encoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_encoder);
    return true;
}

bool CBoxAlgorithmP300IdentifierStimulator::processInput(const size_t /*index*/)
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        if (!m_startReceived) {
            Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer(
                m_decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
            const Kernel::TParameterHandler<CStimulationSet*> op_pStimulationSet(
                m_decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
            ip_buffer = boxContext.getInputChunk(0, i);
            m_decoder->process();

            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }

            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
                for (size_t j = 0; j < op_pStimulationSet->size(); ++j) {
                    if (op_pStimulationSet->getId(j) == m_startStimulation) {
                        this->getLogManager() << Kernel::LogLevel_Trace << "Start\n";
                        m_trialStartTime = op_pStimulationSet->getDate(j) + m_interTrialDuration;
                        m_startReceived  = true;
                    }
                }
            }

            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }
        }

        boxContext.markInputAsDeprecated(0, i);
    }

    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        if (m_startReceived) {
            Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer(
                m_targetDecoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
            const Kernel::TParameterHandler<CStimulationSet*> op_stimSet(
                m_targetDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
            ip_buffer = boxContext.getInputChunk(1, i);
            m_targetDecoder->process();

            if (m_targetDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }

            if (m_targetDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
                for (size_t j = 0; j < op_stimSet->size(); ++j) {
                    const uint64_t stimulationIdx = op_stimSet->getId(j) - m_stimulationBase;
                    if (stimulationIdx < m_nImages) {
                        m_targetNum = int64_t(stimulationIdx);
                        this->getLogManager() << Kernel::LogLevel_Trace << "Choosen number of the targets " << m_targetNum << "\n";
                    }
                }
            }

            if (m_targetDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }
        }

        boxContext.markInputAsDeprecated(1, i);
    }
    return true;
}

bool CBoxAlgorithmP300IdentifierStimulator::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmP300IdentifierStimulator::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const uint64_t time        = this->getPlayerContext().getCurrentTime();
    CStimulationSet stimSet;

    //if start stimulation is receive
    if (m_startReceived) {
        EStates state;  // = EStates::NoFlash;
        uint64_t flashIndex = uint64_t(-1);
        //case of inter-trial
        if (time < m_trialStartTime) { state = EStates::TrialRest; }
        //case of in-trial
        else {
            const uint64_t timeInTrial = time - m_trialStartTime;

            //case of the current time is out of the trial time
            if (timeInTrial >= m_trialDuration) {
                //has next trial
                if (m_nTrial == 0 || m_trialIdx < m_nTrial) {
                    m_trialStartTime = time + m_interTrialDuration;
                    state            = EStates::TrialRest;
                    flashIndex       = uint64_t(-1);
                    m_trialIdx++;
                    generateTrialVars();
                }
                //it was the last trial
                else {
                    state = EStates::None;
                    Reset();
                }
            }
            else {
                const int64_t timeInRepetition = getCurrentTimeInRepetition(timeInTrial);

                //case of the current time is out of the repetition time
                if (timeInRepetition < 0 || uint64_t(timeInRepetition) >= (m_repetWithoutTarget ? m_repetDurationWithoutTarget : m_repetDuration)) {
                    state      = EStates::RepetitionRest;
                    flashIndex = uint64_t(-1);
                }
                else {
                    flashIndex = timeInRepetition / (m_flashDuration + m_noFlashDuration);

                    //case of the current time isn't out of the flash time
                    if (timeInRepetition % (m_flashDuration + m_noFlashDuration) < m_flashDuration) { state = EStates::Flash; }
                    else { state = EStates::NoFlash; }
                }
            }
        }

        //case of the state changed
        if (state != m_lastState) {
            //trigger send about the old state
            switch (m_lastState) {
                //case of the older state was a flash
                case EStates::Flash:
                    stimSet.push_back(OVTK_StimulationId_VisualStimulationStop, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_VisualStimulationStop\n\t; Trial index:" << m_trialIdx
                            << " Repetition index: " << m_repetIdx << "\n";
                    break;
                //case of the older state was a no-flash
                case EStates::NoFlash:
                    break;

                //case of the older state was a inter-repetition
                case EStates::RepetitionRest:
                    if (state != EStates::TrialRest && state != EStates::None) {
                        stimSet.push_back(OVTK_StimulationId_SegmentStart, time, 0);
                        this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStart\n\t; Trial index:" << m_trialIdx
                                << " Repetition index: " << m_repetIdx << "\n";
                    }
                    break;
                //case of the older state was inter-Trial
                case EStates::TrialRest:
                    stimSet.push_back(OVTK_StimulationId_RestStop, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_RestStop\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    stimSet.push_back(OVTK_StimulationId_TrialStart, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_TrialStart\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    stimSet.push_back(OVTK_StimulationId_SegmentStart, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStart\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    break;
                //case of the older state was a None state
                case EStates::None:
                    stimSet.push_back(OVTK_StimulationId_ExperimentStart, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_ExperimentStart\n\t; Trial index:" << m_trialIdx
                            << " Repetition index: " << m_repetIdx << "\n";
                    break;

                default:
                    break;
            }

            //trigger and operation about the new state
            switch (state) {
                //case of the new state is a flash
                case EStates::Flash:
                    stimSet.push_back(m_stimulationBase + m_images[flashIndex], time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_LabelId(x)\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    stimSet.push_back(OVTK_StimulationId_VisualStimulationStart, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_VisualStimulationStart\n\t; Trial index:" << m_trialIdx
                            << " Repetition index: " << m_repetIdx << "\n";
                    break;
                //case of the new state is a no-flash
                case EStates::NoFlash:
                    break;
                //case of the new state is a inter-repetition
                case EStates::RepetitionRest:
                    stimSet.push_back(OVTK_StimulationId_SegmentStop, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    this->generateSequence();
                    break;
                //case of the new state is a inter-trial
                case EStates::TrialRest:
                    m_targetNum = -1;
                    if (m_lastState != EStates::None) {
                        if (m_lastState != EStates::RepetitionRest) {
                            stimSet.push_back(OVTK_StimulationId_SegmentStop, time, 0);
                            this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n\t; Trial index:" << m_trialIdx
                                    << " Repetition index: " << m_repetIdx << "\n";
                        }
                        stimSet.push_back(OVTK_StimulationId_TrialStop, time, 0);
                        this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_TrialStop\n\t; Trial index:" << m_trialIdx <<
                                " Repetition index: "
                                << m_repetIdx << "\n";
                    }
                    stimSet.push_back(OVTK_StimulationId_RestStart, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_RestStart\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    break;
                //case of the new state is a none state
                case EStates::None:
                    if (m_lastState != EStates::RepetitionRest) {
                        stimSet.push_back(OVTK_StimulationId_SegmentStop, time, 0);
                        this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n\t; Trial index:" << m_trialIdx
                                << " Repetition index: " << m_repetIdx << "\n";
                    }
                    stimSet.push_back(OVTK_StimulationId_TrialStop, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_TrialStop\n\t; Trial index:" << m_trialIdx <<
                            " Repetition index: "
                            << m_repetIdx << "\n";
                    stimSet.push_back(OVTK_StimulationId_ExperimentStop, time, 0);
                    this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_ExperimentStop\n\t; Trial index:" << m_trialIdx
                            << " Repetition index: " << m_repetIdx << "\n";
                    break;

                default:
                    break;
            }

            m_lastState = state;
        }
#if 0
        this->getLogManager() << Kernel::LogLevel_Info << "State:" << state_to_string(state) << " - flash index:" << flashIdx << " - repetition index:" << repetitionIdx << " - trial index:" << m_trialIdx << "\n";
#endif
    }

    Kernel::TParameterHandler<CStimulationSet*> ip_stimSet(m_encoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer(
        m_encoder->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));
    ip_stimSet = &stimSet;
    op_buffer  = boxContext.getOutputChunk(0);
    if (!m_headerSent) {
        m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader);
        boxContext.markOutputAsReadyToSend(0, m_lastTime, time);
    }
    if (m_lastTime != time) {
        m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer);
        boxContext.markOutputAsReadyToSend(0, m_lastTime, time);
    }
    m_lastTime   = time;
    m_headerSent = true;
    return true;
}


void CBoxAlgorithmP300IdentifierStimulator::generateSequence()
{
    if (m_repetIdx < m_repetitionCountInTrial) {
        this->getLogManager() << Kernel::LogLevel_Trace << "generate_sequence Repetition: " << m_repetIdx << " Target: " << m_targetNum << "\n";
        if (m_repetIdx > 0) { m_repetWithoutTarget = m_repetTarget[m_repetIdx] == m_repetTarget[m_repetIdx - 1]; }
        else { m_repetWithoutTarget = (m_repetTarget[0] == 0); }
        m_repetIdx++;
        std::vector<size_t> images;
        m_images.clear();
        for (size_t i = 0; i < m_nImages; ++i) { images.push_back(i); }
        this->getLogManager() << Kernel::LogLevel_Trace << "Number target: " << " Repetition without target: " << m_repetWithoutTarget << "\n";
        if (m_repetWithoutTarget && m_targetNum != -1) { images.erase(images.begin() + size_t(m_targetNum)); }
        while (!images.empty()) {
            const int j = rand() % images.size();
            m_images.push_back(size_t(images[j]));
            images.erase(images.begin() + j);
        }
    }
}


void CBoxAlgorithmP300IdentifierStimulator::generateTrialVars()
{
    this->getLogManager() << Kernel::LogLevel_Trace << "generate_trial_vars " << "\n";
    uint64_t nTargetInTrial = 0;
    for (size_t i = 0; i < m_repetitionCountInTrial; ++i) {
        const size_t random = rand();
        //increment the chance to display the target. we would'nt like to have no Target display in a trial.
        double percentRepetitionTargetInc = m_percentRepetitionTarget + (100.0 - m_percentRepetitionTarget) / double(m_repetitionCountInTrial - 1) * double(i);
        const double percentRepetitionTarget = (nTargetInTrial < 1) ? percentRepetitionTargetInc : m_percentRepetitionTarget;

        if (double(random % 100) < percentRepetitionTarget) { nTargetInTrial++; }
        m_repetTarget[i] = nTargetInTrial;
    }
    m_trialDuration = nTargetInTrial * (m_repetDuration + m_interRepetitionDuration)
                      + (m_repetitionCountInTrial - nTargetInTrial) * (m_repetDurationWithoutTarget + m_interRepetitionDuration);
    m_repetIdx  = 0;
    m_targetNum = -1;
}

int64_t CBoxAlgorithmP300IdentifierStimulator::getCurrentTimeInRepetition(const uint64_t timeInTrial)
{
    if (m_repetIdx == 0) { return -1; }
    const int64_t timeInRepetition = int64_t(timeInTrial - m_repetDuration * m_repetTarget[m_repetIdx - 1]
                                             - m_repetDurationWithoutTarget * (m_repetIdx - m_repetTarget[m_repetIdx - 1])
                                             - m_interRepetitionDuration * m_repetIdx);
    //case of the current time in Repetition is out of the current Repetition time
    if (timeInRepetition > 0) {
        generateSequence();
        return getCurrentTimeInRepetition(timeInTrial);
    }
    return int64_t(timeInRepetition + m_interRepetitionDuration + (m_repetWithoutTarget ? m_repetDurationWithoutTarget : m_repetDuration));
}
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
