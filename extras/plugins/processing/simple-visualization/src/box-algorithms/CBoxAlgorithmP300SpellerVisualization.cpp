///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300SpellerVisualization.cpp
/// \brief Classes implementation for the Box P300 Speller Visualization.
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

// @todo for clarity, the StimulusSender related code blocks should be pushed inside the class and away from here

#include "CBoxAlgorithmP300SpellerVisualization.hpp"
#include "../utils.hpp"
#include <tcptagging/IStimulusSender.h>

#include <algorithm>
#include <list>
#include <string>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

static void ToggleButtonShowHideCB(GtkToggleToolButton* button, gpointer data)
{
    if (gtk_toggle_tool_button_get_active(button)) { gtk_widget_show(GTK_WIDGET(data)); }
    else { gtk_widget_hide(GTK_WIDGET(data)); }
}

// This callback flushes all accumulated stimulations to the TCP Tagging 
// after the rendering has completed.
static gboolean FlushCB(gpointer data)
{
    (static_cast<CBoxAlgorithmP300SpellerVisualization*>(data))->FlushQueue();

    return false;   // Only run once
}

bool CBoxAlgorithmP300SpellerVisualization::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    m_mainWidgetInterface    = nullptr;
    m_toolbarWidgetInterface = nullptr;
    m_flashFontDesc          = nullptr;
    m_noFlashFontDesc        = nullptr;
    m_targetFontDesc         = nullptr;
    m_selectedFontDesc       = nullptr;

    // ----------------------------------------------------------------------------------------------------------------------------------------------------------

    m_interfaceFilename     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_rowStimulationBase    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_columnStimulationBase = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    m_flashBgColor     = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 3);
    m_flashFgColor     = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 4);
    m_flashFontSize    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    m_noFlashBgColor   = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 6);
    m_noFlashFgColor   = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 7);
    m_noFlashFontSize  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
    m_targetBgColor    = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 9);
    m_targetFgColor    = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 10);
    m_targetFontSize   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);
    m_selectedBgColor  = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 12);
    m_selectedFgColor  = CGdkcolorAutoCast(boxContext, this->getConfigurationManager(), 13);
    m_selectedFontSize = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 14);

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

    m_idleFuncTag = 0;
    m_stimuliQueue.clear();

    m_mainWidgetInterface = gtk_builder_new(); // glade_xml_new(m_interfaceFilename.toASCIIString(), "p300-speller-main", nullptr);
    if (!gtk_builder_add_from_file(m_mainWidgetInterface, m_interfaceFilename.toASCIIString(), nullptr)) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not load interface file [" << m_interfaceFilename << "]\n";
        this->getLogManager() << Kernel::LogLevel_ImportantWarning <<
                "The file may be missing. However, the interface files now use gtk-builder instead of glade. Did you update your files ?\n";
        return false;
    }

    m_toolbarWidgetInterface = gtk_builder_new(); // glade_xml_new(m_interfaceFilename.toASCIIString(), "p300-speller-toolbar", nullptr);
    gtk_builder_add_from_file(m_toolbarWidgetInterface, m_interfaceFilename.toASCIIString(), nullptr);

    m_mainWindow    = GTK_WIDGET(gtk_builder_get_object(m_mainWidgetInterface, "p300-speller-main"));
    m_toolbarWidget = GTK_WIDGET(gtk_builder_get_object(m_toolbarWidgetInterface, "p300-speller-toolbar"));
    m_table         = GTK_TABLE(gtk_builder_get_object(m_mainWidgetInterface, "p300-speller-table"));
    m_result        = GTK_LABEL(gtk_builder_get_object(m_mainWidgetInterface, "label-result"));
    m_target        = GTK_LABEL(gtk_builder_get_object(m_mainWidgetInterface, "label-target"));

    gtk_builder_connect_signals(m_mainWidgetInterface, nullptr);
    gtk_builder_connect_signals(m_toolbarWidgetInterface, nullptr);

    g_signal_connect(gtk_builder_get_object(m_toolbarWidgetInterface, "toolbutton-show_target_text"), "toggled", G_CALLBACK(ToggleButtonShowHideCB),
                     gtk_builder_get_object(m_mainWidgetInterface, "label-target"));
    g_signal_connect(gtk_builder_get_object(m_toolbarWidgetInterface, "toolbutton-show_target_text"), "toggled", G_CALLBACK(ToggleButtonShowHideCB),
                     gtk_builder_get_object(m_mainWidgetInterface, "label-target-title"));
    g_signal_connect(gtk_builder_get_object(m_toolbarWidgetInterface, "toolbutton-show_result_text"), "toggled", G_CALLBACK(ToggleButtonShowHideCB),
                     gtk_builder_get_object(m_mainWidgetInterface, "label-result"));
    g_signal_connect(gtk_builder_get_object(m_toolbarWidgetInterface, "toolbutton-show_result_text"), "toggled", G_CALLBACK(ToggleButtonShowHideCB),
                     gtk_builder_get_object(m_mainWidgetInterface, "label-result-title"));

    m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    m_visualizationCtx->setWidget(*this, m_mainWindow);
    m_visualizationCtx->setToolbar(*this, m_toolbarWidget);

    guint nRow = 0, nCol = 0;
    g_object_get(m_table, "n-rows", &nRow, nullptr);
    g_object_get(m_table, "n-columns", &nCol, nullptr);

    m_nRow = nRow;
    m_nCol = nCol;

    PangoFontDescription* maxFontDesc = pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_mainWindow)));
    m_flashFontDesc                   = pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_mainWindow)));
    m_noFlashFontDesc                 = pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_mainWindow)));
    m_targetFontDesc                  = pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_mainWindow)));
    m_selectedFontDesc                = pango_font_description_copy(pango_context_get_font_description(gtk_widget_get_pango_context(m_mainWindow)));

    uint64_t maxSize = 0;
    maxSize          = std::max(maxSize, m_flashFontSize);
    maxSize          = std::max(maxSize, m_noFlashFontSize);
    maxSize          = std::max(maxSize, m_targetFontSize);
    maxSize          = std::max(maxSize, m_selectedFontSize);

    pango_font_description_set_size(maxFontDesc, gint(maxSize * PANGO_SCALE));
    pango_font_description_set_size(m_flashFontDesc, gint(m_flashFontSize * PANGO_SCALE));
    pango_font_description_set_size(m_noFlashFontDesc, gint(m_noFlashFontSize * PANGO_SCALE));
    pango_font_description_set_size(m_targetFontDesc, gint(m_targetFontSize * PANGO_SCALE));
    pango_font_description_set_size(m_selectedFontDesc, gint(m_selectedFontSize * PANGO_SCALE));

    this->cacheBuildFromTable(m_table);
    this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB, &m_noFlashBgColor);
    this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB, &m_noFlashFgColor);
    this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB, maxFontDesc);

    pango_font_description_free(maxFontDesc);

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

    m_tableInitialized = false;

    return true;
}

