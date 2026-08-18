#include "ovpCInputChannel.h"
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CInputChannel::initialize(Toolkit::TBoxAlgorithm<IBoxAlgorithm>* boxAlgorithm)
{
    m_isWorking = false;

    m_startTimestamp = 0;
    m_endTimestamp   = 0;

    m_stimulationSet = nullptr;
    m_boxAlgorithm   = boxAlgorithm;

    m_signalDecoder = new Toolkit::TSignalDecoder<Toolkit::TBoxAlgorithm<IBoxAlgorithm>>();
    m_signalDecoder->initialize(*m_boxAlgorithm, 0);

    m_stimDecoder = new Toolkit::TStimulationDecoder<Toolkit::TBoxAlgorithm<IBoxAlgorithm>>();
    m_stimDecoder->initialize(*m_boxAlgorithm, 1);

    return true;
}

bool CInputChannel::uninitialize() const
{
    m_stimDecoder->uninitialize();
    delete m_stimDecoder;

    m_signalDecoder->uninitialize();
    delete m_signalDecoder;

    return true;
}

bool CInputChannel::waitForSignalHeader()
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    if (boxContext.getInputChunkCount(m_signalChannel))
    {
        m_signalDecoder->decode(0);

        if (m_signalDecoder->isHeaderReceived())
        {
            m_isWorking = true;

            m_startTimestamp = boxContext.getInputChunkStartTime(m_signalChannel, 0);
            m_endTimestamp   = boxContext.getInputChunkEndTime(m_signalChannel, 0);

            boxContext.markInputAsDeprecated(m_signalChannel, 0);

            return true;
        }
    }

    return false;
}

CStimulationSet* CInputChannel::getStimulation(uint64_t& startTime, uint64_t& endTime, const size_t index)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    m_stimDecoder->decode(index);
    m_stimulationSet = m_stimDecoder->getOutputStimulationSet();

    startTime = boxContext.getInputChunkStartTime(m_stimulationChannel, index);
    endTime   = boxContext.getInputChunkEndTime(m_stimulationChannel, index);

    boxContext.markInputAsDeprecated(m_stimulationChannel, index);

    return m_stimulationSet;
}

CStimulationSet* CInputChannel::discardStimulation(const size_t index)
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();

    m_stimDecoder->decode(index);
    m_stimulationSet = m_stimDecoder->getOutputStimulationSet();

    boxContext.markInputAsDeprecated(m_stimulationChannel, index);

    return m_stimulationSet;
}


double* CInputChannel::getSignal(uint64_t& startTime, uint64_t& endTime, const size_t index) const
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();
    m_signalDecoder->decode(index);
    if (!m_signalDecoder->isBufferReceived()) { return nullptr; }

    startTime = boxContext.getInputChunkStartTime(m_signalChannel, index);
    endTime   = boxContext.getInputChunkEndTime(m_signalChannel, index);

    boxContext.markInputAsDeprecated(m_signalChannel, index);

    return m_signalDecoder->getOutputMatrix()->getBuffer();
}


double* CInputChannel::discardSignal(const size_t index) const
{
    Kernel::IBoxIO& boxContext = m_boxAlgorithm->getDynamicBoxContext();
    m_signalDecoder->decode(index);
    if (!m_signalDecoder->isBufferReceived()) { return nullptr; }

    boxContext.markInputAsDeprecated(m_signalChannel, index);

    return m_signalDecoder->getOutputMatrix()->getBuffer();
}

#if 0
void CInputChannel::copyData(const bool copyFirstBlock, size_t index)
{
    CMatrix*& matrixBuffer = m_oMatrixBuffer[index & 1];

    double* srcData = m_signalDecoder->getOutputMatrix()->getBuffer() + (copyFirstBlock ? 0 : m_firstBlock);
    double* dstData = matrixBuffer->getBuffer()  + (copyFirstBlock ? m_secondBlock : 0);
    size_t size = (copyFirstBlock ? m_firstBlock : m_secondBlock)*sizeof(double);

    for (size_t i=0; i < m_nChannels; i++, srcData += m_nSamples, dstData += m_nSamples) { System::Memory::copy(dstData, srcData, size); }
}
#endif
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
