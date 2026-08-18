//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
// @todo connect all the vertical and horizontal sliders and scales
// @todo make horizontal scale related to seconds rather than chunks
// @todo add rulers
// @todo need a much more clear design to handle multiple tracks with multiple streams of different sizes

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>
#include <iomanip>
#include <clocale> // std::setlocale
#include <string.h> // strchr on Ubuntu
#include <algorithm>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <fs/Files.h>
#include <system/ovCMath.h>
#include <system/ovCTime.h>

#include "GUI.h"

#include "CBoxConfigurationDialog.hpp"
#include "CCommentEditorDialog.hpp"
#include "Stream.h"
#include "StreamBundle.h"
#include "StreamRendererFactory.h"

#include "TypeError.h"

namespace OpenViBE {
namespace Tracker {

// These take in a string identidifer as MENUCHOICE to be given to builder. Return FALSE from callbacks so they don't block further signal handlers.
#define GTK_CALLBACK_MAPPER(MENUCHOICE, ACTION, MEMBERFUN) \
    auto MEMBERFUN = [](GtkWidget* pMenuItem, gpointer data) -> gboolean { static_cast<GUI*>(data)->MEMBERFUN(); return FALSE; }; \
    g_signal_connect(G_OBJECT(gtk_builder_get_object(m_interface, MENUCHOICE)), \
        ACTION, G_CALLBACK((gboolean(*)(GtkWidget* pMenuItem, gpointer data)) ( MEMBERFUN ) ), this);
#define GTK_CALLBACK_MAPPER_PARAM(MENUCHOICE, ACTION, MEMBERFUN) \
    auto MEMBERFUN = [](GtkWidget* pMenuItem, gpointer data) -> gboolean { static_cast<GUI*>(data)->MEMBERFUN(pMenuItem); return FALSE; }; \
    g_signal_connect(G_OBJECT(gtk_builder_get_object(m_interface, MENUCHOICE)), \
        ACTION, G_CALLBACK((gboolean(*)(GtkWidget* pMenuItem, gpointer data)) ( MEMBERFUN ) ), this);
// These work on gtk objects directly, used when menus are constructed in code, not in .ui file
#define GTK_CALLBACK_MAPPER_OBJECT(MENUOBJECT, ACTION, MEMBERFUN) \
    auto MEMBERFUN = [](GtkWidget* pMenuItem, gpointer data) -> gboolean { static_cast<GUI*>(data)->MEMBERFUN(); return FALSE; }; \
    g_signal_connect(GTK_OBJECT(MENUOBJECT), ACTION, G_CALLBACK((gboolean(*)(GtkWidget* pMenuItem, gpointer data)) MEMBERFUN), this);
#define GTK_CALLBACK_MAPPER_OBJECT_PARAM(MENUOBJECT, ACTION, MEMBERFUN) \
    auto MEMBERFUN = [](GtkWidget* pMenuItem, gpointer data) -> gboolean { static_cast<GUI*>(data)->MEMBERFUN(pMenuItem); return FALSE; }; \
    g_signal_connect(GTK_OBJECT(MENUOBJECT), ACTION, G_CALLBACK((gboolean(*)(GtkWidget* pMenuItem, gpointer data)) MEMBERFUN), this);

GUI::~GUI()
{
    clearRenderers();

    if (m_logListener) {
        Contexted::getLogManager().removeListener(m_logListener);
        delete m_logListener;
    }
}

GUI::GUI(int argc, char* argv[], CTracker& app) : Contexted(app.getKernelContext()), m_tracker(app)
{
    gtk_init(&argc, &argv);

    // We rely on this with 64bit/gtk 2.24, to roll back gtk_init() sometimes switching
    // the locale to one where ',' is needed instead of '.' for separators of floats, 
    // causing issues, for example getConfigurationManager.expandAsFloat("0.05") -> 0; 
    // due to implementation by std::stod().
    std::setlocale(LC_ALL, "C");

    m_interface            = gtk_builder_new();
    const CString filename = Directories::getDataDir() + "/applications/tracker/tracker.ui";
    if (!gtk_builder_add_from_file(m_interface, filename, nullptr)) {
        std::cout << "Problem loading [" << filename << "]\n";
        return;
    }

    m_mainWindow = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker"));

    // Tricky getting the quit handler properly in this case, note the different CB params.
    auto quitFun = [](GtkWidget* /*menuItem*/, GdkEventKey* /*e*/, gpointer data) { return gboolean(static_cast<GUI*>(data)->quitCB()); };
    typedef gboolean quit_handler_t(GtkWidget* pMenuItem, GdkEventKey* e, gpointer data);
    g_signal_connect(G_OBJECT(m_mainWindow), "delete_event", G_CALLBACK( static_cast<quit_handler_t*>(quitFun)), this);

    m_scrollbar      = GTK_WIDGET(gtk_builder_get_object(m_interface, "hscrollbar1"));
    m_scale          = GTK_WIDGET(gtk_builder_get_object(m_interface, "hscale1"));
    m_timeDisplay    = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-label_current_time"));
    m_trackCounter   = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-label_trackno"));
    m_workspaceLabel = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-workspace_label"));

    gtk_range_set_range(GTK_RANGE(m_scale), 1, 60 * 60); // from 1 sec to 1 hour

    gtk_builder_connect_signals(m_interface, nullptr);

    auto idleFun = [](gpointer data) -> gboolean
    {
        static_cast<GUI*>(data)->step();
        return TRUE;
    };
    g_idle_add(idleFun, this);

    // Register callbacks only after we've done all the initializations

    // Menu callbacks
    GTK_CALLBACK_MAPPER("tracker-menu_open_file", "activate", addTrackCB);
    GTK_CALLBACK_MAPPER("tracker-menu_open_workspace", "activate", openWorkspaceCB);
    GTK_CALLBACK_MAPPER("tracker-menu_open_processor", "activate", openProcessorCBFinal);
    GTK_CALLBACK_MAPPER("tracker-menu_clear", "activate", clearCB);
    GTK_CALLBACK_MAPPER("tracker-menu_save", "activate", saveCB);
    GTK_CALLBACK_MAPPER("tracker-menu_saveas", "activate", saveAsCB);
    GTK_CALLBACK_MAPPER("tracker-menu_incrementrevsave", "activate", incrementRevSaveCB);
    GTK_CALLBACK_MAPPER("tracker-menu_quit", "activate", quitCB);

    GTK_CALLBACK_MAPPER("tracker-menu_select", "activate", editSelectionCB);
    GTK_CALLBACK_MAPPER("tracker-menu_select_all", "activate", selectAllCB);
    GTK_CALLBACK_MAPPER("tracker-menu_select_none", "activate", selectNoneCB);

    GTK_CALLBACK_MAPPER("tracker-menu_processor_preferences", "activate", processorPreferencesCB);
    GTK_CALLBACK_MAPPER("tracker-menu_workspace_notes", "activate", editNotesCB);
    GTK_CALLBACK_MAPPER("tracker-menu_delete_all", "activate", deleteAllTracksCB);

    GTK_CALLBACK_MAPPER("tracker-menu_workspace_about", "activate", workspaceInfoCB);
    GTK_CALLBACK_MAPPER("tracker-menu_about", "activate", aboutCB);

    GTK_CALLBACK_MAPPER_PARAM("hscrollbar1", "value-changed", hScrollCB);
    GTK_CALLBACK_MAPPER_PARAM("hscale1", "value-changed", hScaleCB);

    // Buttons
    GTK_CALLBACK_MAPPER("tracker-button_notes", "clicked", editNotesCB2);
    GTK_CALLBACK_MAPPER("tracker-button_play_pause", "clicked", playCB);
    GTK_CALLBACK_MAPPER("tracker-button_forward", "clicked", playFastCB);
    GTK_CALLBACK_MAPPER("tracker-button_stop", "clicked", stopCB);

    GTK_CALLBACK_MAPPER("tracker-select_tracks", "clicked", editSelectionCB2);
    GTK_CALLBACK_MAPPER("tracker-processor_properties", "clicked", processorEditCB);

    GTK_CALLBACK_MAPPER("tracker-processor_choose", "clicked", openProcessorCB);
    GTK_CALLBACK_MAPPER("tracker-processor_edit2", "clicked", processorEditCB2);
    GTK_CALLBACK_MAPPER("processorpreferences-button_ok", "clicked", processorPreferencesButtonOkCB);
    GTK_CALLBACK_MAPPER("processorpreferences-button_cancel", "clicked", processorPreferencesButtonCancelCB);
    GTK_CALLBACK_MAPPER("trackerpreferences-workspace_path-choose", "clicked", selectWorkspacePathCB);


    // Attach a log listener
    m_logListener = new CLogListenerTracker(Contexted::getKernelContext(), m_interface);
    //  m_pLogListener->m_CenterOnBoxFun = [this](CIdentifier& id) { this->getCurrentInterfacedScenario()->centerOnBox(id); };
    Contexted::getLogManager().addListener(m_logListener);
    GTK_CALLBACK_MAPPER("openvibe-messages_tb_clear", "clicked", clearMessagesCB);
    // Enable the default buttons
    GtkWidget* buttonActiveInfo             = GTK_WIDGET(gtk_builder_get_object(m_interface, "openvibe-messages_tb_info"));
    GtkWidget* buttonActiveWarning          = GTK_WIDGET(gtk_builder_get_object(m_interface, "openvibe-messages_tb_warning"));
    GtkWidget* buttonActiveImportantWarning = GTK_WIDGET(gtk_builder_get_object(m_interface, "openvibe-messages_tb_impwarning"));
    GtkWidget* buttonActiveError            = GTK_WIDGET(gtk_builder_get_object(m_interface, "openvibe-messages_tb_error"));
    GtkWidget* buttonActiveFatal            = GTK_WIDGET(gtk_builder_get_object(m_interface, "openvibe-messages_tb_fatal"));
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(buttonActiveInfo), true);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(buttonActiveWarning), true);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(buttonActiveImportantWarning), true);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(buttonActiveError), true);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(buttonActiveFatal), true);

    std::string title(gtk_window_get_title(GTK_WINDOW(m_mainWindow)));
    title += " v" + std::string(OV_VERSION_MAJOR) + "." + std::string(OV_VERSION_MINOR) + "." + std::string(OV_VERSION_PATCH);
