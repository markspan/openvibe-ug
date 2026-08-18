///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmP300IdentifierCardVisualization.cpp
/// \brief Classes implementation for the Box P300 Identifier Card Visualization.
/// \author Baptiste Payan (INRIA).
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

#include "CBoxAlgorithmP300IdentifierCardVisualization.hpp"
#include "../utils.hpp"
#include <iomanip>
#include <list>
#include <string>
#include <vector>

#include "toolkit/ovtk_stimulations.h"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CBoxAlgorithmP300IdentifierCardVisualization::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    m_mainWidgetInterface = nullptr;

    //get value of settings given in the configuration box
    m_interfaceFilename   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_bgColor             = CGdkcolorAutoCast(getStaticBoxContext(), getConfigurationManager(), 1);
    m_targetBgColor       = CGdkcolorAutoCast(getStaticBoxContext(), getConfigurationManager(), 2);
    m_selectedBgColor     = CGdkcolorAutoCast(getStaticBoxContext(), getConfigurationManager(), 3);
    m_cardStimulationBase = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

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

    m_lastTime            = 0;
    m_mainWidgetInterface = gtk_builder_new(); // glade_xml_new(m_interfaceFilename.toASCIIString(), "p300-Identifier-card-main", nullptr);
    if (!gtk_builder_add_from_file(m_mainWidgetInterface, m_interfaceFilename.toASCIIString(), nullptr)) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not load interface file [" << m_interfaceFilename << "]\n";
        this->getLogManager() << Kernel::LogLevel_ImportantWarning <<
                "The file may be missing. However, the interface files now use gtk-builder instead of glade. Did you update your files ?\n";
        return false;
    }

    // m_toolbarWidgetInterface=glade_xml_new(m_interfaceFilename.toASCIIString(), "p300-Identifier-card-toolbar", nullptr);
    m_mainWindow = GTK_WIDGET(gtk_builder_get_object(m_mainWidgetInterface, "p300-Identifier-card-main"));
    // m_toolbarWidget=gtk_builder_get_object(m_toolbarWidgetInterface, "p300-Identifier-card-toolbar");
    m_table = GTK_TABLE(gtk_builder_get_object(m_mainWidgetInterface, "p300-Identifier-card-table"));
    gtk_widget_modify_bg(m_mainWindow, GTK_STATE_NORMAL, &m_bgColor);

    std::stringstream targetColor, selectColor;
    targetColor << std::hex << std::setfill('0') << std::setw(2) << m_targetBgColor.red << std::setw(2) << m_targetBgColor.green << std::setw(2) <<
            m_targetBgColor.blue;
    selectColor << std::hex << std::setfill('0') << std::setw(2) << m_selectedBgColor.red << std::setw(2) << m_selectedBgColor.green << std::setw(2) <<
            m_selectedBgColor.blue;

    m_targetLabel = GTK_LABEL(gtk_builder_get_object(m_mainWidgetInterface, "labelTarget"));
    gtk_label_set_label(m_targetLabel, (R"(<span weight="bold" size="xx-large" color="#)" + targetColor.str() + "\">Target</span>").c_str());
    m_selectedLabel = GTK_LABEL(gtk_builder_get_object(m_mainWidgetInterface, "labelResult"));
    gtk_label_set_label(m_selectedLabel, (R"(<span weight="bold" size="xx-large" color="#)" + targetColor.str() + "\">Selected</span>").c_str());

    gtk_builder_connect_signals(m_mainWidgetInterface, nullptr);
    // gtk_builder_connect_signals(m_toolbarWidgetInterface, nullptr);


    m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    m_visualizationCtx->setWidget(*this, m_mainWindow);
    // getVisualizationContext().setToolbar(m_toolbarWidget);

    m_nCard      = 0;
    m_targetCard = -1;

    m_tableInitialized = false;
    this->cacheBuildFromTable(m_table);
    GtkRequisition size;

    gtk_widget_size_request(GTK_WIDGET(m_caches[1].widget), &size);

    const gint widthWork  = size.width;
    const gint heightWork = size.height;

    for (size_t i = 6; i < boxContext.getSettingCount(); ++i) {
        CString fgImageFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        if (fgImageFilename != CString("")) {
            GError* error = nullptr;

            GdkPixbuf* tmp = gdk_pixbuf_new_from_file(fgImageFilename.toASCIIString(), &error);

            GtkWidget* fgImageTarget = gtk_image_new_from_pixbuf(gdk_pixbuf_scale_simple(tmp, 192, 192, GDK_INTERP_BILINEAR));
            g_object_unref(tmp);

            gtk_widget_show(fgImageTarget);
            g_object_ref(fgImageTarget);
            m_fgImageTargets.push_back(fgImageTarget);

            GtkWidget* fgImageWork = gtk_image_new_from_file(fgImageFilename.toASCIIString());
            gtk_widget_show(fgImageWork);
            g_object_ref(fgImageWork);
            const GdkPixbuf* srcPixbuf = gtk_image_get_pixbuf(GTK_IMAGE(fgImageWork));
            GdkPixbuf* destPixbuf      = gdk_pixbuf_scale_simple(srcPixbuf, widthWork, heightWork, GDK_INTERP_HYPER);
            gtk_image_set_from_pixbuf(GTK_IMAGE(fgImageWork), destPixbuf);
            m_fgImageWorks.push_back(fgImageWork);

            GtkWidget* fgImageResult = gtk_image_new_from_file(fgImageFilename.toASCIIString());
            gtk_widget_show(fgImageResult);
            g_object_ref(fgImageResult);
            m_fgImageResults.push_back(fgImageResult);
            m_nCard++;
        }
    }

    const CString bgImageFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    m_bgImageTarget               = gtk_image_new_from_file((bgImageFilename + CString("-offscreen")).toASCIIString());
    gtk_widget_show(m_bgImageTarget);
    g_object_ref(m_bgImageTarget);

    m_bgImageWork = gtk_image_new_from_file((bgImageFilename + CString("-offscreen")).toASCIIString());
    gtk_widget_show(m_bgImageWork);
    g_object_ref(m_bgImageWork);
    const GdkPixbuf* srcPixbuf = gtk_image_get_pixbuf(GTK_IMAGE(m_bgImageWork));
    GdkPixbuf* destPixbuf      = gdk_pixbuf_scale_simple(srcPixbuf, widthWork, heightWork, GDK_INTERP_HYPER);
    gtk_image_set_from_pixbuf(GTK_IMAGE(m_bgImageWork), destPixbuf);

    m_bgImageResult = gtk_image_new_from_file((bgImageFilename + CString("-offscreen")).toASCIIString());
    gtk_widget_show(m_bgImageResult);
    g_object_ref(m_bgImageResult);

    this->cacheChangeImageCB(m_caches[0], m_bgImageTarget);
    this->cacheChangeImageCB(m_caches[1], m_bgImageWork);
    this->cacheChangeImageCB(m_caches[2], m_bgImageResult);
    this->cacheForEach(&CBoxAlgorithmP300IdentifierCardVisualization::cacheChangeBackgroundCB, &m_bgColor);
    return true;
}

bool CBoxAlgorithmP300IdentifierCardVisualization::uninitialize()
{
    // g_object_unref(m_toolbarWidgetInterface);
    // m_toolbarWidgetInterface= nullptr;

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

    return true;
}

bool CBoxAlgorithmP300IdentifierCardVisualization::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    if (!m_tableInitialized) {
        this->cacheChangeImageCB(m_caches[0], m_bgImageTarget);
        this->cacheChangeImageCB(m_caches[1], m_bgImageWork);
        this->cacheChangeImageCB(m_caches[2], m_bgImageResult);
        m_tableInitialized = true;
    }

    return true;
}


bool CBoxAlgorithmP300IdentifierCardVisualization::process()
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
                const uint64_t id = stimSet->getId(j);

                if (id >= m_cardStimulationBase && id < m_cardStimulationBase + m_nCard) {
                    const int card = int(id - m_cardStimulationBase);
                    if (card == m_targetCard) { flaggingStimulationSet.push_back(OVTK_StimulationId_Target, stimSet->getDate(j), 0); }
                    else { flaggingStimulationSet.push_back(OVTK_StimulationId_NonTarget, stimSet->getDate(j), 0); }
                    this->cacheChangeImageCB(m_caches[1], m_fgImageWorks[card]);
                }
                else if (id == OVTK_StimulationId_ExperimentStart) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_ExperimentStart - resets grid\n";
                    this->cacheChangeImageCB(m_caches[0], m_bgImageTarget);
                    this->cacheChangeBackgroundCB(m_caches[0], &m_bgColor);
                    this->cacheChangeImageCB(m_caches[1], m_bgImageWork);
                    this->cacheChangeBackgroundCB(m_caches[1], &m_bgColor);
                    this->cacheChangeImageCB(m_caches[2], m_bgImageResult);
                    this->cacheChangeBackgroundCB(m_caches[2], &m_bgColor);
                }
                else if (id == OVTK_StimulationId_VisualStimulationStop) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_VisualStimulationStop - resets grid\n";
                    //this->cacheChangeImageCB(m_caches[1], m_backgroundImageWork);
                    GtkContainer* container = GTK_CONTAINER(m_caches[1].widget);
                    gtk_container_remove(container, m_caches[1].image);
                    m_caches[1].image = nullptr;
                }
                else if (id == OVTK_StimulationId_SegmentStop) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_SegmentStop - resets grid\n";
                    this->cacheChangeImageCB(m_caches[1], m_bgImageWork);
                }
                else if (id == OVTK_StimulationId_ExperimentStop) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_ExperimentStop - resets grid\n";
                    this->cacheChangeImageCB(m_caches[0], m_bgImageTarget);
                    this->cacheChangeImageCB(m_caches[1], m_bgImageWork);
                    this->cacheChangeImageCB(m_caches[2], m_bgImageResult);
                    this->cacheChangeBackgroundCB(m_caches[0], &m_bgColor);
                    this->cacheChangeBackgroundCB(m_caches[1], &m_bgColor);
                    this->cacheChangeBackgroundCB(m_caches[2], &m_bgColor);
                }
                else if (id == OVTK_StimulationId_RestStop) {
                    this->getLogManager() << Kernel::LogLevel_Debug << "Received OVTK_StimulationId_RestStop - resets grid\n";
                    this->cacheChangeImageCB(m_caches[2], m_bgImageResult);
                    this->cacheChangeBackgroundCB(m_caches[2], &m_bgColor);
                }
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
                        this->cacheChangeImageCB(m_caches[0], m_fgImageTargets[m_targetCard]);
                        this->cacheChangeBackgroundCB(m_caches[0], &m_targetBgColor);
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

                        this->cacheChangeImageCB(m_caches[1], m_bgImageWork);
                        this->cacheChangeImageCB(m_caches[2], m_fgImageResults[selectedCard]);
                        this->cacheChangeBackgroundCB(m_caches[2], &m_selectedBgColor);
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
    return true;
}