bool CBoxAlgorithmP300SpellerVisualization::uninitialize()
{
    if (m_idleFuncTag) {
        m_stimuliQueue.clear();
        g_source_remove(m_idleFuncTag);
        m_idleFuncTag = 0;
    }

    if (m_stimulusSender) {
        delete m_stimulusSender;
        m_stimulusSender = nullptr;
    }

    if (m_selectedFontDesc) {
        pango_font_description_free(m_selectedFontDesc);
        m_selectedFontDesc = nullptr;
    }

    if (m_targetFontDesc) {
        pango_font_description_free(m_targetFontDesc);
        m_targetFontDesc = nullptr;
    }

    if (m_noFlashFontDesc) {
        pango_font_description_free(m_noFlashFontDesc);
        m_noFlashFontDesc = nullptr;
    }

    if (m_flashFontDesc) {
        pango_font_description_free(m_flashFontDesc);
        m_flashFontDesc = nullptr;
    }

    if (m_toolbarWidgetInterface) {
        g_object_unref(m_toolbarWidgetInterface);
        m_toolbarWidgetInterface = nullptr;
    }

    if (m_mainWidgetInterface) {
        g_object_unref(m_mainWidgetInterface);
        m_mainWidgetInterface = nullptr;
    }

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

    if (m_visualizationCtx) {
        this->releasePluginObject(m_visualizationCtx);
        m_sequenceStimulationDecoder = nullptr;
    }

    return true;
}