#if defined(TARGET_ARCHITECTURE_x64)
    title += " (64bit)";
#else
    title += " (32bit)";
#endif
    gtk_window_set_title(GTK_WINDOW(m_mainWindow), title.c_str());
    gtk_widget_show(m_mainWindow);
}

bool GUI::clearRenderers()
{
    GtkWidget* trackTable = GTK_WIDGET(gtk_builder_get_object(m_interface, "table_tracks"));

    for (auto trk : m_tracks) {
        // To avoid issues, we delete everything first before detaching the widget
        GtkWidget* frame = trk->m_Frame;

        delete trk;

        if (frame) { gtk_container_remove(GTK_CONTAINER(trackTable), frame); }
    }

    m_tracks.clear();

    return true;
}


bool GUI::initializeRenderers()
{
    clearRenderers();

    const size_t numTracks = m_tracker.getWorkspace().getNumTracks();

    if (numTracks == 0) {
        addTracksToMenu();
        resetWidgetProperties();
        m_requestRedraw = true;
        return true;
    }

    const bool showSelectedOnly     = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowSelectedOnly}", false);
    const uint32_t maxRendererCount = uint32_t(m_kernelCtx.getConfigurationManager().expandAsUInteger("${Tracker_Workspace_GUI_MaxRendererCount}", 32));
    std::stringstream ss;
    ss << maxRendererCount;
    m_kernelCtx.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_GUI_MaxRendererCount", ss.str().c_str()); // If it didn't exist


    GtkWidget* trackTable = GTK_WIDGET(gtk_builder_get_object(m_interface, "table_tracks"));

    gtk_table_resize(GTK_TABLE(trackTable), guint(numTracks), 1);

    uint32_t renderersAllocated = 0;

    for (size_t i = 0; i < numTracks; ++i) {
        const StreamBundle* bundle = m_tracker.getWorkspace().getTrack(i);
        const size_t numStreams    = (bundle ? bundle->getNumStreams() : 1);
        GUITrack* track            = new GUITrack();
        GtkWidget* streamTable     = gtk_table_new(1, 2, false);

        m_tracks.push_back(track);

        if (numStreams == 0) {
            std::cout << "Track " << i + 1 << " has no streams\n";
            continue;
        }

        std::stringstream title;
        title << "Track " << (i + 1) << " : " << (bundle ? bundle->getSource() : "Unknown") << (bundle->getDirtyBit() ? " (unsaved)" : " (on disk)");

        if (!showSelectedOnly || m_tracker.getWorkspace().getSelection().countSelectedStreams(i) > 0) {
            track->m_Frame = gtk_frame_new(title.str().c_str());
            gtk_container_add(GTK_CONTAINER(track->m_Frame), streamTable);
            gtk_table_attach_defaults(GTK_TABLE(trackTable), track->m_Frame, 0, 1, guint(i), guint(i + 1));
            gtk_table_set_row_spacing(GTK_TABLE(trackTable), guint(i), 0);
            gtk_table_resize(GTK_TABLE(streamTable), guint(numStreams), 1);
        }

        for (size_t j = 0; j < numStreams; ++j) {
            StreamPtrConst stream = (bundle ? bundle->getStream(j) : nullptr);
            CIdentifier typeID    = CIdentifier::undefined();

            if (stream) { typeID = stream->getTypeIdentifier(); }
            else { std::cout << "Stream " << j << " is null, using label...\n"; }

            log() << Kernel::LogLevel_Debug << "Building renderer for track " << i + 1 << " stream " << j + 1 << "\n";

            StreamRendererBase* renderer;
            if (showSelectedOnly && !stream->getSelected()) { renderer = StreamRendererFactory::getDummyRenderer(getKernelContext()); }
            else if (m_tracker.getWorkspace().getMemorySaveMode() || renderersAllocated >= maxRendererCount) {
                StreamPtr empty = std::make_shared<Stream<TypeError>>(m_kernelCtx);
                renderer        = StreamRendererFactory::getRenderer(getKernelContext(), empty);
            }
            else {
                renderer = StreamRendererFactory::getRenderer(getKernelContext(), stream);
                renderersAllocated++;
                if (renderersAllocated == maxRendererCount) {
                    log() << Kernel::LogLevel_Info << "Reached maximum " << maxRendererCount << " renderers, later streams will be shown as placeholders.\n";
                    log() << Kernel::LogLevel_Info <<
                            "This is a limitation in the current code and the fact that there is limited number of OpenGL contexts.\n";
                }
            }

            if (renderer && renderer->initialize()) {
                if (stream) {
                    std::stringstream ssTitle;
                    ssTitle << "Stream " << j + 1 << " : "
                            << getTypeManager().getTypeName(typeID).toASCIIString()
                            << ((stream->getOverlapping() || stream->getNoncontinuous()) ? " -- Warning: " : "")
                            << (stream->getOverlapping() ? "Overlapping " : "")
                            << (stream->getNoncontinuous() ? "Noncontinuous " : "")
                            << (renderersAllocated >= maxRendererCount ? " - all renderers in use" : "");

                    renderer->setTitle(ssTitle.str().c_str());
                }
                else { renderer->setTitle("Unknown"); }

                std::stringstream prefix;
                prefix << "Tracker_Workspace_GUI_Renderer" << "_Track_" << std::setw(3) << std::setfill('0') << (i + 1)
                        << "_Stream_" << std::setw(2) << std::setfill('0') << (j + 1);
                renderer->restoreSettings(prefix.str());

                if (renderer->getWidget()) // n.b. 'nothing' renderer does not have a widget
                {
                    gtk_table_attach_defaults(GTK_TABLE(streamTable), renderer->getWidget(), 0, 1, guint(j), guint(j + 1));
                    gtk_widget_unref(renderer->getWidget()); // we kept +1 ref so far to make sure gtk doesnt garbage collect it
                }
                renderer->realize();
            }
            else {
                log() << Kernel::LogLevel_Error << "Error: Failed to initialize renderer for stream of type " << typeID << "\n";
                delete renderer;
                renderer = nullptr;
            }

            track->m_Renderers.push_back(renderer);
        }

        gtk_widget_show(streamTable);
        if (track->m_Frame) { gtk_widget_show(track->m_Frame); }
    }

    if (m_tracker.getWorkspace().getMaxDuration() > CTime::min()) {
        gtk_range_set_range(GTK_RANGE(m_scrollbar), 0, m_tracker.getWorkspace().getMaxDuration().toSeconds());
    }


    updateRulerState();

    addTracksToMenu();

    resetWidgetProperties();

