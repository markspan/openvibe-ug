// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"
#include "ovkCBoxAlgorithmContext.h"

#include <cstdlib>
#include <algorithm>
#include <cassert>

namespace OpenViBE {
namespace Kernel {

#define OV_IncorrectTime 0xffffffffffffffffULL

CSimulatedBox::CSimulatedBox(const IKernelContext& ctx, CScheduler& scheduler)
    : TKernelObject<IBoxIO>(ctx), m_scheduler(scheduler), m_lastClockActivationDate(OV_IncorrectTime) {}

CSimulatedBox::~CSimulatedBox() {}

bool CSimulatedBox::setScenarioIdentifier(const CIdentifier& scenarioID)
{
    OV_ERROR_UNLESS_KRF(m_scheduler.getPlayer().getRuntimeScenarioManager().isScenario(scenarioID),
                        "Scenario with identifier " << scenarioID.str() << " does not exist",
                        ErrorType::ResourceNotFound);

    m_scenario = &m_scheduler.getPlayer().getRuntimeScenarioManager().getScenario(scenarioID);
    return true;
}

bool CSimulatedBox::getBoxIdentifier(CIdentifier& boxId) const
{
    OV_ERROR_UNLESS_KRF(m_box, "Simulated box not initialized", Kernel::ErrorType::BadCall);

    boxId = m_box->getIdentifier();
    return true;
}

bool CSimulatedBox::setBoxIdentifier(const CIdentifier& boxId)
{
    OV_ERROR_UNLESS_KRF(m_scenario, "No scenario set", Kernel::ErrorType::BadCall);

    m_box = m_scenario->getBoxDetails(boxId);
    return m_box != nullptr;
}

bool CSimulatedBox::initialize()
{
    OV_ERROR_UNLESS_KRF(m_box, "Simulated box not initialized", Kernel::ErrorType::BadCall);
    OV_ERROR_UNLESS_KRF(m_scenario, "No scenario set", Kernel::ErrorType::BadCall);

    m_chunkConsistencyChecking = this->getConfigurationManager().expandAsBoolean("${Kernel_CheckChunkConsistency}", true);
    m_Inputs.resize(m_box->getInputCount());
    m_Outputs.resize(m_box->getOutputCount());
    m_CurrentOutputs.resize(m_box->getOutputCount());
    m_LastOutputStartTimes.resize(m_box->getOutputCount(), 0);
    m_LastOutputEndTimes.resize(m_box->getOutputCount(), 0);

    m_lastClockActivationDate = OV_IncorrectTime;
    m_clockFrequency          = 0;
    m_clockActivationStep     = 0;

    m_boxAlgorithm = getPluginManager().createBoxAlgorithm(m_box->getAlgorithmClassIdentifier(), nullptr);

    OV_ERROR_UNLESS_KRF(m_boxAlgorithm, "Could not create box algorithm with class id " << m_box->getAlgorithmClassIdentifier().str(),
                        ErrorType::BadResourceCreation);

    {
        CBoxAlgorithmCtx context(getKernelContext(), this, m_box);
        {
            OV_ERROR_UNLESS_KRF(m_boxAlgorithm->initialize(context), "Box algorithm <" << m_box->getName() << "> initialization failed",
                                Kernel::ErrorType::Internal);
        }
    }

    return true;
}

bool CSimulatedBox::uninitialize()
{
    if (!m_boxAlgorithm) { return true; }

    {
        CBoxAlgorithmCtx context(getKernelContext(), this, m_box);
        {
            OV_ERROR_UNLESS_KRF(m_boxAlgorithm->uninitialize(context), "Box algorithm <" << m_box->getName() << "> uninitialization failed",
                                ErrorType::Internal);
        }
    }

    getPluginManager().releasePluginObject(m_boxAlgorithm);
    m_boxAlgorithm = nullptr;

    return true;
}

bool CSimulatedBox::processClock()
{
    {
        CBoxAlgorithmCtx context(getKernelContext(), this, m_box);
        {
            const uint64_t newFreq = m_boxAlgorithm->getClockFrequency(context);
            if (newFreq == 0)
            {
                m_clockActivationStep     = OV_IncorrectTime;
                m_lastClockActivationDate = OV_IncorrectTime;
            }
            else
            {
                OV_ERROR_UNLESS_KRF(newFreq <= m_scheduler.getFrequency()<<32,
                                    "the box <" << m_box->getName() << "> asks to run " << CTime(newFreq).toSeconds() << " times per second, while the player can only manage " << CTime(m_scheduler.getFrequency()<<32).toSeconds() << ". Might we humbly suggest lowering this box's frequency? Thank you most kindly.",
                                    ErrorType::BadConfig);

                // note: 1LL should be left shifted 64 bits but this
                //       would result in an integer over shift (the one
                //       would exit). Thus the left shift of 63 bits
                //       and the left shift of 1 bit after the division
                m_clockActivationStep = ((1ULL << 63) / newFreq) << 1;
            }
            m_clockFrequency = newFreq;
        }
    }

    if ((m_clockFrequency != 0) && (m_lastClockActivationDate == OV_IncorrectTime || m_scheduler.getCurrentTime() - m_lastClockActivationDate >=
                                    m_clockActivationStep))
    {
        CBoxAlgorithmCtx context(getKernelContext(), this, m_box);
        {
            if (m_lastClockActivationDate == OV_IncorrectTime) { m_lastClockActivationDate = m_scheduler.getCurrentTime(); }
            else { m_lastClockActivationDate = m_lastClockActivationDate + m_clockActivationStep; }

            CMessageClock message;
            message.setTime(m_lastClockActivationDate);

            OV_ERROR_UNLESS_KRF(m_boxAlgorithm->processClock(context, message),
                                "Box algorithm <" << m_box->getName() << "> processClock() function failed", Kernel::ErrorType::Internal);

            m_readyToProcess |= context.isAlgorithmReadyToProcess();
        }
    }

    return true;
}

bool CSimulatedBox::processInput(const size_t index, const CChunk& chunk)
{
    m_Inputs[index].push_back(chunk);

    {
        CBoxAlgorithmCtx context(getKernelContext(), this, m_box);
        {
            OV_ERROR_UNLESS_KRF(m_boxAlgorithm->processInput(context, index),
                                // UG fork: the reader's words, not the function's. See UG.md, 2.23.
                                "the box <" << m_box->getName() << "> had to stop because of the problem reported just before this message, "
                                "if there is one. We are so sorry for the interruption", Kernel::ErrorType::Internal);
        }
        m_readyToProcess |= context.isAlgorithmReadyToProcess();
    }

    return true;
}

bool CSimulatedBox::process()
{
    if (!m_readyToProcess) { return true; }
    {
        CBoxAlgorithmCtx context(getKernelContext(), this, m_box);
        {
            // UG fork: the reader's words, not the function's (upstream named the wrong one). See UG.md, 2.23.
            OV_ERROR_UNLESS_KRF(m_boxAlgorithm->process(context),
                                "the box <" << m_box->getName() << "> had to stop because of the problem reported just before this message, "
                                "if there is one. We are so sorry for the interruption",
                                ErrorType::Internal);
        }
    }

    // perform output sending
    {
        CIdentifier* listID = nullptr;
        size_t nbElems      = 0;
        m_scenario->getLinkIdentifierFromBoxList(m_box->getIdentifier(), &listID, &nbElems);
        for (size_t i = 0; i < nbElems; ++i)
        {
            const ILink* link = m_scenario->getLinkDetails(listID[i]);
            if (link)
            {
                CIdentifier dstBoxID        = link->getTargetBoxIdentifier();
                const size_t dstBoxInputIdx = link->getTargetBoxInputIndex();

                const size_t sourceOutputIdx = link->getSourceBoxOutputIndex();
                for (auto& chunk : m_Outputs[sourceOutputIdx]) { m_scheduler.sendInput(chunk, dstBoxID, dstBoxInputIdx); }
            }
        }
        m_scenario->releaseIdentifierList(listID);
    }

    // perform input cleaning
    auto socketIterator = m_Inputs.begin();
    while (socketIterator != m_Inputs.end())
    {
        auto inputChunkIterator = socketIterator->begin();
        while (inputChunkIterator != socketIterator->end())
        {
            if (inputChunkIterator->isDeprecated()) { inputChunkIterator = socketIterator->erase(inputChunkIterator); }
            else { ++inputChunkIterator; }
        }
        ++socketIterator;
    }

    // flushes sent output chunks
    for (auto& socket : m_Outputs) { socket.resize(0); }

    // discards waiting output chunks
    for (const auto& chunk : m_CurrentOutputs)
    {
        OV_FATAL_UNLESS_K(chunk.getBuffer().getSize() == 0, "Output buffer filled but not marked as ready to send. Possible loss of data.",
                          ErrorType::Internal);
    }

    m_readyToProcess = false;

    return true;
}

bool CSimulatedBox::isReadyToProcess() const { return m_readyToProcess; }

// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
// - --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- -
// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

CString CSimulatedBox::getName() const { return m_box->getName(); }

const IScenario& CSimulatedBox::getScenario() const { return *m_scenario; }

// ________________________________________________________________________________________________________________
//

size_t CSimulatedBox::getInputChunkCount(const size_t index) const
{
    OV_ERROR_UNLESS_KRF(index < m_Inputs.size(),
                        "Input index = [" << index << "] is out of range (max index = [" << m_Inputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    return size_t(m_Inputs[index].size());
}

bool CSimulatedBox::getInputChunk(const size_t inputIdx, const size_t chunkIdx, uint64_t& startTime, uint64_t& endTime, size_t& size,
                                  const uint8_t*& buffer) const
{
    OV_ERROR_UNLESS_KRF(inputIdx < m_Inputs.size(),
                        "Input index = [" << inputIdx << "] is out of range (max index = [" << m_Inputs.size() - 1 << "])", Kernel::ErrorType::OutOfBound);

    OV_ERROR_UNLESS_KRF(chunkIdx < m_Inputs[inputIdx].size(),
                        "Input chunk index = [" << chunkIdx << "] is out of range (max index = [" << m_Inputs[inputIdx].size() - 1 << "])",
                        ErrorType::OutOfBound);

    const CChunk& chunk = m_Inputs[inputIdx][chunkIdx];
    startTime           = chunk.getStartTime();
    endTime             = chunk.getEndTime();
    size                = chunk.getBuffer().getSize();
    buffer              = chunk.getBuffer().getDirectPointer();
    return true;
}

const CMemoryBuffer* CSimulatedBox::getInputChunk(const size_t inputIdx, const size_t chunkIdx) const
{
    OV_ERROR_UNLESS_KRN(inputIdx < m_Inputs.size(),
                        "Input index = [" << inputIdx << "] is out of range (max index = [" << m_Inputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    OV_ERROR_UNLESS_KRN(chunkIdx < m_Inputs[inputIdx].size(),
                        "Input chunk index = [" << chunkIdx << "] is out of range (max index = [" << m_Inputs[inputIdx].size() - 1 << "])",
                        ErrorType::OutOfBound);

    return &(m_Inputs[inputIdx][chunkIdx]).getBuffer();
}

uint64_t CSimulatedBox::getInputChunkStartTime(const size_t inputIdx, const size_t chunkIdx) const
{
    OV_ERROR_UNLESS_KRZ(inputIdx < m_Inputs.size(),
                        "Input index = [" << inputIdx << "] is out of range (max index = [" << m_Inputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    OV_ERROR_UNLESS_KRZ(chunkIdx < m_Inputs[inputIdx].size(),
                        "Input chunk index = [" << chunkIdx << "] is out of range (max index = [" << m_Inputs[inputIdx].size() - 1 << "])",
                        ErrorType::OutOfBound);

    const CChunk& chunk = m_Inputs[inputIdx][chunkIdx];
    return chunk.getStartTime();
}

uint64_t CSimulatedBox::getInputChunkEndTime(const size_t inputIdx, const size_t chunkIdx) const
{
    OV_ERROR_UNLESS_KRZ(inputIdx < m_Inputs.size(),
                        "Input index = [" << inputIdx << "] is out of range (max index = [" << m_Inputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    OV_ERROR_UNLESS_KRZ(chunkIdx < m_Inputs[inputIdx].size(),
                        "Input chunk index = [" << chunkIdx << "] is out of range (max index = [" << m_Inputs[inputIdx].size() - 1 << "])",
                        ErrorType::OutOfBound);

    const CChunk& chunk = m_Inputs[inputIdx][chunkIdx];
    return chunk.getEndTime();
}

bool CSimulatedBox::markInputAsDeprecated(const size_t inputIdx, const size_t chunkIdx)
{
    OV_ERROR_UNLESS_KRZ(inputIdx < m_Inputs.size(),
                        "Input index = [" << inputIdx << "] is out of range (max index = [" << m_Inputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    OV_ERROR_UNLESS_KRZ(chunkIdx < m_Inputs[inputIdx].size(),
                        "Input chunk index = [" << chunkIdx << "] is out of range (max index = [" << m_Inputs[inputIdx].size() - 1 << "])",
                        ErrorType::OutOfBound);

    m_Inputs[inputIdx][chunkIdx].markAsDeprecated(true);
    return true;
}

// ________________________________________________________________________________________________________________
//

size_t CSimulatedBox::getOutputChunkSize(const size_t outputIdx) const
{
    OV_ERROR_UNLESS_KRZ(outputIdx < m_CurrentOutputs.size(),
                        "Output index = [" << outputIdx << "] is out of range (max index = [" << m_CurrentOutputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    return m_CurrentOutputs[outputIdx].getBuffer().getSize();
}

bool CSimulatedBox::setOutputChunkSize(const size_t outputIdx, const size_t size, const bool discard)
{
    OV_ERROR_UNLESS_KRF(outputIdx < m_CurrentOutputs.size(),
                        "Output index = [" << outputIdx << "] is out of range (max index = [" << m_CurrentOutputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    return m_CurrentOutputs[outputIdx].getBuffer().setSize(size, discard);
}

uint8_t* CSimulatedBox::getOutputChunkBuffer(const size_t outputIdx)
{
    OV_ERROR_UNLESS_KRN(outputIdx < m_CurrentOutputs.size(),
                        "Output index = [" << outputIdx << "] is out of range (max index = [" << m_CurrentOutputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    return m_CurrentOutputs[outputIdx].getBuffer().getDirectPointer();
}

bool CSimulatedBox::appendOutputChunkData(const size_t outputIdx, const uint8_t* buffer, const size_t size)
{
    OV_ERROR_UNLESS_KRF(outputIdx < m_CurrentOutputs.size(),
                        "Output index = [" << outputIdx << "] is out of range (max index = [" << m_CurrentOutputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    return m_CurrentOutputs[outputIdx].getBuffer().append(buffer, size);
}

CMemoryBuffer* CSimulatedBox::getOutputChunk(const size_t outputIdx)
{
    OV_ERROR_UNLESS_KRN(outputIdx < m_CurrentOutputs.size(),
                        "Output index = [" << outputIdx << "] is out of range (max index = [" << m_CurrentOutputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    return &m_CurrentOutputs[outputIdx].getBuffer();
}

bool CSimulatedBox::markOutputAsReadyToSend(const size_t outputIdx, const uint64_t startTime, const uint64_t endTime)
{
    OV_ERROR_UNLESS_KRF(outputIdx < m_CurrentOutputs.size(),
                        "Output index = [" << outputIdx << "] is out of range (max index = [" << m_CurrentOutputs.size() - 1 << "])",
                        ErrorType::OutOfBound);

    if (m_chunkConsistencyChecking)
    {
        bool isConsistent           = true;
        const char* specificMessage = nullptr;

        // checks chunks consistency
        CIdentifier type;
        m_box->getOutputType(outputIdx, type);
        if (type == OV_TypeId_Stimulations)
        {
            if (m_LastOutputEndTimes[outputIdx] != startTime)
            {
                isConsistent    = false;
                specificMessage = "'Stimulations' streams should have continuously dated chunks";
            }
        }

        if (m_LastOutputEndTimes[outputIdx] > endTime)
        {
            isConsistent    = false;
            specificMessage = "Current 'end time' can not be earlier than previous 'end time'";
        }

        if (m_LastOutputStartTimes[outputIdx] > startTime)
        {
            isConsistent    = false;
            specificMessage = "Current 'start time' can not be earlier than previous 'start time'";
        }

        if (!isConsistent)
        {
            this->getLogManager() << m_chunkConsistencyCheckingLogLevel << "Box <" << m_box->getName() << "> sends inconsistent chunk dates on output [" <<
                    outputIdx << "] (current chunk dates are [" << startTime << "," << endTime << "] whereas previous chunk dates were [" <<
                    m_LastOutputStartTimes[outputIdx] << "," << m_LastOutputEndTimes[outputIdx] << "])\n";
            if (specificMessage) { this->getLogManager() << m_chunkConsistencyCheckingLogLevel << specificMessage << "\n"; }
            this->getLogManager() << m_chunkConsistencyCheckingLogLevel << "Please report to box author and attach your scenario\n";
            this->getLogManager() << LogLevel_Trace << "Previous warning can be disabled setting Kernel_CheckChunkConsistency to false\n";
            m_chunkConsistencyCheckingLogLevel = LogLevel_Trace;
        }

        // sets last times
        m_LastOutputStartTimes[outputIdx] = startTime;
        m_LastOutputEndTimes[outputIdx]   = endTime;
    }

    // sets start and end time
    m_CurrentOutputs[outputIdx].setStartTime(std::min(startTime, endTime));
    m_CurrentOutputs[outputIdx].setEndTime(std::max(startTime, endTime));

    // copies chunk
    m_Outputs[outputIdx].push_back(m_CurrentOutputs[outputIdx]);

    // resets chunk size
    m_CurrentOutputs[outputIdx].getBuffer().setSize(0, true);

    return true;
}

}  // namespace Kernel
}  // namespace OpenViBE
