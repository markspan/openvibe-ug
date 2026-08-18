///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300MagicCardVisualization.cpp
/// \brief Classes implementation for the Box P300 Magic Card Visualization.
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

#include "CBoxAlgorithmP300MagicCardVisualization.hpp"
#include "../utils.hpp"
#include <tcptagging/IStimulusSender.h>

#include <list>
#include <string>
#include <vector>

#include "toolkit/ovtk_stimulations.h"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

// This callback flushes all accumulated stimulations to the TCP Tagging 
// after the rendering has completed.
static gboolean FlushCB(gpointer data)
{
    static_cast<CBoxAlgorithmP300MagicCardVisualization*>(data)->FlushQueue();
    return false;   // Only run once
}

bool CBoxAlgorithmP300MagicCardVisualization::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    m_mainWidgetInterface    = nullptr;
    m_toolbarWidgetInterface = nullptr;

    m_interfaceFilename           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_bgColor                     = CGdkcolorAutoCast(getStaticBoxContext(), getConfigurationManager(), 1);
    m_targetBgColor               = CGdkcolorAutoCast(getStaticBoxContext(), getConfigurationManager(), 2);
    m_selectedBgColor             = CGdkcolorAutoCast(getStaticBoxContext(), getConfigurationManager(), 3);
    m_cardStimulationBase         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    CString tcpTaggingHostAddress = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    CString tcpTaggingHostPort    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
    const CString bgImageFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);

    for (size_t i = 8; i < boxContext.getSettingCount(); ++i) {
        GError* error = nullptr;

        CString fgImageFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        GdkPixbuf* tmp          = gdk_pixbuf_new_from_file(fgImageFilename.toASCIIString(), &error);

        GtkWidget* fgImage = gtk_image_new_from_pixbuf(gdk_pixbuf_scale_simple(tmp, 192, 192, GDK_INTERP_BILINEAR));
        g_object_unref(tmp);

        gtk_widget_show(fgImage);
        g_object_ref(fgImage);
        m_fgImage.push_back(fgImage);
        GtkWidget* bgImage;
        if (bgImageFilename == CString("")) { bgImage = gtk_image_new_from_file((fgImageFilename + CString("-offscreen")).toASCIIString()); }
        else { bgImage = gtk_image_new_from_file(bgImageFilename.toASCIIString()); }
        gtk_widget_show(bgImage);
        g_object_ref(bgImage);
        m_bgImage.push_back(bgImage);
    }

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

    m_cardSelectionStimulationDecoder = &this->getAlgorithmManager().getAlgorithm(
        this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_cardSelectionStimulationDecoder->initialize();

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

    m_mainWidgetInterface = gtk_builder_new(); // glade_xml_new(m_interfaceFilename.toASCIIString(), "p300-magic-card-main", nullptr);
    if (!gtk_builder_add_from_file(m_mainWidgetInterface, m_interfaceFilename.toASCIIString(), nullptr)) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not load interface file [" << m_interfaceFilename << "]\n";
        this->getLogManager() << Kernel::LogLevel_ImportantWarning <<
                "The file may be missing. However, the interface files now use gtk-builder instead of glade. Did you update your files ?\n";
        return false;
    }

    m_toolbarWidgetInterface = gtk_builder_new(); // glade_xml_new(m_interfaceFilename.toASCIIString(), "p300-magic-card-toolbar", nullptr);
    gtk_builder_add_from_file(m_toolbarWidgetInterface, m_interfaceFilename.toASCIIString(), nullptr);

    m_mainWindow    = GTK_WIDGET(gtk_builder_get_object(m_mainWidgetInterface, "p300-magic-card-main"));
    m_toolbarWidget = GTK_WIDGET(gtk_builder_get_object(m_toolbarWidgetInterface, "p300-magic-card-toolbar"));
    m_table         = GTK_TABLE(gtk_builder_get_object(m_mainWidgetInterface, "p300-magic-card-table"));
    gtk_widget_modify_bg(m_mainWindow, GTK_STATE_NORMAL, &m_bgColor);

    gtk_builder_connect_signals(m_mainWidgetInterface, nullptr);
    gtk_builder_connect_signals(m_toolbarWidgetInterface, nullptr);

    m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    m_visualizationCtx->setWidget(*this, m_mainWindow);
    m_visualizationCtx->setToolbar(*this, m_toolbarWidget);

    guint nRow = 0;
    guint nCol = 0;
    g_object_get(m_table, "n-rows", &nRow, nullptr);
    g_object_get(m_table, "n-columns", &nCol, nullptr);

    m_nTableRow  = nRow;
    m_nTableCol  = nCol;
    m_nCard      = m_nTableRow * m_nTableCol;
    m_targetCard = -1;

    m_tableInitialized = false;
    this->cacheBuildFromTable(m_table);
    this->cacheForEach(&CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB, &m_fgImage);
    this->cacheForEach(&CBoxAlgorithmP300MagicCardVisualization::cacheChangeBackgroundCB, &m_bgColor);

    //TCP TAGGING
    m_idleFuncTag = 0;
    m_stimuliQueue.clear();
    m_stimulusSender = TCPTagging::CreateStimulusSender();
    if (!m_stimulusSender->connect("localhost", "15361")) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
    }

    return true;
}