#if defined(TARGET_OS_Windows)
    // @note this block is a way avoid black rectangles appearing on Windows as a side effect of drawing. they do not appear on Linux,
    // so it may be a gtk or at least a platform specific issue. 
    // Basically what this block does is to move the scrollbar to the top, close the expander, and then request them to be
    // reseted to their original positions in a few frames after the content has been drawn. For some reason these actions 'repair' 
    // the drawing on Win (whereas just gtk_widget_queue_draw, gdk_window_invalidate_rect, etc do not seem to cut it).
    // to reproduce the glitch on Windows: Disable this code. Have enough tracks so that some tracks are outside 
    // the view on the upper end of the viewport. Delete a visible track. A black rectangle will be drawn across the buttons.

    GtkWidget* scrollWin  = GTK_WIDGET(gtk_builder_get_object(m_interface, "scrolledwindow-tracks"));
    GtkAdjustment* adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollWin));
    m_hScrollBarValue     = gtk_adjustment_get_value(adjust);
    gtk_adjustment_set_value(adjust, 0);

    GtkWidget* expander = GTK_WIDGET(gtk_builder_get_object(m_interface, "openvibe-expander_messages"));
    m_expanderOpen      = (gtk_expander_get_expanded(GTK_EXPANDER(expander)) == TRUE ? true : false);
    gtk_expander_set_expanded(GTK_EXPANDER(expander), false);

    m_requestResetInNFrames = 2;

    auto resetFun = [](gpointer data) -> gboolean
    {
        GUI* ptr = static_cast<GUI*>(data);
        if (ptr->m_requestResetInNFrames > 0) { ptr->m_requestResetInNFrames--; }
        if (ptr->m_requestResetInNFrames == 0) {
            GtkWidget* expander = GTK_WIDGET(gtk_builder_get_object(ptr->m_interface, "openvibe-expander_messages"));
            gtk_expander_set_expanded(GTK_EXPANDER(expander), ptr->m_expanderOpen);

            GtkWidget* scrollWin  = GTK_WIDGET(gtk_builder_get_object(ptr->m_interface, "scrolledwindow-tracks"));
            GtkAdjustment* adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrollWin));
            gtk_adjustment_set_value(adjust, ptr->m_hScrollBarValue);

            ptr->m_requestResetInNFrames = -1;
            return FALSE; // Don't call again until re-added
        }

        return TRUE;
    };
    g_idle_add(resetFun, this);

#endif

    m_requestRedraw = true;

    return true;
}

bool GUI::redrawAllTracks()
{
    for (size_t i = 0; i < m_tracks.size(); ++i) { redrawTrack(i); }
    m_lastRedraw = System::Time::zgetTime();

    return true;
}

bool GUI::step()
{
    const CTracker::EStates state = m_tracker.step();

    if (state == CTracker::EStates::Playing) {
        const CTime currentTime = CTime(System::Time::zgetTime());
        if (currentTime - m_previousTime > CTime(1.0)) {
            const CTime processedTime = m_tracker.getWorkspace().getProcessedTime();
            const CTime totalTime     = m_tracker.getWorkspace().getPlaylistDuration();

            const double processedTimeSecs = processedTime.toSeconds();
            const double totalTimeSecs     = totalTime.toSeconds();

            char tmp[128];
            sprintf(tmp, "Processed : %.1fm / %.1fm (%.0f%%)", processedTimeSecs / 60.0, totalTimeSecs / 60.0,
                    (totalTime > CTime::min() ? 100.0 * processedTimeSecs / totalTimeSecs : 0));
            gtk_label_set(GTK_LABEL(m_timeDisplay), tmp);

            m_previousTime = currentTime;

            const size_t tracksOnPlaylist = m_tracker.getWorkspace().getNumTracksOnPlaylist();
            const size_t tracksDone       = m_tracker.getWorkspace().getNumTracksDone();

            sprintf(tmp, "Playing tracks (%z until finish)", tracksOnPlaylist - tracksDone);
            gtk_label_set(GTK_LABEL(m_trackCounter), tmp);
        }
    }
    else if (state == CTracker::EStates::Stopped && m_waitingForStop) {
        setPlaytimeWidgetState(true);

        gtk_label_set(GTK_LABEL(m_timeDisplay), "Processed : -");
        gtk_label_set(GTK_LABEL(m_trackCounter), "Stopped");

        initializeRenderers();

        m_waitingForStop = false;
    }
    //else { }  // nop

    if (m_requestRedraw) {
        redrawAllTracks();
        m_requestRedraw = false;
    }

    return true;
}

bool GUI::run()
{
    initializeRenderers();
    gtk_main();
    return true;
}

bool GUI::addTrackCB()
{
    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "OpenScenarioCB\n";

    GtkFileFilter* fileFilterSpecific = gtk_file_filter_new();

    gtk_file_filter_add_pattern(fileFilterSpecific, "*.ov");
    gtk_file_filter_set_name(fileFilterSpecific, "OV files");

    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to open...", nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
                                                              GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);

    // GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(widgetDialogOpen), true);

    const CString defaultPath = Directories::getDataDir() + CString("/scenarios/signals/");

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widgetDialogOpen), defaultPath.toASCIIString());

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widgetDialogOpen), true);

    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        GSList* list;
        GSList* file = list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widgetDialogOpen));
        while (file) {
            char* filename = static_cast<char*>(file->data);
            char* backslash;
            while ((backslash = strchr(filename, '\\')) != nullptr) { *backslash = '/'; }

            m_tracker.getWorkspace().addTrack(filename);

            g_free(file->data);
            file = file->next;
        }
        g_slist_free(list);

        initializeRenderers();
    }

    gtk_widget_destroy(widgetDialogOpen);

    return true;
}

bool GUI::resetWidgetProperties()
{
    static const double DEFAULT_SCALE_SECS = 30.0; // how many seconds of data to view at once per default

    gtk_range_set_value(GTK_RANGE(m_scrollbar), 0);

    if (!m_tracks.empty() && !m_tracker.getWorkspace().getMemorySaveMode()) {
        const CTime maxDuration = m_tracker.getWorkspace().getMaxDuration();
        const double maxSecs    = maxDuration.toSeconds();

        if (maxSecs > 0) {
            gtk_range_set_range(GTK_RANGE(m_scale), 1, maxSecs);
            gtk_range_set_value(GTK_RANGE(m_scale), (maxSecs < DEFAULT_SCALE_SECS ? maxSecs : DEFAULT_SCALE_SECS));
        }
        gtk_widget_set_sensitive(m_scrollbar, true);
        gtk_widget_set_sensitive(m_scale, true);
    }
    else {
        gtk_widget_set_sensitive(m_scrollbar, false);
        gtk_widget_set_sensitive(m_scale, false);
    }

    const CString workspaceFile = m_tracker.getWorkspace().getFilename();
    const uint64_t revision     = m_tracker.getWorkspace().getRevision();

    GtkWidget* saveAsH = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-menu_save"));
    if (workspaceFile.length() > 0) {
        gtk_widget_set_sensitive(saveAsH, true);
        char basename[512];
        FS::Files::getFilename(workspaceFile, basename, 512);
        std::stringstream ss;
        ss << "Workspace: " << basename << " (rev " << revision << ")";
        gtk_label_set_text(GTK_LABEL(m_workspaceLabel), ss.str().c_str());
    }
    else {
        gtk_widget_set_sensitive(saveAsH, false);
        gtk_label_set_text(GTK_LABEL(m_workspaceLabel), "Workspace: Unnamed");
    }

    const CString processorFile = m_tracker.getWorkspace().getProcessorFile();
    GtkWidget* widget           = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_file"));
    char basename[512];
    FS::Files::getFilename(processorFile.toASCIIString(), basename, 512);
    gtk_label_set_text(GTK_LABEL(widget), basename);
    gtk_widget_show(widget);

    updateIdleWidgetState();

#if 0

    m_totalChunks = 0;
    m_numChannels = 0;
    m_chunkSize = 0;

    const StreamBundle* tr = m_rTracker.getWorkspace().getTrack(0);
    if (!tr) {
        return false;
    }
    const Stream<TypeSignal>* ptr = nullptr;

    for (size_t i=0;i<tr->getNumStreams();i++)
    {
        if(tr->getStream(i) && tr->getStream(i)->getTypeIdentifier()==OV_TypeId_Signal)
        {
            ptr = reinterpret_cast< const Stream<TypeSignal>* >(tr->getStream(i));
            break;
        }
    }
    if(ptr)
    {
        // Count samples first
        TypeSignal::Buffer* buf;
        ptr->getChunk(0, &buf);
        if(buf && buf->m_buffer.getDimensionCount()==2) 
        {
            m_totalChunks = ptr->getChunkCount();
            m_numChannels = buf->m_buffer.getDimensionSize(0);
            m_chunkSize = buf->m_buffer.getDimensionSize(1);
        }
    }

    if(m_totalChunks<2) { std::cout << "Warning: File has less than 2 chunks, ranges may behave oddly\n"; }

    const uint32_t chunksPerView = std::min<uint32_t>(20,m_totalChunks-1);

    // @todo change the scrollbars to seconds
    gtk_range_set_range(GTK_RANGE(m_hScale), 1, m_totalChunks-1);
    gtk_range_set_value(GTK_RANGE(m_hScale), chunksPerView);

    gtk_range_set_range(GTK_RANGE(m_hScrollbar), 0, m_rTracker.getWorkspace().getMaxDuration() );
    gtk_range_set_value(GTK_RANGE(m_hScrollbar), 0);

    // gtk_range_set_value(GTK_RANGE(m_hScrollbar), 0);
    // gtk_range_set_round_digits(GTK_RANGE(m_hScrollbar), 0); //not in gtk ver we use on Windows
    gtk_range_set_increments(GTK_RANGE(m_hScrollbar), 10, 100);

#endif

    if (m_selectionWindow && gtk_widget_get_visible(m_selectionWindow)) {
        // Will Redraw the window
        editSelectionCB();
    }

    return true;
}

