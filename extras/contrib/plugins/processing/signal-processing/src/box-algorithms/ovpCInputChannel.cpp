#include "ovpCInputChannel.h"

#include <iostream>
#include <cstring>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

namespace {
class _AutoCast_
{
public:
    _AutoCast_(const Kernel::IBox& box, Kernel::IConfigurationManager& configManager, const size_t index) : m_configManager(configManager)
    {
        box.getSettingValue(index, m_settingValue);
    }

    operator uint64_t() const { return m_configManager.expandAsUInteger(m_settingValue); }
    operator int64_t() const { return m_configManager.expandAsInteger(m_settingValue); }
    operator double() const { return m_configManager.expandAsFloat(m_settingValue); }
    operator bool() const { return m_configManager.expandAsBoolean(m_settingValue); }
    operator const CString() const { return m_configManager.expand(m_settingValue); }
protected:
    Kernel::IConfigurationManager& m_configManager;
    CString m_settingValue;
};
}  // namespace

CInputChannel::CInputChannel()
{
    m_oMatrix[0] = nullptr;
    m_oMatrix[1] = nullptr;
}

CInputChannel::~CInputChannel()
{
    if (m_oMatrix[0]) { delete m_oMatrix[0]; }
    if (m_oMatrix[1]) { delete m_oMatrix[1]; }
}

bool CInputChannel::initialize(Toolkit::TBoxAlgorithm<IBoxAlgorithm>* boxAlgorithm)
{
    m_status     = 0;
    m_oMatrix[0] = nullptr;
    m_oMatrix[1] = nullptr;

    m_timeStimulationPos   = 0;
    m_timeStimulationStart = 0;
    m_timeStimulationEnd   = 0;
    m_hasFirstStimulation  = false;

    m_timeSignalPos   = 0;
    m_timeSignalStart = 0;
    m_timeSignalEnd   = 0;

    m_stimulationSet = nullptr;
    m_boxAlgorithm   = boxAlgorithm;
    m_ptrMatrixIdx   = 0;

    m_synchroStimulation = m_boxAlgorithm->getTypeManager().getEnumerationEntryValueFromName(
        OV_TypeId_Stimulation,
        _AutoCast_(m_boxAlgorithm->getStaticBoxContext(), m_boxAlgorithm->getConfigurationManager(), 0));

    m_signalDecoder = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
        m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_signalDecoder->initialize();
    ip_bufferSignal.initialize(m_signalDecoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
    op_matrixSignal.initialize(m_signalDecoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Matrix));
    op_sampling.initialize(m_signalDecoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling));

    m_stimDecoder = &m_boxAlgorithm->getAlgorithmManager().getAlgorithm(
        m_boxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_stimDecoder->initialize();
    ip_bufferStimulation.initialize(m_stimDecoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    op_stimulationSet.initialize(m_stimDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    return true;
}

bool CInputChannel::uninitialize()
{
    op_stimulationSet.uninitialize();
    ip_bufferStimulation.uninitialize();
    m_stimDecoder->uninitialize();
    m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_stimDecoder);


    op_sampling.uninitialize();
    op_matrixSignal.uninitialize();
    ip_bufferSignal.uninitialize();
    m_signalDecoder->uninitialize();
    m_boxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_signalDecoder);


    return true;
}

bool CInputChannel::waitForSignalHeader()
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    if (boxContext.getInputChunkCount(SIGNAL_CHANNEL))
    {
        ip_bufferSignal = boxContext.getInputChunk(SIGNAL_CHANNEL, 0);
        m_signalDecoder->process();

        if (m_signalDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader))
        {
            m_status |= SIGNAL_HEADER_DETECTED;

            if (m_oMatrix[0]) { delete m_oMatrix[0]; }
            m_oMatrix[0] = new CMatrix();
            if (m_oMatrix[1]) { delete m_oMatrix[1]; }
            m_oMatrix[1] = new CMatrix();
            m_oMatrix[0]->copyDescription(*op_matrixSignal);
            m_oMatrix[1]->copyDescription(*op_matrixSignal);
            boxContext.markInputAsDeprecated(SIGNAL_CHANNEL, 0);

            return true;
        }
    }

    return false;
}

void CInputChannel::waitForSynchro()
{
    waitForSynchroStimulation();
    waitForSynchroSignal();
}

void CInputChannel::waitForSynchroStimulation()
{
    if (hasSynchroStimulation()) { return; }

    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(STIMULATION_CHANNEL); ++i) //Stimulation de l'input 1
    {
        ip_bufferStimulation = boxContext.getInputChunk(STIMULATION_CHANNEL, i);
        m_stimDecoder->process();
        m_stimulationSet = op_stimulationSet;

        m_timeStimulationStart = boxContext.getInputChunkStartTime(STIMULATION_CHANNEL, i);
        m_timeStimulationEnd   = boxContext.getInputChunkEndTime(STIMULATION_CHANNEL, i);

        for (size_t j = 0; j < m_stimulationSet->size(); ++j)
        {
            if (m_stimulationSet->getId(j) == m_synchroStimulation)
            {
                m_status |= STIMULATION_SYNCHRO_DETECTED;
                m_timeStimulationPos = m_stimulationSet->getDate(j);
                m_boxAlgorithm->getLogManager() << Kernel::LogLevel_Info << "Get Synchronisation Stimulation at channel " << STIMULATION_CHANNEL << "\n";

                return;
            }
        }
        boxContext.markInputAsDeprecated(STIMULATION_CHANNEL, i);
    }
}

