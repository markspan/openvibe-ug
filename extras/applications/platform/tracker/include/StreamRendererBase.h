//
// OpenViBE Tracker
//


#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>
#include <memory> // shared_ptr

#include <mensia/advanced-visualization.hpp>
#include <TGtkGLWidget.hpp>

#include <IRuler.hpp>

#include "Stream.h"
#include "Contexted.h"

typedef struct _GtkWidget GtkWidget;

namespace OpenViBE {
namespace Tracker {

/**
 * \class StreamRendererBase 
 * \brief Abstract, non-typed class for visually rendering the Streams in the Tracker GUI
 * \details See the derived, typed versions of the class
 *
 * \author J. T. Lindgren
 *
 */
class StreamRendererBase : protected Contexted
{
public:
    explicit StreamRendererBase(const Kernel::IKernelContext& ctx) : Contexted(ctx) { }
    ~StreamRendererBase() override { }

    // Calls for API users

    // Initialize the renderer
    virtual bool initialize();
    virtual bool uninitialize();

    // Show the windows
    virtual bool realize();

    virtual bool setTitle(const char* title);

    // Draw chunks in range [startTime,endTime]
    virtual bool spool(const CTime startTime, const CTime endTime) = 0;

    virtual CString renderAsText(const size_t indent = 0) const;

    virtual bool showChunkList()
    {
        log() << Kernel::LogLevel_Warning << "Chunk listing not implemented for stream type (or unavailable in current Tracker mode)\n";
        return true;
    }

    virtual CTime getChunkDuration() const { return m_chunkDuration; }

    virtual bool setRulerVisibility(bool isVisible);

    // Save/load settings as configuration manager tokens
    // The tokens loaded/saved will have the given prefix
    virtual bool restoreSettings(const std::string& prefix);
    virtual bool storeSettings(const std::string& prefix);

    GtkWidget* getWidget() const { return m_main; }

    // GtkGlWidget calls the following

    virtual bool Redraw(bool bImmediate = false);
    virtual bool Reshape(uint32_t width, uint32_t height);
    virtual bool Draw();
    virtual bool PreDraw();
    virtual bool PostDraw();
    virtual void DrawLeft();
    virtual void DrawRight();
    virtual void DrawBottom();
    virtual bool MouseButton(int x, int y, int button, int status);
    virtual bool MouseMotion(int x, int y);

    virtual bool Keyboard(uint32_t /*width*/, uint32_t /*height*/, uint32_t /*value*/, bool /*unused*/)
    {
        std::cout << "keyb requested\n";
        return true;
    }


protected:
    // Resets the renderer aperture (number of samples/chunks)
    virtual bool reset(CTime startTime, CTime endTime) = 0;
    virtual bool updateRulerVisibility();
    // After pushing samples, request rebuild & Redraw
    virtual bool finalize();

    CTime m_startTime     = CTime::min();
    CTime m_endTime       = CTime::min();
    CTime m_chunkDuration = CTime::min();

    uint32_t m_width     = 640;
    uint32_t m_height    = 480;
    uint32_t m_textureID = 0;

    bool m_rotate         = false;
    bool m_isScaleVisible = true;

    typedef struct
    {
        float r = 0, g = 0, b = 0;
    } color_t;

    color_t m_color;

    GtkWidget* m_viewport    = nullptr;
    GtkWidget* m_main        = nullptr;
    GtkWidget* m_top         = nullptr;
    GtkWidget* m_left        = nullptr;
    GtkWidget* m_right       = nullptr;
    GtkWidget* m_bottom      = nullptr;
    GtkWidget* m_cornerLeft  = nullptr;
    GtkWidget* m_cornerRight = nullptr;

    std::string m_title;

    std::vector<AdvancedVisualization::IRenderer*> m_renderers;

    AdvancedVisualization::CRendererContext* m_rendererCtx    = nullptr;
    AdvancedVisualization::CRendererContext* m_subRendererCtx = nullptr;

    AdvancedVisualization::TGtkGLWidget<StreamRendererBase> m_gtkGLWidget;

    AdvancedVisualization::IRuler* m_ruler = nullptr;