bool GUI::redrawStream(const size_t trackIndex, const size_t streamIndex)
{
    const StreamBundle* tr = m_tracker.getWorkspace().getTrack(trackIndex);
    if (!tr) {
        std::cout << "No track for index " << trackIndex << "\n";

        return false;
    }

    StreamRendererBase* renderer = m_tracks[trackIndex]->m_Renderers[streamIndex];
    if (!renderer) {
        std::cout << "No renderer for stream " << streamIndex << " in track " << trackIndex << "\n";
        return false;
    }

    const StreamPtrConst stream = tr->getStream(streamIndex);
    if (!stream) {
        std::cout << "No stream " << streamIndex << " in track " << trackIndex << "\n";
        return false;
    }

    const double hValue = double(std::floor(gtk_range_get_value(GTK_RANGE(m_scrollbar))));
    const double hScale = double(std::floor(gtk_range_get_value(GTK_RANGE(m_scale))));

    // std::cout << "HSlider is at " << hValue << " and " << hScale << "\n";

    const CTime startTime = CTime(hValue);
    const CTime endTime   = startTime + CTime(hScale);

    //  std::cout << "Spooling " << trackIndex << " : " << streamIndex << " for " << CTime(startTime).toSeconds() << " to "
    //      << CTime(endTime).toSeconds() << "\n";

    renderer->spool(startTime, endTime);

    return true;
}

bool GUI::redrawTrack(const size_t index)
{
    //  CTime time = System::Time::zgetTime();

    for (size_t streamIndex = 0; streamIndex < m_tracks[index]->m_Renderers.size(); ++streamIndex) { redrawStream(index, streamIndex); }

    return true;
}

bool GUI::openWorkspaceCB()
{
    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "OpenScenarioCB\n";

    GtkFileFilter* fileFilterSpecific = gtk_file_filter_new();

    gtk_file_filter_add_pattern(fileFilterSpecific, "*.ovw");
    gtk_file_filter_set_name(fileFilterSpecific, "Workspace files");

    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to open...", nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
                                                              GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);

    // GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(widgetDialogOpen), true);

    const CString defaultPath = Directories::getUserDataDir() + "/scenarios/workspaces/";

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widgetDialogOpen), defaultPath.toASCIIString());

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widgetDialogOpen), false);

    bool fileSelected = false;
    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        GSList* list;
        GSList* file = list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widgetDialogOpen));
        while (file) {
            char* filename = static_cast<char*>(file->data);
            char* backslash;
            while ((backslash = strchr(filename, '\\')) != nullptr) { *backslash = '/'; }

            m_tracker.getWorkspace().load(CString(filename));

            initializeRenderers();

            fileSelected = true;

            g_free(file->data);
            file = file->next;
        }
        g_slist_free(list);
    }
    else {
        //      GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
        //      gtk_entry_set_text(GTK_ENTRY(widget), "");
    }


    gtk_widget_destroy(widgetDialogOpen);

    return fileSelected;
}


bool GUI::openProcessorCB() const
{
    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "OpenScenarioCB\n";

    GtkFileFilter* fileFilterSpecific = gtk_file_filter_new();

    gtk_file_filter_add_pattern(fileFilterSpecific, "*.xml");
    gtk_file_filter_set_name(fileFilterSpecific, "Scenario files");

    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to open...", nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
                                                              GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);

    // GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(widgetDialogOpen), true);

    const CString defaultPath = Directories::getDataDir() + CString("/applications/tracker/");

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widgetDialogOpen), defaultPath.toASCIIString());
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widgetDialogOpen), false);

    bool fileSelected = false;
    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        GSList* list;
        GSList* file = list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widgetDialogOpen));
        while (file) {
            char* filename = static_cast<char*>(file->data);
            char* backslash;
            while ((backslash = strchr(filename, '\\')) != nullptr) { *backslash = '/'; }

            GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
            gtk_entry_set_text(GTK_ENTRY(widget), filename);

            fileSelected = true;

            g_free(file->data);
            file = file->next;
        }
        g_slist_free(list);
    }
    else {
        GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
        gtk_entry_set_text(GTK_ENTRY(widget), "");
    }

    gtk_widget_destroy(widgetDialogOpen);

    return fileSelected;
}

bool GUI::openProcessorCBFinal()
{
    if (openProcessorCB()) {
        GtkWidget* widget    = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
        const char* filename = gtk_entry_get_text(GTK_ENTRY(widget));

        m_tracker.getWorkspace().setProcessor(filename);

        resetWidgetProperties();
    }
    return true;
}

bool GUI::selectWorkspacePathCBFinal() const
{
    if (selectWorkspacePathCB()) {
        GtkWidget* widget    = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-workspace_path"));
        const char* filename = gtk_entry_get_text(GTK_ENTRY(widget));

        return m_tracker.getWorkspace().setWorkingPath(CString(filename));
    }
    return false;
}
// Note: this doesn't set the path to the workspace as it may be called from Preferences place with "Cancel"
bool GUI::selectWorkspacePathCB() const
{
    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "OpenScenarioCB\n";

    GtkFileFilter* fileFilterSpecific = gtk_file_filter_new();

    //gtk_file_filter_add_pattern(fileFilterSpecific, "*.xml");
    gtk_file_filter_set_name(fileFilterSpecific, "Folders");

    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Please select work path folder ...", nullptr, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);

    // GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(widgetDialogOpen), true);

    const CString defaultPath = Directories::getUserDataDir();

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widgetDialogOpen), defaultPath.toASCIIString());
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widgetDialogOpen), false);

    bool fileSelected = false;
    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        //char* filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgetDialogOpen));
        GSList* list;
        GSList* file = list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widgetDialogOpen));
        while (file) {
            char* filename = static_cast<char*>(file->data);
            char* backslash;
            while ((backslash = strchr(filename, '\\')) != nullptr) { *backslash = '/'; }

            GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-workspace_path"));
            gtk_entry_set_text(GTK_ENTRY(widget), filename);

            fileSelected = true;

            g_free(file->data);
            file = file->next;
        }
        g_slist_free(list);
    }
    else {
        //      GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-workspace_path"));
        //      gtk_entry_set_text(GTK_ENTRY(widget), defaultPath);
    }

    gtk_widget_destroy(widgetDialogOpen);

    return fileSelected;
}

bool GUI::saveAsCB()
{
    if (m_tracker.getWorkspace().getWorkingPath().length() == 0 && !selectWorkspacePathCBFinal()) { return false; }

    //  m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "OpenScenarioCB\n";

    GtkFileFilter* fileFilterSpecific = gtk_file_filter_new();

    gtk_file_filter_add_pattern(fileFilterSpecific, "*.ovw");
    gtk_file_filter_set_name(fileFilterSpecific, "Workspace files");

    GtkWidget* widgetDialogOpen = gtk_file_chooser_dialog_new("Select file to save as...", nullptr, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
                                                              GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widgetDialogOpen), fileFilterSpecific);

    // GTK 2 known bug: this won't work if  setCurrentFolder is also used on the dialog.
    gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(widgetDialogOpen), true);

    const CString defaultPath = Directories::getUserDataDir();

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widgetDialogOpen), defaultPath.toASCIIString());

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(widgetDialogOpen), false);

    bool fileSelected = false;
    if (gtk_dialog_run(GTK_DIALOG(widgetDialogOpen)) == GTK_RESPONSE_ACCEPT) {
        //char* filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widgetDialogOpen));
        GSList* list;
        GSList* file = list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widgetDialogOpen));
        while (file) {
            char* filename = static_cast<char*>(file->data);
            char* backslash;
            while ((backslash = strchr(filename, '\\')) != nullptr) { *backslash = '/'; }

            std::string fn = std::string(filename);
            if (fn.substr(fn.length() - 4, 4) != std::string(".ovw")) { fn += std::string(".ovw"); }

            storeRendererSettings();

            if (m_tracker.getWorkspace().save(fn.c_str())) {
                resetWidgetProperties(); // Filenames may have changed
                initializeRenderers();
                m_requestRedraw = true;

                fileSelected = true;
            }

            g_free(file->data);
            file = file->next;
        }
        g_slist_free(list);
    }
    else { }

    gtk_widget_destroy(widgetDialogOpen);

    return fileSelected;
}

