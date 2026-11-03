///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300SpellerVisualization.cpp
/// \brief Classes implementation for the Box P300 Speller Visualization.
/// UG fork: ported from GTK to Qt. The stimulation logic is unchanged, the display is CP300SpellerView.
/// \author Yann Renard (INRIA).
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

#include "CBoxAlgorithmP300SpellerVisualization.hpp"
#include "CP300SpellerView.h"

#include <system/ovCTime.h>
#include <tcptagging/IStimulusSender.h>
#include <visualization-toolkit/ovviz_all.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {

// A color setting: red, green and blue in percent
QColor parseColor(const CString& setting)
{
    std::stringstream ss(setting.toASCIIString());
    int r = 0, g = 0, b = 0;
    char c;
    ss >> r >> c >> g >> c >> b;
    return QColor::fromRgbF(float(r) / 100, float(g) / 100, float(b) / 100);
}

}  // namespace

CBoxAlgorithmP300SpellerVisualization::CBoxAlgorithmP300SpellerVisualization()  = default;
CBoxAlgorithmP300SpellerVisualization::~CBoxAlgorithmP300SpellerVisualization() = default;

bool CBoxAlgorithmP300SpellerVisualization::initialize()
{
    m_interfaceFilename     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_rowStimulationBase    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_columnStimulationBase = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    // Three settings for each look: background, foreground and font size
    const std::array<EP300CellStyle, 4> settingOrder = {EP300CellStyle::Flash, EP300CellStyle::NoFlash, EP300CellStyle::Target, EP300CellStyle::Selected};
    SP300SpellerLook look;
    for (size_t i = 0; i < settingOrder.size(); ++i) {
        const auto style         = size_t(settingOrder[i]);
        const CString background = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3 + 3 * i);
        const CString foreground = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4 + 3 * i);
        look.background[style]   = parseColor(background);
        look.foreground[style]   = parseColor(foreground);
        look.fontSize[style]     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5 + 3 * i);
    }

    SP300SpellerLayout layout;
    std::string error;
    if (!loadP300SpellerLayout(m_interfaceFilename.toASCIIString(), layout, error)) {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not load the letter grid: " << error << "\n";
        return false;
    }
    m_nRow  = uint64_t(layout.rows);
    m_nCol  = uint64_t(layout.columns);
    m_state = std::make_unique<CP300SpellerState>(layout);

    // ----------------------------------------------------------------------------------------------------------------------------------------------------------

    m_sequenceStimulationDecoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_sequenceStimulationDecoder->initialize();

    m_targetStimulationDecoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_targetStimulationDecoder->initialize();

    m_targetFlaggingStimulationEncoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
    m_targetFlaggingStimulationEncoder->initialize();

    m_rowSelectionStimulationDecoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_rowSelectionStimulationDecoder->initialize();

    m_columnSelectionStimulationDecoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_columnSelectionStimulationDecoder->initialize();

    m_sequenceMemoryBuffer.initialize(
        m_sequenceStimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    m_sequenceStimulationSet.initialize(m_sequenceStimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    m_targetMemoryBuffer.initialize(m_targetStimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    m_targetStimulationSet.initialize(m_targetStimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    m_targetFlaggingStimulationSet.initialize(
        m_targetFlaggingStimulationEncoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
    m_targetFlaggingMemoryBuffer.initialize(
        m_targetFlaggingStimulationEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));

    m_lastTime = 0;

    m_stimulusSender = nullptr;
    m_stimuliQueue.clear();
    m_heldStimuli.clear();
    m_lastUpdate    = 0;
    m_lastTagTime   = 0;
    m_warnedNoFrame = false;

    const CIdentifier windowManagerID(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto* visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (!visualizationCtx) {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
        return false;
    }

    m_view = std::make_unique<CP300SpellerView>(this->getLogManager(), visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine(), layout, look);
    if (!m_view->visualItem()) {
        // Handing a null item to the window manager crashes it; fail the box instead.
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to build the speller view\n";
        this->releasePluginObject(visualizationCtx);
        return false;
    }

    CIdentifier boxID(getStaticBoxContext().getIdentifier());
    visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_view->visualItem());
    this->releasePluginObject(visualizationCtx);

    m_lastTargetRow = -1;
    m_lastTargetCol = -1;
    m_targetRow     = -1;
    m_targetCol     = -1;
    m_selectedRow   = -1;
    m_selectedCol   = -1;

    m_stimulusSender = TCPTagging::CreateStimulusSender();

    if (!m_stimulusSender->connect("localhost", "15361")) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS TCP Tagging, stimuli wont be forwarded.\n";
    }

    return true;
}

bool CBoxAlgorithmP300SpellerVisualization::uninitialize()
{
    m_stimuliQueue.clear();
    m_heldStimuli.clear();

    if (m_stimulusSender) {
        delete m_stimulusSender;
        m_stimulusSender = nullptr;
    }

    if (m_view) {
        const CIdentifier windowManagerID(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
        auto* visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
        if (visualizationCtx) {
            CIdentifier boxID(getStaticBoxContext().getIdentifier());
            visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
            this->releasePluginObject(visualizationCtx);
        }
        m_view.reset();
    }
    m_state.reset();

    m_targetFlaggingStimulationSet.uninitialize();
    m_targetFlaggingMemoryBuffer.uninitialize();

    m_targetStimulationSet.uninitialize();
    m_targetMemoryBuffer.uninitialize();

    m_sequenceStimulationSet.uninitialize();
    m_sequenceMemoryBuffer.uninitialize();

    if (m_columnSelectionStimulationDecoder) {
        m_columnSelectionStimulationDecoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_columnSelectionStimulationDecoder);
        m_columnSelectionStimulationDecoder = nullptr;
    }

    if (m_rowSelectionStimulationDecoder) {
        m_rowSelectionStimulationDecoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_rowSelectionStimulationDecoder);
        m_rowSelectionStimulationDecoder = nullptr;
    }

    if (m_targetFlaggingStimulationEncoder) {
        m_targetFlaggingStimulationEncoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_targetFlaggingStimulationEncoder);
        m_targetFlaggingStimulationEncoder = nullptr;
    }

    if (m_targetStimulationDecoder) {
        m_targetStimulationDecoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_targetStimulationDecoder);
        m_targetStimulationDecoder = nullptr;
    }

    if (m_sequenceStimulationDecoder) {
        m_sequenceStimulationDecoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_sequenceStimulationDecoder);
        m_sequenceStimulationDecoder = nullptr;
    }

    return true;
}