bool CBoxAlgorithmP300MagicCardVisualization::uninitialize()
{
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

    if (m_cardSelectionStimulationDecoder) {
        m_cardSelectionStimulationDecoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_cardSelectionStimulationDecoder);
        m_cardSelectionStimulationDecoder = nullptr;
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
        m_visualizationCtx = nullptr;
    }


    //TCP TAGGING
    m_stimuliQueue.clear();
    if (m_stimulusSender) {
        delete m_stimulusSender;
        m_stimulusSender = nullptr;
    }
    return true;
}

bool CBoxAlgorithmP300MagicCardVisualization::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    if (!m_tableInitialized) {
        this->cacheForEach(&CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB, &m_fgImage);
        m_tableInitialized = true;
    }

    return true;
}

bool CBoxAlgorithmP300MagicCardVisualization::process()
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
            const CStimulationSet* stimSet = m_sequenceStimulationSet;
            for (size_t j = 0; j < stimSet->size(); ++j) {
                uint64_t id = stimSet->getId(j);
                if (id >= m_cardStimulationBase && id < m_cardStimulationBase + m_nCard) {
                    const int card = int(id - m_cardStimulationBase);
                    if (card == m_targetCard) {
                        m_stimuliQueue.push_back(OVTK_StimulationId_Target);
                        flaggingStimulationSet.push_back(OVTK_StimulationId_Target, stimSet->getDate(j), 0);
                    }
                    else {
                        m_stimuliQueue.push_back(OVTK_StimulationId_NonTarget);
                        flaggingStimulationSet.push_back(OVTK_StimulationId_NonTarget, stimSet->getDate(j), 0);
                    }

                    this->cacheForEachIf(card, &CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB,
                                         &CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB, &m_fgImage, &m_bgImage);
                    this->cacheForEach(&CBoxAlgorithmP300MagicCardVisualization::cacheChangeBackgroundCB, &m_bgColor);
                }
                if (id == OVTK_StimulationId_ExperimentStart) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_ExperimentStart - resets grid\n";
                    this->cacheForEach(&CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB, &m_bgImage);
                }
                if (id == OVTK_StimulationId_VisualStimulationStop) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_VisualStimulationStop - resets grid\n";
                    this->cacheForEach(&CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB, &m_bgImage);
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
                const CStimulationSet* stimSet = m_targetStimulationSet;
                for (size_t j = 0; j < stimSet->size(); ++j) {
                    uint64_t id = stimSet->getId(j);
                    if (id >= m_cardStimulationBase && id < m_cardStimulationBase + m_nCard) {
                        this->getLogManager() << Kernel::LogLevel_Debug << "Received Target Card " << id << "\n";
                        m_targetCard = int(id - m_cardStimulationBase);

                        this->getLogManager() << Kernel::LogLevel_Debug << "Displays Target Cell\n";
                        this->cacheForEachIf(m_targetCard, &CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB,
                                             &CBoxAlgorithmP300MagicCardVisualization::cacheChangeNullCB, &m_fgImage, nullptr);
                        this->cacheForEachIf(m_targetCard, &CBoxAlgorithmP300MagicCardVisualization::cacheChangeBackgroundCB,
                                             &CBoxAlgorithmP300MagicCardVisualization::cacheChangeNullCB, &m_targetBgColor, nullptr);

                        // Merge the current target into the stimulation stream. It can be differentiated
                        // from a 'flash' spec because it IS between OVTK_StimulationId_RestStart and
                        // OVTK_StimulationId_RestStop stimulations in the P300 timeline.
                        {
                            //or just stimulationIdentifier
                            m_stimuliQueue.push_back(m_targetCard + m_cardStimulationBase);
                        }
                    }
                }
            }
            if (m_targetStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }
            boxContext.markInputAsDeprecated(1, i);
        }
    }

    // --- Selection stimulations

    Kernel::TParameterHandler<const CMemoryBuffer*> selectionMemoryBuffer(
        m_cardSelectionStimulationDecoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    const Kernel::TParameterHandler<CStimulationSet*> selectionStimulationSet(
        m_cardSelectionStimulationDecoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    for (size_t i = 0; i < boxContext.getInputChunkCount(2); ++i) {
        if (m_lastTime >= boxContext.getInputChunkStartTime(2, i)) {
            selectionMemoryBuffer = boxContext.getInputChunk(2, i);
            m_cardSelectionStimulationDecoder->process();

            if (m_cardSelectionStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }

            if (m_cardSelectionStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
                const CStimulationSet* stimSet = selectionStimulationSet;
                for (size_t j = 0; j < stimSet->size(); ++j) {
                    uint64_t id = stimSet->getId(j);
                    if (id >= m_cardStimulationBase && id < m_cardStimulationBase + m_nCard) {
                        this->getLogManager() << Kernel::LogLevel_Debug << "Received Selected Card " << id << "\n";
                        const int selectedCard = int(id - m_cardStimulationBase);

                        this->getLogManager() << Kernel::LogLevel_Debug << "Displays Selected Cell\n";

                        this->cacheForEachIf(selectedCard, &CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB,
                                             &CBoxAlgorithmP300MagicCardVisualization::cacheChangeNullCB, &m_fgImage, nullptr);
                        this->cacheForEachIf(selectedCard, &CBoxAlgorithmP300MagicCardVisualization::cacheChangeBackgroundCB,
                                             &CBoxAlgorithmP300MagicCardVisualization::cacheChangeNullCB, &m_selectedBgColor, nullptr);
                    }
                    if (id == OVTK_StimulationId_Label_00) {
                        this->getLogManager() << Kernel::LogLevel_Trace << "Selection Rejected !\n";
                        std::string label;
                        label = gtk_label_get_text(m_result);
                        label += "*";
                        gtk_label_set_text(m_result, label.c_str());
                    }
                }
            }

            if (m_cardSelectionStimulationDecoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }

            boxContext.markInputAsDeprecated(2, i);
        }
    }

    // After any possible rendering, we flush the accumulated stimuli. The default idle func is low priority, so it should be run after rendering by gtk.
    if (m_idleFuncTag == 0) { m_idleFuncTag = g_idle_add(FlushCB, this); }
    return true;
}

// _________________________________________________________________________________________________________________________________________________________
//

void CBoxAlgorithmP300MagicCardVisualization::cacheBuildFromTable(GtkTable* table)
{
    if (table) {
        for (GList* list = table->children; list; list = list->next) {
            const GtkTableChild* child = static_cast<GtkTableChild*>(list->data);

            for (size_t i = child->top_attach; i < child->bottom_attach; ++i) {
                for (size_t j = child->left_attach; j < child->right_attach; ++j) {
                    const int idx         = int(i * m_nTableCol + j);
                    widget_style_t& style = m_caches[idx];
                    style.index           = idx;
                    style.parent          = child->widget;
                    style.widget          = gtk_bin_get_child(GTK_BIN(style.parent));
                    style.image           = gtk_bin_get_child(GTK_BIN(style.widget));
                }
            }
        }
    }
}

void CBoxAlgorithmP300MagicCardVisualization::cacheForEach(cache_callback callback, void* data)
{
    for (auto& cache : m_caches) { (this->*callback)(cache.second, data); }
}

void CBoxAlgorithmP300MagicCardVisualization::cacheForEachIf(const int card, cache_callback ifCB, cache_callback elseCB, void* ifUserData, void* elseUserData)
{
    for (auto& cache : m_caches) {
        if (card == cache.second.index) { (this->*ifCB)(cache.second, ifUserData); }
        else { (this->*elseCB)(cache.second, elseUserData); }
    }
}

void CBoxAlgorithmP300MagicCardVisualization::cacheChangeNullCB(widget_style_t& /*widgetStyle*/, void* /*data*/) {}

void CBoxAlgorithmP300MagicCardVisualization::cacheChangeImageCB(widget_style_t& style, void* data)
{
    GtkContainer* container = GTK_CONTAINER(style.widget);
    const auto* pvImage     = static_cast<std::vector<GtkWidget*>*>(data);

    GtkWidget* image = (*pvImage)[style.index];

    if (style.image != image) {
        if (style.image) { gtk_container_remove(container, style.image); }
        gtk_container_add(container, image);
        style.image = image;
    }
}

void CBoxAlgorithmP300MagicCardVisualization::cacheChangeBackgroundCB(widget_style_t& style, void* data)
{
    const GdkColor color = *static_cast<GdkColor*>(data);
    if (memcmp(&style.bgColor, &color, sizeof(GdkColor)) != 0) {
        gtk_widget_modify_bg(style.parent, GTK_STATE_NORMAL, &color);
        gtk_widget_modify_bg(style.widget, GTK_STATE_NORMAL, &color);
        gtk_widget_modify_bg(style.image, GTK_STATE_NORMAL, &color);
        style.bgColor = color;
    }
}

// Note that we don't need concurrency control here as gtk callbacks run in the main thread
void CBoxAlgorithmP300MagicCardVisualization::FlushQueue()
{
    for (const auto& stimulation : m_stimuliQueue) { m_stimulusSender->sendStimulation(stimulation); }
    m_stimuliQueue.clear();

    // This function will be automatically removed after completion, so set to 0
    m_idleFuncTag = 0;
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