bool GUI::saveCB()
{
    if (m_tracker.getWorkspace().getWorkingPath().length() == 0 && !selectWorkspacePathCBFinal()) { return false; }

    storeRendererSettings();

    m_tracker.getWorkspace().save(m_tracker.getWorkspace().getFilename());

    // filenames may have changed, reset
    resetWidgetProperties();
    initializeRenderers();

    m_requestRedraw = true;
    return true;
}

bool GUI::incrementRevSaveCB()
{
    if (m_tracker.getWorkspace().getWorkingPath().length() == 0 && !selectWorkspacePathCBFinal()) { return false; }

    storeRendererSettings();

    m_tracker.getWorkspace().incrementRevisionAndSave(m_tracker.getWorkspace().getFilename());

    // filenames may have changed, reset
    resetWidgetProperties();
    initializeRenderers();

    m_requestRedraw = true;
    return true;
}

#include <BoxAdapter.h>

bool GUI::quitCB()
{
    this->clearRenderers();
    gtk_main_quit();
    return false;
}

bool GUI::clearCB()
{
    if (m_selectionWindow) {
        gtk_widget_destroy(m_selectionWindow);
        m_selectionWindow = nullptr;
    }

    m_tracker.getWorkspace().clear();

    initializeRenderers();

    return true;
}

bool GUI::deleteAllTracksCB()
{
    m_tracker.getWorkspace().clearTracks();

    initializeRenderers();

    return true;
}

bool GUI::playCB()
{
    m_waitingForStop = true;

    setPlaytimeWidgetState(false);

    if (!m_tracker.play(false)) {
        setPlaytimeWidgetState(true);
        return false;
    }

    return true;
}

bool GUI::playFastCB()
{
    m_waitingForStop = true;

    setPlaytimeWidgetState(false);

    if (!m_tracker.play(true)) {
        setPlaytimeWidgetState(true);
        return false;
    }

    return true;
}


bool GUI::stopCB() const
{
    // Send stop request. The threads may stop later.
    const bool retVal = m_tracker.stop();
    return retVal;
}

bool GUI::editSelectionCB()
{
    if (m_selectionWindow) { gtk_widget_destroy(m_selectionWindow); }

    Workspace& wp        = m_tracker.getWorkspace();
    Selection& selection = m_tracker.getWorkspace().getSelection();

    const size_t nRows = wp.getNumTracks();
    size_t nCols       = 0;
    for (size_t i = 0; i < nRows; ++i) { nCols = std::max<size_t>(nCols, wp.getTrack(i)->getNumStreams()); }

    m_selectionWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_selectionWindow), "Select tracks/streams");

    if (!selection.isEmpty()) {
        GtkWidget* table = gtk_table_new(guint(nRows + 1), guint(nCols + 1), true);
        gtk_table_set_col_spacings(GTK_TABLE(table), 5);

        gtk_container_add(GTK_CONTAINER(m_selectionWindow), table);

        for (size_t i = 0; i < nRows; ++i) {
            for (size_t j = 0; j < wp.getTrack(i)->getNumStreams(); ++j) {
                GtkWidget* button = gtk_check_button_new();
                gtk_table_attach_defaults(GTK_TABLE(table), button, guint(j + 1), guint(j + 2), guint(i + 1), guint(i + 2));

                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), wp.getTrack(i)->getStream(j)->getSelected());

                g_object_set_data(G_OBJECT(button), "setSelection-track", reinterpret_cast<void*>(i));
                g_object_set_data(G_OBJECT(button), "setSelection-stream", reinterpret_cast<void*>(j));
                GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(button), "clicked", setSelectionCB);

                gtk_widget_show(button);
            }

            std::stringstream ss;
            ss << "Track" << i + 1;
            GtkWidget* label = gtk_label_new(ss.str().c_str());
            gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, guint(i + 1), guint(i + 2));
            gtk_widget_show(label);
        }
        for (size_t i = 0; i < nCols; ++i) {
            std::stringstream ss;
            ss << "Stream" << i + 1;
            GtkWidget* label = gtk_label_new(ss.str().c_str());
            gtk_table_attach_defaults(GTK_TABLE(table), label, guint(i + 1), guint(i + 2), 0, 1);
            gtk_widget_show(label);
        }

        gtk_widget_show(table);
    }
    else {
        GtkWidget* label = GTK_WIDGET(gtk_label_new("No tracks with streams"));
        gtk_container_add(GTK_CONTAINER(m_selectionWindow), label);
        gtk_widget_show(label);
    }

    // Hide instead of destroy on closing the window
    g_signal_connect(m_selectionWindow, "delete_event", G_CALLBACK(gtk_widget_hide_on_delete), nullptr);

    gtk_widget_show(m_selectionWindow);

    return true;
}

bool GUI::selectAllCB()
{
    m_tracker.getWorkspace().getSelection().reset(true);
    if (m_selectionWindow && gtk_widget_get_visible(m_selectionWindow)) { editSelectionCB(); }
    updateIdleWidgetState();

    const bool selectedOnlyState = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowSelectedOnly}", false);
    if (selectedOnlyState) { initializeRenderers(); }

    return true;
}

bool GUI::selectNoneCB()
{
    m_tracker.getWorkspace().getSelection().reset(false);
    if (m_selectionWindow && gtk_widget_get_visible(m_selectionWindow)) { editSelectionCB(); }
    updateIdleWidgetState();

    const bool selectedOnlyState = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowSelectedOnly}", false);
    if (selectedOnlyState) { initializeRenderers(); }

    return true;
}


bool GUI::processorPreferencesCB() const
{
    uint32_t sendPort, receivePort;
    m_tracker.getWorkspace().getProcessorPorts(sendPort, receivePort);

    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
    gtk_entry_set_text(GTK_ENTRY(widget), m_tracker.getWorkspace().getProcessorFile());

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_port"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), sendPort);

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-numthreads"));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), m_tracker.getNumThreads());

    bool noGUI, doSend, doReceive;
    m_tracker.getWorkspace().getProcessorFlags(noGUI, doSend, doReceive);
    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_dosend"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), doSend);
    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_doreceive"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), doReceive);
    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_nogui"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), noGUI);

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_catenate"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), m_tracker.getWorkspace().getCatenateMode());


    widget                      = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-workspace_path"));
    const CString workspacePath = m_tracker.getWorkspace().getWorkingPath();
    gtk_entry_set_text(GTK_ENTRY(widget), workspacePath.toASCIIString());

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-memorysave"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), m_tracker.getWorkspace().getMemorySaveMode());

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-inplacemode"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), m_tracker.getWorkspace().getInplaceMode());

    widget             = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-processor_arguments"));
    const CString args = m_tracker.getWorkspace().getProcessorArguments();
    gtk_entry_set_text(GTK_ENTRY(widget), args.toASCIIString());

    GtkDialog* dialog = GTK_DIALOG(gtk_builder_get_object(m_interface, "dialog-processor-preferences"));
    gtk_dialog_run(GTK_DIALOG(dialog));

    return true;
}