void CInputChannel::waitForSynchroSignal()
{
    if (m_timeStimulationStart == 0 || hasSynchroSignal()) { return; }

    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    if (hasSynchroStimulation())
    {
        for (size_t i = 0; i < boxContext.getInputChunkCount(SIGNAL_CHANNEL); ++i) //Stimulation de l'input 1
        {
            m_timeSignalStart = boxContext.getInputChunkStartTime(SIGNAL_CHANNEL, i);
            m_timeSignalEnd   = boxContext.getInputChunkEndTime(SIGNAL_CHANNEL, i);
            if ((m_timeStimulationPos >= m_timeSignalStart) && (m_timeStimulationPos < m_timeSignalEnd)) { processSynchroSignal(); }
            boxContext.markInputAsDeprecated(SIGNAL_CHANNEL, i);

            if (hasSynchroSignal()) { break; }
        }
    }
    else
    {
        for (size_t i = 0; i < boxContext.getInputChunkCount(SIGNAL_CHANNEL); ++i) //Stimulation de l'input 1
        {
            m_timeSignalEnd = boxContext.getInputChunkEndTime(SIGNAL_CHANNEL, i);
            if (m_timeSignalEnd < m_timeStimulationStart) { boxContext.markInputAsDeprecated(SIGNAL_CHANNEL, i); }
        }
    }
}

void CInputChannel::processSynchroSignal()
{
    m_status |= SIGNAL_SYNCHRO_DETECTED;
    m_nChannels     = m_oMatrix[0]->getDimensionSize(0);
    m_nSamples      = m_oMatrix[0]->getDimensionSize(1);
    m_firstBlock    = size_t(double(m_nSamples * (m_timeStimulationPos - m_timeSignalStart)) / double(m_timeSignalEnd - m_timeSignalStart));
    m_secondBlock   = m_nSamples - m_firstBlock;
    m_timeSignalPos = m_timeSignalEnd;

    copyData(false, m_ptrMatrixIdx);

    m_boxAlgorithm->getLogManager() << Kernel::LogLevel_Info << "Cutting parameter for both part :  " << m_firstBlock << "+" << m_secondBlock << "\n";
}

CStimulationSet* CInputChannel::getStimulation(uint64_t& startTimestamp, uint64_t& endTimestamp, const size_t stimulationIndex)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    ip_bufferStimulation = boxContext.getInputChunk(STIMULATION_CHANNEL, stimulationIndex);
    m_stimDecoder->process();
    m_stimulationSet = op_stimulationSet;

    startTimestamp        = m_hasFirstStimulation ? boxContext.getInputChunkStartTime(STIMULATION_CHANNEL, stimulationIndex) : m_timeStimulationPos;
    endTimestamp          = boxContext.getInputChunkEndTime(STIMULATION_CHANNEL, stimulationIndex);
    m_hasFirstStimulation = true;

    boxContext.markInputAsDeprecated(STIMULATION_CHANNEL, stimulationIndex);

    return m_stimulationSet;
}


CMatrix* CInputChannel::getSignal(uint64_t& startTimestamp, uint64_t& endTimestamp, const size_t signalIndex)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();
    ip_bufferSignal            = boxContext.getInputChunk(SIGNAL_CHANNEL, signalIndex);
    m_signalDecoder->process();
    if (!m_signalDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer)) { return nullptr; }

    startTimestamp = boxContext.getInputChunkStartTime(SIGNAL_CHANNEL, signalIndex);
    endTimestamp   = boxContext.getInputChunkEndTime(SIGNAL_CHANNEL, signalIndex);

    copyData(true, m_ptrMatrixIdx);
    copyData(false, m_ptrMatrixIdx + 1);

    boxContext.markInputAsDeprecated(SIGNAL_CHANNEL, signalIndex);

    return getMatrix();
}

void CInputChannel::copyData(const bool copyFirstBlock, const size_t matrixIndex)
{
    CMatrix*& buffer = m_oMatrix[matrixIndex & 1];

    double* src       = op_matrixSignal->getBuffer() + (copyFirstBlock ? 0 : m_firstBlock);
    double* dst       = buffer->getBuffer() + (copyFirstBlock ? m_secondBlock : 0);
    const size_t size = (copyFirstBlock ? m_firstBlock : m_secondBlock) * sizeof(double);

    for (size_t i = 0; i < m_nChannels; i++, src += m_nSamples, dst += m_nSamples) { memcpy(dst, src, size_t(size)); }
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
