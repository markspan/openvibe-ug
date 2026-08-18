//
// @todo the code logic in process() should be simplified and made more clear, perhaps by adding more states to the machine
//
#include "CBoxAlgorithmP300SpellerStimulator.hpp"

#include <cstdlib>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

/*
CString toString(const EStates state)
{
    switch (state)
    {
        case EStates::None: return "None";
        case EStates::Flash: return "Flash";
        case EStates::NoFlash: return "NoFlash";
        case EStates::RepetitionRest: return "RepetitionRest";
        case EStates::TrialRest: return "TrialRest";
        default: return "unknown";
    }
}
*/

bool CBoxAlgorithmP300SpellerStimulator::initialize()
{
    m_decoder = nullptr;
    m_encoder = nullptr;

    m_startStimulation = this->getTypeManager().getEnumerationEntryValueFromName(
        OV_TypeId_Stimulation, FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
    m_rowStimulationBase = this->getTypeManager().getEnumerationEntryValueFromName(
        OV_TypeId_Stimulation, FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
    m_columnStimulationBase = this->getTypeManager().getEnumerationEntryValueFromName(
        OV_TypeId_Stimulation, FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));

    m_nRow = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_nCol = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    if (m_nRow == 0 || m_nCol == 0) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "This stimulator should at least have 1 row and 1 column (got " << m_nRow << " and " <<
                m_nCol << "\n";
        return false;
    }

    if (m_nRow != m_nCol) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "This stimulator should have the same number of row(s) and columns(s) (got " << m_nRow <<
                " and " << m_nCol << "\n";
        return false;
    }

    m_nRepetInTrial      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    m_nTrial             = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
    m_flashDuration      = CTime(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7))).time();
    m_noFlashDuration    = CTime(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8))).time();
    m_interRepetDuration = CTime(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9))).time();
    m_interTrialDuration = CTime(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10))).time();

    m_avoidNeighborFlashing = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);

    const uint64_t durationThreshold = (10LL << 32) / 1000; // 10ms
    if (m_interRepetDuration < durationThreshold) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Inter repetition duration should not be less than 10 ms\n";
        m_interRepetDuration = durationThreshold;
    }

    if (m_interTrialDuration < durationThreshold) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Inter trial duration should not be less than 10 ms\n";
        m_interTrialDuration = durationThreshold;
    }

    if (m_avoidNeighborFlashing) { this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Avoid neighbor flashing setting is not considered yet\n"; }

    // ----------------------------------------------------------------------------------------------------------------------------------------------------------

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
    m_encoder->initialize();

    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_decoder->initialize();

    m_lastTime      = 0;
    m_headerSent    = false;
    m_startReceived = false;

    m_lastState      = EStates::None;
    m_trialStartTime = m_interTrialDuration;

    m_nFlashInRepet = m_nRow + m_nCol;
    m_repetDuration = m_nFlashInRepet * (m_flashDuration + m_noFlashDuration);
    m_trialDuration = m_nRepetInTrial * (m_repetDuration + m_interRepetDuration);
    m_trialIdx      = 1;

    this->generateSequence();
    return true;
}

bool CBoxAlgorithmP300SpellerStimulator::uninitialize()
{
    if (m_decoder) {
        m_decoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_decoder);
        m_decoder = nullptr;
    }

    if (m_encoder) {
        m_encoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_encoder);
        m_encoder = nullptr;
    }

    return true;
}

bool CBoxAlgorithmP300SpellerStimulator::processInput(const size_t /*index*/)
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        if (!m_startReceived) {
            Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer(
                m_decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
            const Kernel::TParameterHandler<CStimulationSet*> op_stimSet(
                m_decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
            ip_buffer = boxContext.getInputChunk(0, i);
            m_decoder->process();

            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
                for (size_t j = 0; j < op_stimSet->size(); ++j) {
                    if (op_stimSet->getId(j) == m_startStimulation) {
                        m_trialStartTime = op_stimSet->getDate(j) + m_interTrialDuration;
                        m_startReceived  = true;
                    }
                }
            }
            if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }
        }

        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}