bool GUI::processorPreferencesButtonOkCB()
{
    GtkWidget* widget    = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
    const char* filename = gtk_entry_get_text(GTK_ENTRY(widget));
    m_tracker.getWorkspace().setProcessor(filename);

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_port"));
    gtk_spin_button_update(GTK_SPIN_BUTTON(widget));
    const uint32_t firstPort = uint32_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
    m_tracker.getWorkspace().setProcessorPorts(firstPort, firstPort + 1);

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-numthreads"));
    gtk_spin_button_update(GTK_SPIN_BUTTON(widget));
    const uint32_t numThreads = uint32_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
    m_tracker.setNumThreads(numThreads);

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_catenate"));
    m_tracker.getWorkspace().setCatenateMode((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0));

    widget               = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_nogui"));
    const bool noGui     = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0);
    widget               = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_dosend"));
    const bool doSend    = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0);
    widget               = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-processor_doreceive"));
    const bool doReceive = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0);

    m_tracker.getWorkspace().setProcessorFlags(noGui, doSend, doReceive);

    widget           = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-workspace_path"));
    const char* path = gtk_entry_get_text(GTK_ENTRY(widget));
    m_tracker.getWorkspace().setWorkingPath(CString(path));

    widget                       = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-memorysave"));
    const bool oldMemorySaveMode = m_tracker.getWorkspace().getMemorySaveMode();
    const bool newMemorySaveMode = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0);
    if (newMemorySaveMode != oldMemorySaveMode) {
        m_tracker.getWorkspace().setMemorySaveMode(newMemorySaveMode);
        initializeRenderers();
    }

    widget = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-inplacemode"));
    m_tracker.getWorkspace().setInplaceMode((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) != 0));

    widget           = GTK_WIDGET(gtk_builder_get_object(m_interface, "trackerpreferences-processor_arguments"));
    const char* args = gtk_entry_get_text(GTK_ENTRY(widget));
    m_tracker.getWorkspace().setProcessorArguments(CString(args));

    GtkDialog* dialog = GTK_DIALOG(gtk_builder_get_object(m_interface, "dialog-processor-preferences"));
    gtk_widget_hide(GTK_WIDGET(dialog));

    resetWidgetProperties();

    return true;
}


bool GUI::processorPreferencesButtonCancelCB() const
{
    GtkDialog* dialog = GTK_DIALOG(gtk_builder_get_object(m_interface, "dialog-processor-preferences"));
    gtk_widget_hide(GTK_WIDGET(dialog));

    return true;
}


bool GUI::processorEditCB() const
{
    // Here we configure the processor which already has an .xml set
    return m_tracker.getWorkspace().configureProcessor(nullptr);
}

bool GUI::processorEditCB2() const
{
    // This tries to configure whatever the user has entered into the box, even if
    // Ok button hasn't been pressed yet to register that choice.
    GtkWidget* widget    = GTK_WIDGET(gtk_builder_get_object(m_interface, "processorpreferences-filename"));
    const char* filename = gtk_entry_get_text(GTK_ENTRY(widget));
    return m_tracker.getWorkspace().configureProcessor(filename);
}

#include "CCommentEditorDialog.hpp"

bool GUI::editNotesCB() const
{
    const CString guiFile = Directories::getDataDir() + "/applications/tracker/designer-interface.ui";
    Designer::CCommentEditorDialog dialog(getKernelContext(), m_tracker.getWorkspace().getNotes(), guiFile.toASCIIString());
    return dialog.Run();
}


bool GUI::hScrollCB(GtkWidget* /*widget*/)
{
    //  std::cout << "Hscroll" << gtk_range_get_value(GTK_RANGE(widget)) << "\n";
    m_requestRedraw = true;
    return true;
}

bool GUI::hScaleCB(GtkWidget* /*widget*/)
{
    // @since we store the pointers in the class, no need to pass in widget...
    //std::cout << "Hscale " << gtk_range_get_value(GTK_RANGE(widget)) << "\n";
    //  double stepSizeSecs = gtk_range_get_value(GTK_RANGE(widget));
    //  CTime maxDuration = m_rTracker.getWorkspace().getMaxDuration();
    //  uint64_t verticalStepsAvailable = CTime(stepSizeSecs).time();
    //  const uint32_t chunksPerView = uint32_t( gtk_range_get_value(GTK_RANGE(widget)));

    m_requestRedraw = true;
    return true;
}

bool GUI::workspaceInfoCB() const
{
    GtkWidget* window     = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-workspace_information"));
    GtkTextBuffer* buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(m_interface, "tracker-textbuffer-workspace_information"));

    // Hide instead of destroy on closing the window
    g_signal_connect(window, "delete_event", G_CALLBACK(gtk_widget_hide_on_delete), nullptr);

    const CString workspaceInfo = getWorkspaceInfo();

    gtk_text_buffer_set_text(buffer, workspaceInfo.toASCIIString(), -1);
    gtk_widget_show_all(window);
    gtk_window_present(GTK_WINDOW(window));

    return true;
}

bool GUI::aboutCB() const
{
    GtkWidget* window = GTK_WIDGET(gtk_builder_get_object(m_interface, "aboutdialog-newversion"));
    gtk_dialog_set_response_sensitive(GTK_DIALOG(window), GTK_RESPONSE_CLOSE, true);
    g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(gtk_widget_hide), nullptr);
    gtk_dialog_run(GTK_DIALOG(window));
    return true;
}

bool GUI::deleteTrackCB(GtkWidget* widget)
{
    const size_t index = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "delete-track-cb"));
    if (index >= m_tracks.size()) { return false; }


    GtkWidget* trackTable = GTK_WIDGET(gtk_builder_get_object(m_interface, "table_tracks"));

    // To avoid issues, we delete everything first before detaching the widget
    GtkWidget* frame = m_tracks[index]->m_Frame;

    delete m_tracks[index];
    m_tracks.erase(m_tracks.begin() + index);

    if (frame) { gtk_container_remove(GTK_CONTAINER(trackTable), frame); }

    m_tracker.getWorkspace().removeTrack(index);


    // This will store the current renderer configurations but with the new indexing
    storeRendererSettings();
    initializeRenderers();
    return true;
}

bool GUI::moveTrackCB(GtkWidget* widget)
{
    const size_t track = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "move-track-cb"));
    if (track >= m_tracks.size()) { return false; }
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Choose target slot", GTK_WINDOW(m_mainWindow),
                                                    GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, nullptr);

    GtkAdjustment* adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(gdouble(track+1), 1, gdouble(m_tracker.getWorkspace().getNumTracks()), 1, 1, 0));
    GtkWidget* button         = gtk_spin_button_new(adjustment, 1, 0);
    // gtk_spin_button_set_range(GTK_SPIN_BUTTON(button), 1, m_rTracker.getWorkspace().getNumTracks());

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), button);
    gtk_widget_show(button);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(button));
        const size_t newTrack = size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(button))) - 1;

        // @fixme scalings will be lost
        m_tracker.getWorkspace().moveTrack(track, newTrack);

        // This will store the current renderer configurations but with the new indexing
        // storeRendererSettings();

        initializeRenderers();
    }

    gtk_widget_destroy(dialog);

    return true;
}

bool GUI::moveStreamCB(GtkWidget* widget)
{
    const size_t track  = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "move-stream-cb-track"));
    const size_t stream = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "move-stream-cb-stream"));

    GtkWidget* dialog = gtk_dialog_new_with_buttons("Choose track & stream", GTK_WINDOW(m_mainWindow),
                                                    GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                                    GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, nullptr);

    // Target track
    GtkAdjustment* adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(gdouble(track+1), 1, gdouble(m_tracker.getWorkspace().getNumTracks()), 1, 1, 0));
    GtkWidget* buttonTrack    = gtk_spin_button_new(adjustment, 1, 0);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), buttonTrack);
    gtk_widget_show(buttonTrack);

    // Target stream
    // @fixme the limits would depend on the target track
    size_t maxNumStreams = 0;
    for (size_t i = 0; i < m_tracker.getWorkspace().getNumTracks(); ++i) {
        maxNumStreams = std::max<size_t>(maxNumStreams, m_tracker.getWorkspace().getTrack(i)->getNumStreams());
    }
    adjustment              = GTK_ADJUSTMENT(gtk_adjustment_new(gdouble(stream+1), 1, gdouble(maxNumStreams), 1, 1, 0));
    GtkWidget* buttonStream = gtk_spin_button_new(adjustment, 1, 0);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), buttonStream);
    gtk_widget_show(buttonStream);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(buttonTrack));
        const size_t newTrack = size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(buttonTrack))) - 1;

        gtk_spin_button_update(GTK_SPIN_BUTTON(buttonStream));
        size_t newStream = size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(buttonStream))) - 1;

        newStream = std::min<size_t>(newStream, m_tracker.getWorkspace().getTrack(newTrack)->getNumStreams());

        // @fixme scalings will be lost
        m_tracker.getWorkspace().moveStream(track, stream, newTrack, newStream);

        // This will store the current renderer configurations but with the new indexing
        // storeRendererSettings();

        initializeRenderers();
    }

    gtk_widget_destroy(dialog);

    return true;
}