// _________________________________________________________________________________________________________________________________________________________
//

void CBoxAlgorithmP300IdentifierCardVisualization::cacheBuildFromTable(GtkTable* table)
{
    if (table) {
        for (GList* list = table->children; list; list = list->next) {
            const GtkTableChild* child = static_cast<GtkTableChild*>(list->data);

            if (child->top_attach != 0) {
                int idx = 0;
                for (size_t i = child->top_attach; i < child->bottom_attach; ++i) {
                    for (size_t j = child->left_attach; j < child->right_attach; ++j) {
                        idx++;
                        widget_style_t style;
                        style.index  = idx;
                        style.parent = child->widget;
                        style.widget = gtk_bin_get_child(GTK_BIN(style.parent));
                        style.image  = gtk_bin_get_child(GTK_BIN(style.widget));
                        m_caches.push_back(style);
                    }
                }
            }
        }
    }
}

void CBoxAlgorithmP300IdentifierCardVisualization::cacheForEach(cache_callback callback, void* data)
{
    for (auto& cache : m_caches) { (this->*callback)(cache, data); }
}

void CBoxAlgorithmP300IdentifierCardVisualization::cacheForEachIf(const int card, cache_callback ifCB, cache_callback elseCB,
                                                                  void* ifUserData, void* elseUserData)
{
    for (auto& cache : m_caches) {
        if (card == cache.index) { (this->*ifCB)(cache, ifUserData); }
        else { (this->*elseCB)(cache, elseUserData); }
    }
}

void CBoxAlgorithmP300IdentifierCardVisualization::cacheChangeNullCB(widget_style_t& /*style*/, void* /*data*/) {}

void CBoxAlgorithmP300IdentifierCardVisualization::cacheChangeImageCB(widget_style_t& style, void* data)
{
    GtkContainer* container = GTK_CONTAINER(style.widget);
    auto* image             = static_cast<GtkWidget*>(data);

    if (style.image != image) {
        if (style.image) { gtk_container_remove(container, style.image); }
        gtk_container_add(container, image);
        style.image = image;
    }
}

void CBoxAlgorithmP300IdentifierCardVisualization::cacheChangeBackgroundCB(widget_style_t& style, void* data)
{
    const GdkColor color = *static_cast<GdkColor*>(data);
    if (memcmp(&style.bgColor, &color, sizeof(GdkColor)) != 0) {
        gtk_widget_modify_bg(style.parent, GTK_STATE_NORMAL, &color);
        gtk_widget_modify_bg(style.widget, GTK_STATE_NORMAL, &color);
        gtk_widget_modify_bg(style.image, GTK_STATE_NORMAL, &color);
        style.bgColor = color;
    }
}
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