bool CBoxAlgorithmP300SpellerVisualization::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    if (!m_tableInitialized) {
        this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB, &m_noFlashBgColor);
        this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB, &m_noFlashFgColor);
        this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB, m_noFlashFontDesc);
        m_tableInitialized = true;
    }

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
                    this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB, &m_noFlashBgColor);
                    this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB, &m_noFlashFgColor);
                    this->cacheForEach(&CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB, m_noFlashFontDesc);
                }
                if (id == OVTK_StimulationId_Reset) {
                    gtk_label_set_text(m_target, "");
                    gtk_label_set_text(m_result, "");
                }

                if (flash) {
                    this->cacheForEachIf(row, col, &CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB,
                                         &CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB, &m_flashBgColor, &m_noFlashBgColor);
                    this->cacheForEachIf(row, col, &CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB,
                                         &CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB, &m_flashFgColor, &m_noFlashFgColor);
                    this->cacheForEachIf(row, col, &CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB,
                                         &CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB, m_flashFontDesc, m_noFlashFontDesc);

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
                        this->cacheForEachIf(m_targetRow, m_targetCol, &CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB,
                                             &CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB, &m_targetBgColor, nullptr);
                        this->cacheForEachIf(m_targetRow, m_targetCol, &CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB,
                                             &CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB, &m_targetFgColor, nullptr);
                        this->cacheForEachIf(m_targetRow, m_targetCol, &CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB,
                                             &CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB, m_targetFontDesc, nullptr);

                        std::vector<GtkWidget*> widgets;
                        this->cacheForEachIf(m_targetRow, m_targetCol, &CBoxAlgorithmP300SpellerVisualization::cacheCollectChildWidgetCB,
                                             &CBoxAlgorithmP300SpellerVisualization::cacheCollectChildWidgetCB, &widgets, nullptr);

                        // Merge the current target into the stimulation stream. It can be differentiated
                        // from a 'flash' spec because it IS between OVTK_StimulationId_RestStart and
                        // OVTK_StimulationId_RestStop stimulations in the P300 timeline.
                        {
                            m_stimuliQueue.push_back(m_targetRow + m_rowStimulationBase);
                            m_stimuliQueue.push_back(m_targetCol + m_columnStimulationBase);
                        }

                        if (widgets.size() == 1) {
                            if (GTK_IS_LABEL(widgets[0])) {
                                std::string label;
                                label = gtk_label_get_text(m_target);
                                label += gtk_label_get_text(GTK_LABEL(widgets[0]));
                                gtk_label_set_text(m_target, label.c_str());
                            }
                            else {
                                this->getLogManager() << Kernel::LogLevel_Warning << "Expected label class widget... could not find a valid text to append\n";
                            }
                        }
                        else {
                            this->getLogManager() << Kernel::LogLevel_Warning << "Did not find a unique widget at row:" << size_t(m_targetRow) << " column:" <<
                                    size_t(m_targetCol) << "\n";
                        }

                        m_targetHistory.emplace_back(m_targetRow, m_targetCol);
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
                        if (id >= m_columnStimulationBase && id < m_columnStimulationBase + m_nRow) {
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
                                this->cacheForEachIf(m_selectedRow, m_selectedCol, &CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB,
                                                     &CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB, &m_selectedBgColor, nullptr);
                                this->cacheForEachIf(m_selectedRow, m_selectedCol, &CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB,
                                                     &CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB, &m_selectedFgColor, nullptr);
                                this->cacheForEachIf(m_selectedRow, m_selectedCol, &CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB,
                                                     &CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB, m_selectedFontDesc, nullptr);

                                std::vector<GtkWidget*> widgets;
                                this->cacheForEachIf(m_selectedRow, m_selectedCol, &CBoxAlgorithmP300SpellerVisualization::cacheCollectChildWidgetCB,
                                                     &CBoxAlgorithmP300SpellerVisualization::cacheCollectChildWidgetCB, &widgets, nullptr);

                                if (widgets.size() == 1) {
                                    if (GTK_IS_LABEL(widgets[0])) {
                                        std::string label;
                                        label = gtk_label_get_text(GTK_LABEL(widgets[0]));
                                        if (!m_targetHistory.empty()) {
                                            auto it          = m_targetHistory.begin();
                                            bool correct     = (it->first == m_selectedRow && it->second == m_selectedCol);
                                            bool halfCorrect = (it->first == m_selectedRow || it->second == m_selectedCol);
                                            m_targetHistory.pop_front();
                                            std::string tmp;
                                            if (correct) { tmp = "<span color=\"darkgreen\">"; }
                                            else if (halfCorrect) { tmp = "<span color=\"darkorange\">"; }
                                            else { tmp = "<span color=\"darkred\">"; }
                                            label = tmp.append(label).append("</span>");
                                        }
                                        label = std::string(gtk_label_get_label(m_result)).append(label);
                                        gtk_label_set_markup(m_result, label.c_str());
                                    }
                                    else {
                                        this->getLogManager() << Kernel::LogLevel_Warning <<
                                                "Expected label class widget... could not find a valid text to append\n";
                                    }
                                }
                                else {
                                    this->getLogManager() << Kernel::LogLevel_Warning << "Did not find a unique widget at row : " << size_t(m_selectedRow) <<
                                            " column : " << size_t(m_selectedCol) << "\n";
                                }
                            }
                            else {
                                this->getLogManager() << Kernel::LogLevel_Trace << "Selection Rejected !\n";
                                std::string label;
                                label = gtk_label_get_text(m_result);
                                label += "*";
                                gtk_label_set_text(m_result, label.c_str());
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

    // After any possible rendering, we flush the accumulated stimuli. The default idle func is low priority, so it should be run after rendering by gtk.
    if (m_idleFuncTag == 0) { m_idleFuncTag = g_idle_add(FlushCB, this); }

    return true;
}

// _________________________________________________________________________________________________________________________________________________________
//

void CBoxAlgorithmP300SpellerVisualization::cacheBuildFromTable(GtkTable* table)
{
    if (table) {
        const GdkColor white = InitGDKColor(65535, 65535, 65535, 65535);

        for (GList* list = table->children; list; list = list->next) {
            const GtkTableChild* child = static_cast<GtkTableChild*>(list->data);

            for (size_t i = child->top_attach; i < child->bottom_attach; ++i) {
                for (size_t j = child->left_attach; j < child->right_attach; ++j) {
                    widget_style_t& style = m_cache[i][j];
                    style.widget          = child->widget;
                    style.childWidget     = gtk_bin_get_child(GTK_BIN(child->widget));
                    style.bgColor         = white;
                    style.fgColor         = white;
                    style.fontDesc        = nullptr;
                }
            }
        }
    }
}

void CBoxAlgorithmP300SpellerVisualization::cacheForEach(const cache_callback callback, void* data)
{
    for (auto i = m_cache.begin(); i != m_cache.end(); ++i) {
        for (auto j = i->second.begin(); j != i->second.end(); ++j) { (this->*callback)(j->second, data); }
    }
}

void CBoxAlgorithmP300SpellerVisualization::cacheForEachIf(const int iLine, const int iColumn, const cache_callback ifCB, const cache_callback elseCB,
                                                           void* ifUserData, void* elseUserData)
{
    for (auto i = m_cache.begin(); i != m_cache.end(); ++i) {
        for (auto j = i->second.begin(); j != i->second.end(); ++j) {
            const bool line   = (iLine != -1);
            const bool column = (iColumn != -1);
            bool inLine       = false;
            bool inCol        = false;
            bool first;

            if (line && size_t(iLine) == i->first) { inLine = true; }
            if (column && size_t(iColumn) == j->first) { inCol = true; }

            if (line && column) { first = inLine && inCol; }
            else { first = inLine || inCol; }

            if (first) { (this->*ifCB)(j->second, ifUserData); }
            else { (this->*elseCB)(j->second, elseUserData); }
        }
    }
}

void CBoxAlgorithmP300SpellerVisualization::cacheChangeNullCB(widget_style_t& /*rWidgetStyle*/, void* /*data*/) { }

void CBoxAlgorithmP300SpellerVisualization::cacheChangeBackgroundCB(widget_style_t& style, void* data)
{
    const GdkColor color = *static_cast<GdkColor*>(data);
    if (memcmp(&style.bgColor, &color, sizeof(GdkColor)) != 0) {
        gtk_widget_modify_bg(style.widget, GTK_STATE_NORMAL, &color);
        style.bgColor = color;
    }
}

void CBoxAlgorithmP300SpellerVisualization::cacheChangeForegroundCB(widget_style_t& style, void* data)
{
    const GdkColor color = *static_cast<GdkColor*>(data);
    if (memcmp(&style.fgColor, &color, sizeof(GdkColor)) != 0) {
        gtk_widget_modify_fg(style.childWidget, GTK_STATE_NORMAL, &color);
        style.fgColor = color;
    }
}

void CBoxAlgorithmP300SpellerVisualization::cacheChangeFontCB(widget_style_t& style, void* data)
{
    auto* fontDescription = static_cast<PangoFontDescription*>(data);
    if (style.fontDesc != fontDescription) {
        gtk_widget_modify_font(style.childWidget, fontDescription);
        style.fontDesc = fontDescription;
    }
}

void CBoxAlgorithmP300SpellerVisualization::cacheCollectWidgetCB(widget_style_t& style, void* data)
{
    if (data) { (static_cast<std::vector<GtkWidget*>*>(data))->push_back(style.widget); }
}

void CBoxAlgorithmP300SpellerVisualization::cacheCollectChildWidgetCB(widget_style_t& style, void* data)
{
    if (data) { (static_cast<std::vector<GtkWidget*>*>(data))->push_back(style.childWidget); }
}

// Note that we don't need concurrency control here as gtk callbacks run in the main thread
void CBoxAlgorithmP300SpellerVisualization::FlushQueue()
{
    for (const auto& stimulation : m_stimuliQueue) { m_stimulusSender->sendStimulation(stimulation); }
    m_stimuliQueue.clear();

    // This function will be automatically removed after completion, so set to 0
    m_idleFuncTag = 0;
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