bool GUI::showChunksCB(GtkWidget* widget)
{
    void* ptr                    = g_object_get_data(G_OBJECT(widget), "show-chunks-cb");
    StreamRendererBase* renderer = reinterpret_cast<StreamRendererBase*>(ptr);
    if (!renderer) {
        std::cout << "Error: No renderer for stream\n";
        return false;
    }
    return renderer->showChunkList();
}

bool GUI::deleteStreamCB(GtkWidget* widget)
{
    const size_t track  = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "delete-stream-cb-track"));
    const size_t stream = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "delete-stream-cb-stream"));

    // We must make sure the renderer is not holding refs anymore to the track
    // @todo it'd be more efficient just to delete the right renderer and not all of them
    m_tracker.getWorkspace().removeStream(track, stream);

    initializeRenderers();

    return true;
}

bool GUI::toggleRulerCB(GtkWidget* widget)
{
    const bool newState = (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)) != 0 ? true : false);
    m_kernelCtx.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_GUI_ShowRulers", (newState ? "true" : "false"));
    updateRulerState();
    return true;
}

bool GUI::toggleShowSelectedOnlyCB(GtkWidget* widget)
{
    const bool newState = (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)) != 0 ? true : false);
    m_kernelCtx.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_GUI_ShowSelectedOnly", (newState ? "true" : "false"));
    initializeRenderers();

    return true;
}

#include "CBoxConfigurationDialog.hpp"

bool GUI::applyBoxPluginCB(GtkWidget* widget)
{
    // Which filter to apply?
    const size_t index = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "applyBoxPlugin-cb"));
    // Get the box so we can present the configuration GUI
    auto& plugins         = m_tracker.getBoxPlugins().getBoxPlugins();
    BoxAdapterStream* box = plugins[index];

    // The .ui files used are taken by CMake from Designer, like the dialog code
    const CString defaultPath      = Directories::getDataDir() + CString("/applications/tracker/");
    const CString guiFilename      = defaultPath + "designer-interface.ui";
    const CString settingsFilename = defaultPath + "designer-interface-settings.ui";

    bool applyClicked;
    if (box->getBox().getSettingCount() > 0) {
        Designer::CBoxConfigurationDialog dialog(getKernelContext(), box->getBox(), guiFilename, settingsFilename, false);
        applyClicked = dialog.Run();
    }
    else {
        // if theres no settings, just consider this as auto-applying
        applyClicked = true;
    }

    if (applyClicked) {
        m_tracker.applyBoxPlugin(index);
        initializeRenderers();
        m_requestRedraw = true;
    }

    return true;
}

bool GUI::trackerPluginCB(GtkWidget* widget)
{
    // Which filter to apply?
    const size_t index = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "trackerPlugin-cb"));
    const bool success = m_tracker.applyTrackerPlugin(index);

    initializeRenderers();
    m_requestRedraw = true;

    return success;
}


bool GUI::setSelectionCB(GtkWidget* widget)
{
    const size_t track  = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "setSelection-track"));
    const size_t stream = reinterpret_cast<size_t>(g_object_get_data(G_OBJECT(widget), "setSelection-stream"));

    const gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    m_tracker.getWorkspace().getTrack(track)->getStream(stream)->setSelected(active != 0);

    updateIdleWidgetState();

    const bool selectedOnlyState = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowSelectedOnly}", false);
    if (selectedOnlyState) { initializeRenderers(); }

    return true;
}

bool GUI::clearMessagesCB() const
{
    m_logListener->clearMessages();
    return true;
}

GUI::GUITrack::~GUITrack()
{
    for (auto renderer : m_Renderers) {
        if (renderer) {
            renderer->uninitialize();
            delete renderer;
        }
    }
    m_Renderers.clear();
}

// @todo refactor to Workspace. but can't do that neatly while renderers are responsible for the text.
CString GUI::getWorkspaceInfo() const
{
    std::stringstream ss;
    Workspace& wp = m_tracker.getWorkspace();

    ss << "Current workspace" << std::endl;
    ss << "-----------------" << std::endl;
    ss << "Number of tracks: " << wp.getNumTracks() << std::endl;
    ss << "Maximum track duration: " << wp.getMaxDuration().toSeconds() << "s" << std::endl;
    ss << "Working path: " << wp.getWorkingPath() << std::endl;
    ss << "Workspace file: " << wp.getFilename() << std::endl;
    ss << "Workspace revision: " << wp.getRevision() << std::endl;

    if (wp.getMemorySaveMode()) { ss << std::endl << "Note: Memory save mode enabled, track details unknown." << std::endl; }

    for (size_t i = 0; i < wp.getNumTracks(); ++i) {
        const StreamBundle* bundle = wp.getTrack(i);
        if (!bundle) {
            ss << std::endl;
            ss << "Track " << i + 1 << " is empty\n";
            continue;
        }

        ss << std::endl;
        ss << "Track " << i + 1 << "/" << wp.getNumTracks() << " : " << bundle->getNumStreams() << " streams" << std::endl;
        ss << "  Source: " << bundle->getSource() << std::endl;
        for (size_t j = 0; j < m_tracks[i]->m_Renderers.size(); ++j) {
            const StreamPtrConst stream = bundle->getStream(j);
            const CIdentifier typeID    = stream->getTypeIdentifier();

            ss << "  Stream " << j + 1 << "/" << bundle->getNumStreams() << " : " << getTypeManager().getTypeName(typeID).toASCIIString() << " "
                    << typeID.str() << std::endl;
            ss << "    Time range: [" << stream->getStartTime().toSeconds() << ", " << stream->getDuration().toSeconds() << "]s" << std::endl;
            ss << "    Total chunks: " << stream->getChunkCount() << " ( " << (stream->getOverlapping() ? "Overlapping " : "")
                    << (stream->getNoncontinuous() ? "Noncontinuous" : "") << (!(stream->getOverlapping() || stream->getNoncontinuous()) ? "Continuous" : "")
                    << " )" << std::endl;
            ss << "    Selected: " << (stream->getSelected() ? "Yes" : "No") << std::endl;

            // @fixme not too happy that the renderer is responsible for this, but since Stream is a template
            // class, it'd need a override for each type
            if (m_tracks[i]->m_Renderers[j]) { ss << m_tracks[i]->m_Renderers[j]->renderAsText(4); }
            else { ss << "    Empty stream" << std::endl; }
        }
    }

    ss << "\nConfiguration tokens (refreshed on save): \n\n";

    const auto tokens = m_tracker.getWorkspace().getConfigurationTokens();
    for (const auto& token : tokens) { ss << token.first << " = " << token.second << "\n"; }

    return ss.str().c_str();
}