bool CBoxAlgorithmP300SpellerStimulator::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmP300SpellerStimulator::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    const uint64_t currentTime = this->getPlayerContext().getCurrentTime();
    CStimulationSet stimulationSet;

    if (m_startReceived) {
        EStates state     = EStates::NoFlash;
        size_t flashIndex = size_t(-1);
        if (currentTime < m_trialStartTime) { state = EStates::TrialRest; }
        else {
            if ((m_trialIdx > m_nTrial) && (m_nTrial > 0)) { state = EStates::ExperimentStop; }
            else {
                const uint64_t curTimeInTrial = currentTime - m_trialStartTime;
                const uint64_t curTimeInRepet = curTimeInTrial % (m_repetDuration + m_interRepetDuration);

                // FIXME is it necessary to keep next line uncomment ?
                //size_t repritionIndexInTrial = currentTimeInTrial/(m_repetDuration+m_interRepetDuration);
                const size_t flashIndexInRepet = curTimeInRepet / (m_flashDuration + m_noFlashDuration);

                flashIndex = flashIndexInRepet;
                // FIXME is it necessary to keep next line uncomment ?
                //repetitionIdx = repritionIndexInTrial;

                if (curTimeInTrial >= m_trialDuration) {
                    if (m_nTrial == 0 || m_trialIdx <= m_nTrial) {
                        m_trialStartTime = currentTime + m_interTrialDuration;
                        state            = EStates::TrialRest;
                        flashIndex       = size_t(-1);
                        // FIXME is it necessary to keep next line uncomment ?
                        //repetitionIdx = size_t(-1);
                        m_trialIdx++;
                    }
                    else {
                        m_trialStartTime = currentTime + m_interTrialDuration;
                        state            = EStates::None;
                    }
                }
                else {
                    if (curTimeInRepet >= m_repetDuration) {
                        state      = EStates::RepetitionRest;
                        flashIndex = size_t(-1);
                    }
                    else {
                        if (curTimeInRepet % (m_flashDuration + m_noFlashDuration) < m_flashDuration) { state = EStates::Flash; }
                        else { state = EStates::NoFlash; }
                    }
                }
            }
        }

        if (state != m_lastState) {
            const bool bRow = ((flashIndex & 1) == 1);
            const int row   = long(bRow ? m_rows[flashIndex >> 1] : -1);
            const int col   = long(bRow ? -1 : m_columns[flashIndex >> 1]);

            switch (m_lastState) {
                case EStates::Flash:
                    stimulationSet.push_back(OVTK_StimulationId_VisualStimulationStop, currentTime, 0);
                //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_VisualStimulationStop\n";
                    break;

                case EStates::NoFlash:
                    break;

                case EStates::RepetitionRest:
                    if (state != EStates::TrialRest && state != EStates::None) {
                        stimulationSet.push_back(OVTK_StimulationId_SegmentStart, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStart\n";
                    }
                    break;

                case EStates::TrialRest:
                    if (m_trialIdx <= m_nTrial) {
                        stimulationSet.push_back(OVTK_StimulationId_RestStop, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_RestStop\n";
                        stimulationSet.push_back(OVTK_StimulationId_TrialStart, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_TrialStart\n";
                        stimulationSet.push_back(OVTK_StimulationId_SegmentStart, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStart\n";
                    }
                    break;

                case EStates::None:
                    if (m_trialIdx <= m_nTrial) {
                        stimulationSet.push_back(OVTK_StimulationId_ExperimentStart, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_ExperimentStart\n";
                    }
                    break;

                default:
                    break;
            }

            switch (state) {
                case EStates::Flash:
                    stimulationSet.push_back(bRow ? m_rowStimulationBase + row : m_columnStimulationBase + col, currentTime, 0);
                //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_LabelId(x)\n";
                    stimulationSet.push_back(OVTK_StimulationId_VisualStimulationStart, currentTime, 0);
                //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_VisualStimulationStart\n";
                    break;

                case EStates::NoFlash:
                    break;

                case EStates::RepetitionRest:
                    stimulationSet.push_back(OVTK_StimulationId_SegmentStop, currentTime, 0);
                //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n";
                    this->generateSequence();
                    break;

                case EStates::TrialRest:
                    if (m_lastState != EStates::None) {
                        if (m_lastState != EStates::RepetitionRest) {
                            stimulationSet.push_back(OVTK_StimulationId_SegmentStop, currentTime, 0);
                            //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n";
                        }
                        stimulationSet.push_back(OVTK_StimulationId_TrialStop, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_TrialStop\n";
                    }
                    if (m_trialIdx <= m_nTrial) {
                        stimulationSet.push_back(OVTK_StimulationId_RestStart, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_RestStart\n";
                    }
                    break;

                case EStates::None:
                    if (m_lastState != EStates::RepetitionRest) {
                        stimulationSet.push_back(OVTK_StimulationId_SegmentStop, currentTime, 0);
                        //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_SegmentStop\n";
                    }
                    stimulationSet.push_back(OVTK_StimulationId_TrialStop, currentTime, 0);
                //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_TrialStop\n";
                    break;
                case EStates::ExperimentStop:
                    // The experiment stop is sent with some delay to allow the last flash / letter to be processed gracefully by the DSP later
                    stimulationSet.push_back(OVTK_StimulationId_ExperimentStop, currentTime + CTime(3.0).time(), 0);
                //this->getLogManager() << Kernel::LogLevel_Trace << "sends OVTK_StimulationId_ExperimentStop\n";
                    break;

                default: break;
            }

            m_lastState = state;
        }
    }

    Kernel::TParameterHandler<CStimulationSet*> ip_stimSet(m_encoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer(
        m_encoder->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));
    ip_stimSet = &stimulationSet;
    op_buffer  = boxContext.getOutputChunk(0);
    if (!m_headerSent) {
        m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader);
        boxContext.markOutputAsReadyToSend(0, m_lastTime, currentTime);
    }
    if (m_lastTime != currentTime) {
        m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer);
        boxContext.markOutputAsReadyToSend(0, m_lastTime, currentTime);
    }
    m_lastTime   = currentTime;
    m_headerSent = true;

    return true;
}

void CBoxAlgorithmP300SpellerStimulator::generateSequence()
{
    std::vector<size_t> rows;
    m_rows.clear();
    for (size_t i = 0; i < m_nRow; ++i) { rows.push_back(i); }
    for (size_t i = 0; i < m_nRow; ++i) {
        const size_t j = rand() % rows.size();
        m_rows[i]      = rows[j];
        rows.erase(rows.begin() + j);
    }

    std::vector<size_t> cols;
    m_columns.clear();
    for (size_t i = 0; i < m_nCol; ++i) { cols.push_back(i); }
    for (size_t i = 0; i < m_nCol; ++i) {
        const size_t j = rand() % cols.size();
        m_columns[i]   = cols[j];
        cols.erase(cols.begin() + j);
    }
}
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