    // mouse related
    std::map<int, int> m_buttons;
    int m_mouseX            = 0;
    int m_mouseY            = 0;
    bool m_mouseInitialized = false;
};

/**
 * \class spoolImpl
 * \brief Push chunks in a specified interval to a renderer
 * \details
 *
 * A generic template that is used internally by the classes derived from StreamRendererBase
 * to implement spool(t1,t2) : pushing a [t1,t2] interval of chunks to the renderer.
 *
 * \author J. T. Lindgren
 *
 */
template <typename T, typename TRenderer>
bool spoolImpl(std::shared_ptr<const Stream<T>> stream, TRenderer& renderer, CTime startTime, CTime endTime)
{
    renderer.reset(startTime, endTime);

    typename T::Buffer* buf;

    // Push the visible chunks to renderer
    uint64_t chunksSent = 0;
    for (size_t i = 0; i < stream->getChunkCount(); ++i) {
        if (stream->getChunk(i, &buf) && buf->m_StartTime >= startTime && buf->m_EndTime <= endTime) {
            renderer.push(*buf);
            chunksSent++;
        }
    }

    renderer.finalize();

    return true;
}

// Helper function for showMatrixList(), adds a column to a tree view, @fixme move somewhere else
void add_column(GtkTreeView* treeView, const char* name, uint32_t id, uint32_t minWidth);

/**
 * \class showMatrixList
 * \brief Generic template that can be used to draw lists of any stream that derives from the matrix type.
 * @fixme might refactor somewhere else
 * \author J. T. Lindgren
 *
 */
template <typename T>
bool showMatrixList(std::shared_ptr<const Stream<T>> stream, GtkWidget** listWindow, const char* title)
{
    if (*listWindow) {
        gtk_window_present(GTK_WINDOW(*listWindow));
        return true;
    }

    GtkBuilder* builder    = gtk_builder_new();
    const CString filename = Directories::getDataDir() + "/applications/tracker/tracker.ui";
    if (!gtk_builder_add_from_file(builder, filename, nullptr)) {
        std::cout << "Problem loading [" << filename << "]\n";
        return false;
    }

    *listWindow           = GTK_WIDGET(gtk_builder_get_object(builder, "tracker-stimulation_list"));
    GtkTreeView* treeView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tracker-stimulation_list-treeview"));
    //  GtkListStore* listStore = GTK_LIST_STORE(gtk_builder_get_object(pBuilder, "liststore_select"));
    GtkTreeStore* treeStore = gtk_tree_store_new(4, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_STRING);

    gtk_window_set_title(GTK_WINDOW(*listWindow), title);

    add_column(treeView, "Chunk#", 0, 10);
    add_column(treeView, "ChunkStart (s)", 1, 5);
    add_column(treeView, "ChunkEnd (s)", 2, 5);
    add_column(treeView, "Chunk dims", 3, 10);

    gtk_tree_view_set_model(treeView, GTK_TREE_MODEL(treeStore));

    GtkTreeIter it;
    gtk_tree_store_clear(treeStore);

    //  ::gtk_tree_view_set_model(m_pChannelTreeView, nullptr);
    for (size_t i = 0; i < stream->getChunkCount(); ++i) {
        const typename T::Buffer* ptr = stream->getChunk(i);
        if (!ptr) { break; }

        std::stringstream ss;
        ss << ptr->m_buffer.getDimensionCount() << " : ";
        for (uint32_t j = 0; j < ptr->m_buffer.getDimensionCount(); ++j) { ss << (j > 0 ? " x " : "") << ptr->m_buffer.getDimensionSize(j); }

        gtk_tree_store_append(treeStore, &it, nullptr);
        ::gtk_tree_store_set(treeStore, &it, 0, i, 1, ptr->m_StartTime.toSeconds(), 2, ptr->m_EndTime.toSeconds(), 3, ss.str().c_str(), -1);
    }

    // Hide instead of destroy on closing the window
    g_signal_connect(*listWindow, "delete_event", G_CALLBACK(gtk_widget_hide_on_delete), nullptr);
    gtk_widget_show_all(GTK_WIDGET(*listWindow));
    g_object_unref(builder);

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