bool GUI::addTracksToMenu()
{
    GtkWidget* menuRoot = GTK_WIDGET(gtk_builder_get_object(m_interface, "tracker-menu_tracks"));

    // Instead of carefully making sure that the menu is consistent, just rebuild the whole thing
    GtkWidget* menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menuRoot));
    if (menu) { gtk_widget_destroy(menu); }

    const bool memorySaveMode = m_tracker.getWorkspace().getMemorySaveMode();

    m_trackMenuWidgets.clear();

    menu = gtk_menu_new();

    bool rulerState     = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowRulers}", true);
    GtkWidget* menuItem = gtk_check_menu_item_new_with_label("Show rulers");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuItem), rulerState);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
    GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(menuItem), "activate", toggleRulerCB);
    gtk_widget_set_sensitive(menuItem, !memorySaveMode);
    // m_TrackMenuWidgets.push_back(menuItem);

    bool selectedOnlyState = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowSelectedOnly}", false);
    menuItem               = gtk_check_menu_item_new_with_label("Show selected only");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuItem), selectedOnlyState);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
    GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(menuItem), "activate", toggleShowSelectedOnlyCB);

    GtkWidget* separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

    /// Box plugins menu
    menuItem = gtk_menu_item_new_with_label("Apply box plugin");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
    m_trackMenuWidgets.push_back(menuItem);

    GtkWidget* subMenu = gtk_menu_new();
    m_trackMenuWidgets.push_back(subMenu);

    auto& plugins = m_tracker.getBoxPlugins().getBoxPlugins();
    for (size_t i = 0; i < plugins.size(); ++i) {
        const CString name = plugins[i]->getBox().getName();

        GtkWidget* subMenuItem = gtk_menu_item_new_with_label(name.toASCIIString());
        gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), subMenuItem);
        g_object_set_data(G_OBJECT(subMenuItem), "applyBoxPlugin-cb", reinterpret_cast<void*>(i));
        GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(subMenuItem), "activate", applyBoxPluginCB);
        m_trackMenuWidgets.push_back(subMenuItem);
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), subMenu);

    gtk_widget_show(menuItem);
    gtk_widget_show(subMenu);

    //// Tracker plugins menu
    menuItem = gtk_menu_item_new_with_label("Apply Tracker plugin");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);
    m_trackMenuWidgets.push_back(menuItem);

    subMenu = gtk_menu_new();
    m_trackMenuWidgets.push_back(subMenu);

    auto& trackerPlugins = m_tracker.getTrackerPlugins().getTrackerPlugins();
    for (size_t i = 0; i < trackerPlugins.size(); ++i) {
        const std::string name = trackerPlugins[i]->getName();

        GtkWidget* subMenuItem = gtk_menu_item_new_with_label(name.c_str());
        gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), subMenuItem);
        g_object_set_data(G_OBJECT(subMenuItem), "trackerPlugin-cb", reinterpret_cast<void*>(i));
        GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(subMenuItem), "activate", trackerPluginCB);
        m_trackMenuWidgets.push_back(subMenuItem);
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), subMenu);

    gtk_widget_show(menuItem);
    gtk_widget_show(subMenu);

    //// Per track

    separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

    {
        std::unique_lock<std::mutex>(m_tracker.getWorkspace().getMutex());
        const size_t numTracks = m_tracker.getWorkspace().getNumTracks();

        for (size_t i = 0; i < numTracks; ++i) {
            std::stringstream ss;
            ss << "Track " << i + 1;
            menuItem = gtk_menu_item_new_with_label(ss.str().c_str());
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuItem);

            subMenu = gtk_menu_new();

            GtkWidget* subMenuItem = gtk_menu_item_new_with_label("Delete Track");
            gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), subMenuItem);
            g_object_set_data(G_OBJECT(subMenuItem), "delete-track-cb", reinterpret_cast<void*>(i));
            GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(subMenuItem), "activate", deleteTrackCB);

            subMenuItem = gtk_menu_item_new_with_label("Move Track");
            gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), subMenuItem);
            g_object_set_data(G_OBJECT(subMenuItem), "move-track-cb", reinterpret_cast<void*>(i));
            GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(subMenuItem), "activate", moveTrackCB);

            separator = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), separator);

            const StreamBundle* tr = m_tracker.getWorkspace().getTrack(i);
            for (size_t j = 0; tr && j < tr->getNumStreams(); ++j) {
                std::stringstream ss2;
                ss2 << "Stream " << j + 1;
                subMenuItem = gtk_menu_item_new_with_label(ss2.str().c_str());
                gtk_menu_shell_append(GTK_MENU_SHELL(subMenu), subMenuItem);

                GtkWidget* streamMenuItems = gtk_menu_new();


                GtkWidget* tmp = gtk_menu_item_new_with_label("Show structure");
                gtk_menu_shell_append(GTK_MENU_SHELL(streamMenuItems), tmp);
                g_object_set_data(G_OBJECT(tmp), "show-chunks-cb", reinterpret_cast<void*>(m_tracks[i]->m_Renderers[j]));
                GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(tmp), "activate", showChunksCB);
                gtk_widget_set_sensitive(tmp, !memorySaveMode);

                tmp = gtk_menu_item_new_with_label("Move stream");
                gtk_menu_shell_append(GTK_MENU_SHELL(streamMenuItems), tmp);
                g_object_set_data(G_OBJECT(tmp), "move-stream-cb-track", reinterpret_cast<void*>(i));
                g_object_set_data(G_OBJECT(tmp), "move-stream-cb-stream", reinterpret_cast<void*>(j));
                GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(tmp), "activate", moveStreamCB);
                gtk_widget_set_sensitive(tmp, !memorySaveMode);

                tmp = gtk_menu_item_new_with_label("Delete stream");
                gtk_menu_shell_append(GTK_MENU_SHELL(streamMenuItems), tmp);
                g_object_set_data(G_OBJECT(tmp), "delete-stream-cb-track", reinterpret_cast<void*>(i));
                g_object_set_data(G_OBJECT(tmp), "delete-stream-cb-stream", reinterpret_cast<void*>(j));
                GTK_CALLBACK_MAPPER_OBJECT_PARAM(GTK_OBJECT(tmp), "activate", deleteStreamCB);
                gtk_widget_set_sensitive(tmp, !memorySaveMode);

                gtk_menu_item_set_submenu(GTK_MENU_ITEM(subMenuItem), streamMenuItems);

                gtk_widget_show(subMenuItem);
            }
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItem), subMenu);

            gtk_widget_show(menuItem);
            gtk_widget_show(subMenu);
        }
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuRoot), menu);

    gtk_widget_show_all(menu);

    return true;
}

bool GUI::setPlaytimeWidgetState(const bool enabled)
{
    // Before enabling any of these during play, test thoroughly that it works during it
    const std::vector<std::string> widgets{
        "tracker-button_play_pause", "tracker-button_forward", "tracker-menu_open_file", "tracker-menu_open_workspace", "tracker-menu_open_processor",
        "tracker-menu_clear", "tracker-menu_save", "tracker-menu_saveas", "tracker-menu_incrementrevsave", "tracker-menu_select", "tracker-menu_select_all",
        "tracker-menu_select_none", "tracker-menu_processor_preferences", "tracker-menu_delete_all", "tracker-select_tracks", "tracker-processor_properties",
        "tracker-menu_tracks", "tracker-menu_workspace_about"
    };

    for (const auto& name : widgets) {
        GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, name.c_str()));
        gtk_widget_set_sensitive(widget, enabled);
    }

    for (auto widget : m_trackMenuWidgets) { gtk_widget_set_sensitive(widget, enabled); }

    return true;
}

bool GUI::updateIdleWidgetState()
{
    const std::vector<std::string> widgetsNeedingStreams{
        "tracker-menu_select", "tracker-select_tracks", "tracker-menu_select_all", "tracker-menu_select_none", "tracker-menu_delete_all", "tracker-menu_tracks"
    };
    // n.b. actually the 'apply' widgets might be the only ones needing
    // selection, we must allow 'play' on empty for situations where the user only wants to record
    const std::vector<std::string> widgetsNeedingSelection;
    /*
    const std::vector<std::string> widgetsNeedingSelection{
        "tracker-button_play_pause"
        , "tracker-button_forward"
        , "tracker-button_stop"
    };
    */

    const bool haveStreams = !m_tracker.getWorkspace().getSelection().isEmpty();

    for (const auto& name : widgetsNeedingStreams) {
        GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, name.c_str()));
        if (!widget) {
            log() << Kernel::LogLevel_Error << "Error getting widget " << name << "\n";
            continue;
        }

        gtk_widget_set_sensitive(widget, haveStreams);
    }

    const bool haveSelection = m_tracker.getWorkspace().getSelection().isSomethingSelected();

    for (const auto& name : widgetsNeedingSelection) {
        GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, name.c_str()));
        if (!widget) {
            log() << Kernel::LogLevel_Error << "Error getting widget " << name << "\n";
            continue;
        }

        gtk_widget_set_sensitive(widget, haveSelection);
    }

    for (auto widget : m_trackMenuWidgets) { gtk_widget_set_sensitive(widget, haveSelection); }

    // some widgets need a processor
    const bool hasProcessor = m_tracker.getWorkspace().hasProcessor();
    const std::vector<std::string> widgetsNeedingProcessor{
        "tracker-button_play_pause", "tracker-button_stop", "tracker-button_forward", "tracker-processor_properties"
    };
    for (const auto& name : widgetsNeedingProcessor) {
        GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_interface, name.c_str()));

        gtk_widget_set_sensitive(widget, hasProcessor);
    }

    return true;
}

bool GUI::storeRendererSettings()
{
    bool retVal = true;

    // get rid of possibly dangling tokens from deleted streams/tracks
    m_tracker.getWorkspace().wipeConfigurationTokens("Tracker_Workspace_GUI_Renderer_");

    // Store current ones
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (!m_tracks[i]) { continue; }

        for (size_t j = 0; j < m_tracks[i]->m_Renderers.size(); ++j) {
            std::stringstream ss;
            ss << "Tracker_Workspace_GUI_Renderer" << "_Track_" << std::setw(3) << std::setfill('0') << (i + 1)
                    << "_Stream_" << std::setw(2) << std::setfill('0') << (j + 1);
            retVal &= m_tracks[i]->m_Renderers[j]->storeSettings(ss.str());
        }
    }

    return retVal;
}

bool GUI::updateRulerState()
{
    const bool rulerState = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Tracker_Workspace_GUI_ShowRulers}", true);
    for (auto* trk : m_tracks) { for (auto* renderer : trk->m_Renderers) { if (renderer) { renderer->setRulerVisibility(rulerState); } } }
    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
