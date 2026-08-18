//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
// @note This renderer is intended to be used when there is only 
// a stimulation track. More often, the Tracker users might want to
// see the stimulations overlayed on the signal track.
//
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <mensia/advanced-visualization.hpp>
#include <TGtkGLWidget.hpp>
#include <GtkGL.hpp>

#include "StreamRendererStimulation.h"
#include "TypeStimulation.h"

#include "ruler/TRulerAutoType.hpp"
#include "ruler/TRulerPair.hpp"
#include "ruler/TRulerConditionalPair.hpp"
#include "ruler/CRulerConditionIsTimeLocked.hpp"

#include "ruler/CRulerProgressV.hpp"

#include "ruler/CRulerBottomCount.hpp"
#include "ruler/CRulerBottomTime.hpp"

#include "ruler/CRulerLeftChannelNames.hpp"

#include "ruler/CRulerRightCount.hpp"
#include "ruler/CRulerRightScale.hpp"
#include "ruler/CRulerRightLabels.hpp"

namespace OpenViBE {
namespace Tracker {

bool StreamRendererStimulation::initialize()
{
    m_nChannel        = 1;
    m_sampling        = 512;
    m_samplesPerChunk = 32;
    m_chunkDuration   = CTime(m_sampling, m_samplesPerChunk);

    // TRendererStimulation < false, CRendererLine >:new CRendererLine
    AdvancedVisualization::IRenderer* renderer = AdvancedVisualization::IRenderer::Create(AdvancedVisualization::ERendererType::Line, true);
    if (renderer == nullptr) { return false; }

    // Creates renderer context
    m_rendererCtx = new AdvancedVisualization::CRendererContext();
    m_rendererCtx->Clear();
    m_rendererCtx->SetTimeScale(1);
    m_rendererCtx->SetCheckBoardVisibility(true);
    m_rendererCtx->SetScaleVisibility(m_isScaleVisible);
    m_rendererCtx->SetDataType(AdvancedVisualization::CRendererContext::EDataType::Signal);
    m_rendererCtx->AddChannel("Stims");
    m_rendererCtx->SetSampleDuration(CTime(m_sampling, 1).time());

    m_ruler = new AdvancedVisualization::TRulerPair<AdvancedVisualization::CRulerProgressV, AdvancedVisualization::TRulerPair<
                                                        AdvancedVisualization::TRulerAutoType<
                                                            AdvancedVisualization::IRuler, AdvancedVisualization::TRulerConditionalPair<
                                                                AdvancedVisualization::CRulerBottomTime, AdvancedVisualization::CRulerBottomCount,
                                                                AdvancedVisualization::CRulerConditionIsTimeLocked>, AdvancedVisualization::IRuler>,
                                                        AdvancedVisualization::TRulerPair<
                                                            AdvancedVisualization::CRulerLeftChannelNames, AdvancedVisualization::CRulerRightScale>>>;

    m_ruler->SetRendererContext(m_rendererCtx);
    m_ruler->SetRenderer(renderer);

    if (!StreamRendererBase::initialize()) { return false; }

    m_gtkGLWidget.Initialize(*this, m_viewport, m_left, m_right, m_bottom);
    m_gtkGLWidget.SetPointSmoothingActive(false);

    m_renderers.push_back(renderer);

    gtk_widget_set_size_request(m_main, 640, 100);

    return true;
}

bool StreamRendererStimulation::reset(const CTime startTime, const CTime endTime)
{
    m_startTime = startTime;
    m_endTime   = endTime;

    //  std::cout << "Overridden stimulation renderer reset\n";

    m_renderers[0]->Clear(0);

    const uint64_t chunkCount = (m_endTime - m_startTime).time() / m_chunkDuration.time();
    const uint32_t numSamples = uint32_t(m_samplesPerChunk * chunkCount);

    m_renderers[0]->SetChannelCount(m_nChannel);
    m_renderers[0]->SetSampleCount(numSamples);

    //  @FIXME The offset is needed to have correct numbers on the ruler; remove ifdef once the feature is in
#ifdef RENDERER_SUPPORTS_OFFSET
    m_renderers[0]->SetTimeOffset(m_startTime.time());
#endif

    // Stick in empty chunks to get a background
    std::vector<float> empty;
    empty.resize(m_samplesPerChunk * 1, 0);  // 1 channel
    for (uint64_t i = 0; i < chunkCount; ++i) { m_renderers[0]->Feed(&empty[0], m_samplesPerChunk); }

    m_renderers[0]->Rebuild(*m_rendererCtx);

    return true;
}

bool StreamRendererStimulation::push(const TypeStimulation::Buffer& chunk, bool /*zeroInput*/)
{
    for (size_t i = 0; i < chunk.m_buffer.size(); ++i) { m_renderers[0]->Feed(chunk.m_buffer.getDate(i) - m_startTime.time(), chunk.m_buffer.getId(i)); }

    return true;
}

CString StreamRendererStimulation::renderAsText(const size_t /*indent*/) const
{
    // No specific details for stimulation streams
    return "";
}

bool StreamRendererStimulation::showChunkList()
{
    if (m_stimulationListWindow) {
        gtk_window_present(GTK_WINDOW(m_stimulationListWindow));
        return true;
    }

    GtkBuilder* pBuilder   = gtk_builder_new();
    const CString filename = Directories::getDataDir() + "/applications/tracker/tracker.ui";
    if (!gtk_builder_add_from_file(pBuilder, filename, nullptr)) {
        std::cout << "Problem loading [" << filename << "]\n";
        return false;
    }

    m_stimulationListWindow      = GTK_WIDGET(gtk_builder_get_object(pBuilder, "tracker-stimulation_list"));
    GtkTreeView* channelTreeView = GTK_TREE_VIEW(gtk_builder_get_object(pBuilder, "tracker-stimulation_list-treeview"));
    // GtkListStore* channelListStore = GTK_LIST_STORE(gtk_builder_get_object(pBuilder, "liststore_select"));
    GtkTreeStore* channelListStore = gtk_tree_store_new(7, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_UINT64, G_TYPE_STRING,
                                                        G_TYPE_DOUBLE);

    gtk_window_set_title(GTK_WINDOW(m_stimulationListWindow), "List of Stimulations in the stream");

    add_column(channelTreeView, "Chunk#", 0, 20);
    add_column(channelTreeView, "ChunkStart (s)", 1, 10);
    add_column(channelTreeView, "ChunkEnd (s)", 2, 10);
    add_column(channelTreeView, "Stim Time (s)", 3, 20);
    add_column(channelTreeView, "Stim Id", 4, 5);
    add_column(channelTreeView, "Stim Name", 5, 40);
    add_column(channelTreeView, "Stim Duration", 6, 10);

    gtk_tree_view_set_model(channelTreeView, GTK_TREE_MODEL(channelListStore));

    GtkTreeIter it;
    gtk_tree_store_clear(channelListStore);

    // ::gtk_tree_view_set_model(m_pChannelTreeView, nullptr);
    for (size_t i = 0; i < m_stream->getChunkCount(); ++i) {
        const TypeStimulation::Buffer* ptr = m_stream->getChunk(i);
        if (!ptr) { break; }

        for (uint32_t s = 0; s < ptr->m_buffer.size(); ++s) {
            const CString stimName = m_kernelCtx.getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, ptr->m_buffer.getId(s));

            gtk_tree_store_append(channelListStore, &it, nullptr);
            gtk_tree_store_set(channelListStore, &it,
                               0, i,
                               1, ptr->m_StartTime.toSeconds(),
                               2, ptr->m_EndTime.toSeconds(),
                               3, CTime(ptr->m_buffer.getDate(s)).toSeconds(),
                               4, ptr->m_buffer.getId(s),
                               5, stimName.toASCIIString(),
                               6, CTime(ptr->m_buffer.getDuration(s)).toSeconds(), -1);
        }
    }

    //  GList* cols = gtk_tree_view_get_columns(m_pChannelTreeView);

    // Hide instead of destroy on closing the window
    g_signal_connect(m_stimulationListWindow, "delete_event", G_CALLBACK(gtk_widget_hide_on_delete), nullptr);
    gtk_widget_show_all(GTK_WIDGET(m_stimulationListWindow));
    g_object_unref(pBuilder);

    return true;
}

bool StreamRendererStimulation::MouseButton(const int x, const int y, const int button, const int status)
{
    // if (button == 3 && status == 1) { showStimulationList(); }
    return StreamRendererBase::MouseButton(x, y, button, status);
}

}  // namespace Tracker
}  // namespace OpenViBE
