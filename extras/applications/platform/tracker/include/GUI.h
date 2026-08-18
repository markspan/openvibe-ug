#pragma once

#include "CTracker.h"
#include "Workspace.h"

#include "Stream.h"
#include "TypeSignal.h"
#include "TypeSpectrum.h"
#include "TypeStimulation.h"

#include "Contexted.h"

#include "CLogListenerTracker.h"

// Forward declare
struct _GtkBuilder;
typedef struct _GtkWidget GtkWidget;
// typedef struct _GtkTable    GtkTable;

namespace OpenViBE {
namespace Tracker {
class StreamRendererBase;

/**
 * \class GUI 
 * \brief The main GUI code for the OpenViBE Tracker. 
 * \author J. T. Lindgren
 *
 * \details
 * This file handles most of the GTK related GUI of the Tracker, with the actual functions modifying the data trying to be as much as possible 
 * implemented in the non-GUI classes. Although the GUI is currently quite monolithic, the rendering of the different Stream types has been
 * delegated to StreamRenderer* classes, which in turn use the Mensia Advanced Visualizations toolkit. Code from Designer is also used by 
 * making CMake include it.
 *
 */
class GUI final : protected Contexted
{
public:
    GUI(int argc, char* argv[], CTracker& app);
    ~GUI() override;

    bool run();

    /**
     * \class GUITrack
     * \brief Holds the renderers for each stream of a Track, and the correspondig GTK table for the widgets.
     * \author J. T. Lindgren
     */
    class GUITrack
    {
    public:
        GUITrack() { }
        ~GUITrack();

        GtkWidget* m_Frame = nullptr;

        // Renderer per stream
        std::vector<StreamRendererBase*> m_Renderers;
    };

protected:
    // The main loop to do work, e.g. feed the tracks to processor
    bool step();

    CString getWorkspaceInfo() const;

    bool addTracksToMenu();

    // Enable/disable certain widgets during play (processing)
    bool setPlaytimeWidgetState(bool enabled);
    // Enable/disable certain widgets depending on the current state (streams, processors, etc)
    bool updateIdleWidgetState();

    bool storeRendererSettings();

    // Callbacks
    bool addTrackCB();
    bool openWorkspaceCB();
    bool openProcessorCB() const;
    bool openProcessorCBFinal();
    bool saveAsCB();
    bool saveCB();
    bool incrementRevSaveCB();
    bool clearCB();
    bool quitCB();
    bool stopCB() const;
    bool playCB();
    bool playFastCB();
    bool workspaceInfoCB() const;
    bool aboutCB() const;

    bool selectWorkspacePathCB() const;
    bool selectWorkspacePathCBFinal() const;

    bool editSelectionCB();
    bool editSelectionCB2() { return editSelectionCB(); }
    bool selectAllCB();
    bool selectNoneCB();

    bool processorPreferencesCB() const;
    bool processorPreferencesButtonOkCB();
    bool processorPreferencesButtonCancelCB() const;
    bool processorEditCB() const;
    bool processorEditCB2() const;

    bool editNotesCB() const;
    bool editNotesCB2() const { return editNotesCB(); }

    bool hScrollCB(GtkWidget* widget);
    bool hScaleCB(GtkWidget* widget);

    bool deleteAllTracksCB();
    bool deleteTrackCB(GtkWidget* widget);
    bool moveTrackCB(GtkWidget* widget);
    bool toggleRulerCB(GtkWidget* widget);
    bool toggleShowSelectedOnlyCB(GtkWidget* widget);

    static bool showChunksCB(GtkWidget* widget);
    bool moveStreamCB(GtkWidget* widget);
    bool deleteStreamCB(GtkWidget* widget);

    bool applyBoxPluginCB(GtkWidget* widget);
    bool trackerPluginCB(GtkWidget* widget);
    bool setSelectionCB(GtkWidget* widget);

    bool clearMessagesCB() const;

    // Render a stream by pushing the chunks in view to the renderer
    template <typename T, typename TRenderer>
    bool draw(const Stream<T>* stream, TRenderer& renderer, CTime startTime, CTime endTime)
    {
        renderer.reset();

        typename T::Buffer* buf;

        // Push the visible chunks to renderer
        uint64_t chunksSent = 0;
        for (size_t i = 0; i < stream->getChunkCount(); ++i) {
            if (stream->getChunk(i, &buf) && buf->m_StartTime >= startTime && buf->m_EndTime <= endTime) {
                renderer.push(*buf);
                chunksSent++;
            }
        }

        /*
        CTime viewDuration = (endTime - startTime).ceil();
        uint64_t chunkCount = viewDuration /renderer.getChunkDuration();

        std::cout << "View duration " << viewDuration.toSeconds() << ", chks " << chunkCount << "\n";

        std::cout << "Pushed " << chunksSent << " chks, padding " << chunkCount-chunksSent << " -> " << chunkCount << "\n";
        if (buf && chunksSent < chunkCount)
        {
            while (chunksSent < chunkCount)
            {
                // @fixme push empty
                renderer.push(*buf, true);
                chunksSent++;
            }
        }
        */
        renderer.finalize();

        return true;
    }

    bool redrawTrack(size_t index);
    bool redrawAllTracks();
    bool redrawStream(size_t trackIndex, size_t streamIndex);
    bool resetWidgetProperties();

    bool initializeRenderers();
    bool clearRenderers();
    bool updateRulerState();

    CTracker& m_tracker;

    std::vector<GUITrack*> m_tracks; // One per each track in workspace

    struct _GtkBuilder* m_interface = nullptr;

    GtkWidget* m_mainWindow      = nullptr;
    GtkWidget* m_scrollbar       = nullptr;
    GtkWidget* m_scale           = nullptr;
    GtkWidget* m_workspaceLabel  = nullptr;
    GtkWidget* m_timeDisplay     = nullptr;
    GtkWidget* m_trackCounter    = nullptr;
    GtkWidget* m_selectionWindow = nullptr;

    uint64_t m_numChannels = 0;
    uint64_t m_chunkSize   = 0;
    uint64_t m_totalChunks = 0;

    CTime m_previousTime = CTime::min();
    CTime m_lastRedraw   = CTime::min();

#if defined(TARGET_OS_Windows)
    int m_requestResetInNFrames = -1;
    bool m_expanderOpen         = false;
    double m_hScrollBarValue    = 0;
#endif

    bool m_requestRedraw  = false;
    bool m_waitingForStop = false;

    CLogListenerTracker* m_logListener = nullptr;

    // This hack is needed since on Ubuntu 16.04, for some reason
    // gtk_widget_set_sensitive doesn't work on main level menu items,
    // and descending the menu as a container doesn't seem to allow
    // graying out the menu items either
    std::vector<GtkWidget*> m_trackMenuWidgets;
};
}  // namespace Tracker
}  // namespace OpenViBE