bool CBoxAlgorithmP300SpellerVisualization::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmP300SpellerVisualization::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // --- Sequence stimulations

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        CStimulationSet flaggingStimulationSet;

        m_sequenceMemoryBuffer         = boxContext.getInputChunk(0, i);
        m_targetFlaggingStimulationSet = &flaggingStimulationSet;
        m_targetFlaggingMemoryBuffer   = boxContext.getOutputChunk(0);

        m_sequenceStimulationDecoder->process();

        m_lastTime = boxContext.getInputChunkEndTime(0, i);

        if (m_sequenceStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) {
            m_targetFlaggingStimulationEncoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader);
        }

        if (m_sequenceStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
            CStimulationSet* stimulationSet = m_sequenceStimulationSet;
            for (size_t j = 0; j < stimulationSet->size(); ++j) {
                uint64_t id   = stimulationSet->getId(j);
                bool flash    = false;
                int row       = -1;
                int col       = -1;
                bool isTarget = false;

                if (id >= m_rowStimulationBase && id < m_rowStimulationBase + m_nRow) {
                    row      = int(id - m_rowStimulationBase);
                    flash    = true;
                    isTarget = (row == m_lastTargetRow);
                }
                if (id >= m_columnStimulationBase && id < m_columnStimulationBase + m_nCol) {
                    col      = int(id - m_columnStimulationBase);
                    flash    = true;
                    isTarget = (col == m_lastTargetCol);
                }
                if (id == OVTK_StimulationId_VisualStimulationStop) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_VisualStimulationStop - resets grid\n";
                    m_state->clearCells();
                }
                if (id == OVTK_StimulationId_Reset) {
                    m_state->clearText();
                }

                if (flash) {
                    m_state->flash(row, col);

                    // We now know if this flash corresponds to the current target or not, merge this to the outgoing stimulation stream
                    if (isTarget) {
                        m_stimuliQueue.push_back(OVTK_StimulationId_Target);
                        flaggingStimulationSet.push_back(OVTK_StimulationId_Target, stimulationSet->getDate(j), 0);
                    }
                    else {
                        m_stimuliQueue.push_back(OVTK_StimulationId_NonTarget);
                        flaggingStimulationSet.push_back(OVTK_StimulationId_NonTarget, stimulationSet->getDate(j), 0);
                    }
                }

                // Pass the stimulation to the server also as-is. If its a flash, it can be differentiated from a 'target' spec because
                // its NOT between OVTK_StimulationId_RestStart and OVTK_StimulationId_RestStop stimuli in the generated P300 timeline.
                m_stimuliQueue.push_back(id);
            }
            m_targetFlaggingStimulationEncoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer);
        }

        if (m_sequenceStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) {
            m_targetFlaggingStimulationEncoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeEnd);
        }

        boxContext.markInputAsDeprecated(0, i);
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
    }

    // --- Target stimulations

    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        if (m_lastTime >= boxContext.getInputChunkStartTime(1, i)) {
            m_targetMemoryBuffer = boxContext.getInputChunk(1, i);
            m_targetStimulationDecoder->process();

            if (m_targetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }

            if (m_targetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
                CStimulationSet* stimulationSet = m_targetStimulationSet;
                for (size_t j = 0; j < stimulationSet->size(); ++j) {
                    uint64_t id = stimulationSet->getId(j);
                    bool target = false;
                    if (id >= m_rowStimulationBase && id < m_rowStimulationBase + m_nRow) {
                        this->getLogManager() << Kernel::LogLevel_Debug << "Received Target Row " << id << "\n";
                        m_targetRow = int(id - m_rowStimulationBase);
                        target      = true;
                    }
                    if (id >= m_columnStimulationBase && id < m_columnStimulationBase + m_nCol) {
                        this->getLogManager() << Kernel::LogLevel_Debug << "Received Target Column " << id << "\n";
                        m_targetCol = int(id - m_columnStimulationBase);
                        target      = true;
                    }

                    if (target && m_targetRow != -1 && m_targetCol != -1) {
                        this->getLogManager() << Kernel::LogLevel_Debug << "Displays Target Cell\n";
                        // Merge the current target into the stimulation stream. It can be differentiated
                        // from a 'flash' spec because it IS between OVTK_StimulationId_RestStart and
                        // OVTK_StimulationId_RestStop stimulations in the P300 timeline.
                        {
                            m_stimuliQueue.push_back(m_targetRow + m_rowStimulationBase);
                            m_stimuliQueue.push_back(m_targetCol + m_columnStimulationBase);
                        }

                        m_state->showTarget(m_targetRow, m_targetCol);
                        m_lastTargetRow = m_targetRow;
                        m_lastTargetCol = m_targetCol;
                        m_targetRow     = -1;
                        m_targetCol     = -1;
                    }
                }
            }

            if (m_targetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }

            boxContext.markInputAsDeprecated(1, i);
        }
    }

    // --- Selection stimulations

    for (size_t k = 2; k < 4; ++k) {
        Kernel::IAlgorithmProxy* decoder = (k == 2 ? m_rowSelectionStimulationDecoder : m_columnSelectionStimulationDecoder);
        Kernel::TParameterHandler<const CMemoryBuffer*> selectionMemoryBuffer(
            decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
        Kernel::TParameterHandler<CStimulationSet*> selectionStimulationSet(
            decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

        for (size_t i = 0; i < boxContext.getInputChunkCount(k); ++i) {
            if (m_lastTime >= boxContext.getInputChunkStartTime(k, i)) {
                selectionMemoryBuffer = boxContext.getInputChunk(k, i);
                decoder->process();

                if (decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }

                if (decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
                    CStimulationSet* stimulationSet = selectionStimulationSet;
                    for (size_t j = 0; j < stimulationSet->size(); ++j) {
                        uint64_t id   = stimulationSet->getId(j);
                        bool selected = false;
                        if (id >= m_rowStimulationBase && id < m_rowStimulationBase + m_nRow) {
                            this->getLogManager() << Kernel::LogLevel_Debug << "Received Selected Row " << id << "\n";
                            m_selectedRow = int(id - m_rowStimulationBase);
                            selected      = true;
                        }
                        if (id >= m_columnStimulationBase && id < m_columnStimulationBase + m_nCol) {
                            this->getLogManager() << Kernel::LogLevel_Debug << "Received Selected Column " << id << "\n";
                            m_selectedCol = int(id - m_columnStimulationBase);
                            selected      = true;
                        }
                        if (id == OVTK_StimulationId_Label_00) {
                            if (k == 2) { m_selectedRow = -2; }
                            if (k == 3) { m_selectedCol = -2; }
                            selected = true;
                        }
                        if (selected && m_selectedRow != -1 && m_selectedCol != -1) {
                            if (m_selectedRow >= 0 && m_selectedCol >= 0) {
                                this->getLogManager() << Kernel::LogLevel_Debug << "Displays Selected Cell\n";
                                m_state->showSelected(m_selectedRow, m_selectedCol);
                            }
                            else {
                                this->getLogManager() << Kernel::LogLevel_Trace << "Selection Rejected !\n";
                                m_state->rejectSelection();
                            }

                            m_selectedRow = -1;
                            m_selectedCol = -1;
                        }
                    }
                }

                if (decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }
                boxContext.markInputAsDeprecated(k, i);
            }
        }
    }

    // What changed goes to the GUI. The stimulations of this round are sent once it is on screen, see processClock().
    if (m_state->takeChanged()) { m_lastUpdate = m_view->show(*m_state); }
    else if (m_heldStimuli.empty()) {
        // Nothing to draw and nothing being drawn
        const uint64_t now = System::Time::zgetTimeRaw(false);
        for (const uint64_t stimulation : m_stimuliQueue) { sendTag(stimulation, now); }
        m_stimuliQueue.clear();
    }

    // Behind what is being drawn, to keep the stimuli in order at the server
    const uint64_t queuedAt = System::Time::zgetTimeRaw(false);
    for (const uint64_t stimulation : m_stimuliQueue) { m_heldStimuli.push_back({stimulation, m_lastUpdate, queuedAt}); }
    m_stimuliQueue.clear();

    return true;
}

bool CBoxAlgorithmP300SpellerVisualization::processClock(Kernel::CMessageClock& /*msg*/)
{
    uint64_t update = 0;
    uint64_t time   = 0;
    while (m_view->takeDisplayed(update, time)) {
        while (!m_heldStimuli.empty() && m_heldStimuli.front().update <= update) {
            sendTag(m_heldStimuli.front().stimulation, time);
            m_heldStimuli.pop_front();
        }
    }

    // Tags are worth more late than lost, e.g. when the panel of the box is out of sight and draws nothing
    const uint64_t now = System::Time::zgetTimeRaw(false);
    if (!m_heldStimuli.empty() && CTime(now - m_heldStimuli.front().queuedAt).toSeconds() > MAX_FRAME_WAIT_SECONDS) {
        if (!m_warnedNoFrame) {
            this->getLogManager() << Kernel::LogLevel_Warning << "No frame drawn for the speller: stimuli are sent without knowing when they were on screen\n";
            m_warnedNoFrame = true;
        }
        for (const SHeldStimulus& held : m_heldStimuli) { sendTag(held.stimulation, now); }
        m_heldStimuli.clear();
    }
    return true;
}

// The time goes with the tag: that of the frame, if there is one, and the acquisition server dates the marker with it.
// A sender refuses a time before its last one.
void CBoxAlgorithmP300SpellerVisualization::sendTag(const uint64_t stimulation, const uint64_t time)
{
    m_lastTagTime = std::max(m_lastTagTime, time);
    m_stimulusSender->sendStimulation(stimulation, m_lastTagTime, TCPTagging::IStimulusSender::Flag_Fptime);
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
