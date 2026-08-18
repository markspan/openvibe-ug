#pragma once

#include "StreamBundle.h"

#include "ProcExternalProcessing.h"

#include <openvibe/ov_all.h>

#include "WorkspaceNotes.h"
#include "Selection.h"
#include "ParallelExecutor.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class Workspace 
 * \brief Workspace is a set of Tracks (StreamBundles) that contain the .ov file content that the user wants to work with
 * \author J. T. Lindgren
 *
 */
class Workspace final : protected Contexted
{
public:
    explicit Workspace(const Kernel::IKernelContext& ctx)
        : Contexted(ctx), m_workspaceFile(""), m_workspacePath(""), m_processor(ctx), m_selection(ctx, m_tracks) { setUniqueWorkingPath(); }

    ~Workspace() override;

    // Reset workspace to the state like it was after construction
    bool clear();

    // Manipulating tracks and streams currently in the workspace
    bool addTrack(const char* filename);    // Add an .ov file as a track

    bool moveTrack(size_t sourceIdx, size_t targetIdx);
    bool moveStream(size_t sourceTrack, size_t sourceStream, size_t targetTrack, size_t targetStream);

    bool clearTracks();
    bool removeTrack(size_t idx);
    bool removeStream(size_t track, size_t stream);

    size_t getNumTracks() const { return m_tracks.size(); }

    size_t getNumTracksOnPlaylist() const { return m_playlist.size(); }
    size_t getNumTracksDone() const { return m_tracksDone; }

    bool reloadTrack(size_t index);

    // Does not transfer pointer ownership
    StreamBundle* getTrack(const size_t index)
    {
        std::unique_lock<std::mutex> (m_om_Mutex);

        if (index >= m_tracks.size()) { return nullptr; }
        return m_tracks[index];
    }

    // Transfers ownership of the pointer. Returns the index inserted to.
    bool setTrack(const size_t index, StreamBundle* track)
    {
        std::unique_lock<std::mutex> (m_om_Mutex);
        if (index >= m_tracks.size()) { m_tracks.resize(index + 1, nullptr); }
        delete m_tracks[index];
        m_tracks[index] = track;

        return true;
    }

    bool play(const bool playFast);
    bool stop(bool stopProcessor = true);

    // Push the selected part of the tracks to processor
    bool step();

    // get selection. Since it returns a reference, to set notes, simply modify the ref'd obj content.
    Selection& getSelection() { return m_selection; }

    // Max duration of all tracks in the workspace, in fixed point seconds
    CTime getMaxDuration() const;

    CTime getProcessedTime() const;
    CTime getPlaylistDuration() const { return m_playlistDuration; }

    // Set output for chunks
    // @todo refactor these to a getter/setter of a processor object?
    bool setProcessor(const char* scenarioXml);
    bool setProcessorFlags(bool noGUI, bool doSend, bool doReceive);
    bool getProcessorFlags(bool& noGUI, bool& doSend, bool& doReceive) const;
    const char* getProcessorFile() const { return m_processor.getFilename().c_str(); }
    bool configureProcessor(const char* filename) { return m_processor.configure(filename); }
    bool setProcessorPorts(uint32_t sendPort, uint32_t recvPort);
    bool getProcessorPorts(uint32_t& sendPort, uint32_t& recvPort) const { return m_processor.getProcessorPorts(sendPort, recvPort); }

    bool hasProcessor() const
    {
        const char* fn = getProcessorFile();
        return (fn && fn[0] != 0);
    }

    CString getProcessorArguments() const { return m_processorArguments; }

    bool setProcessorArguments(const CString& args)
    {
        m_processorArguments = args;
        return true;
    }

    CString getWorkingPath() const { return m_workspacePath; }

    bool setWorkingPath(const CString& path)
    {
        if (path.length() == 0) {
            log() << Kernel::LogLevel_Error << "Empty workspace path not allowed\n";
            return false;
        }
        m_workspacePath = path;
        return true;
    }

    // Creates a 'unique' working path and assigns it, but does not create the folder
    bool setUniqueWorkingPath();

    CString getFilename() const { return m_workspaceFile; }

    bool setFilename(const CString& filename)
    {
        if (filename.length() == 0) {
            // unset
            m_workspaceFile = CString("");
            return true;
        }
        return this->save(filename);
    }

    // get workspace notes. Since it returns a reference, to set notes, simply modify the ref'd obj content.
    Kernel::IComment& getNotes() { return m_notes; }

    // @todo make generic property getter/setter?
    bool getCatenateMode() const { return m_catenateMode; }

    bool setCatenateMode(const bool newState)
    {
        m_catenateMode = newState;
        return true;
    }

    // @fixme rework the setter to spool files to/from disk when mode changes
    bool getMemorySaveMode() const { return m_memorySaveMode; }
    bool setMemorySaveMode(bool active);

    // Modify tracks in place?
    bool getInplaceMode() const { return m_inplaceMode; }

    bool setInplaceMode(const bool newState)
    {
        m_inplaceMode = newState;
        return true;
    }

    uint64_t getRevision() const { return m_revision; }
    //uint64_t getNumRevisions() const { return m_NumRevisions; }

    // The processors can poll this now and then to find out if they should quit ahead of the file end
    bool isQuitRequested() const
    {
        std::unique_lock<std::mutex> m_Mutex;
        return m_pleaseQuit;
    }

    std::mutex& getMutex() const { return m_Mutex; }

    // Clear configuration tokens with a given prefix
    bool wipeConfigurationTokens(const std::string& prefix) const;

    bool setParallelExecutor(ParallelExecutor* ptr)
    {
        m_executor = ptr;
        return true;
    }

    // Save and load workspace
    bool save(const CString& filename);
    bool load(const CString& filename);

    bool incrementRevisionAndSave(const CString& filename);

    // get sorted workspace specific tokens
    std::vector<std::pair<std::string, std::string>> getConfigurationTokens() const;

protected:
    bool saveAll();

    bool getNextTrack(size_t& nextTrack) const;
    bool assemblePlaylist();

    bool spoolRecordingToDisk(size_t trackIndex);

    bool playReceiveOnly();
    bool playCatenate();
    bool playNormal();

    // Assemble user-specified and generated arguments to the processor
    std::string getProcessorArguments(size_t index);

    // Generic mutex to protect access to certain variables from multiple threads
    mutable std::mutex m_Mutex;

    std::vector<StreamBundle*> m_tracks;

    bool m_playFast       = false;
    bool m_catenateMode   = false;
    bool m_inplaceMode    = false;
    bool m_memorySaveMode = false;
    bool m_pleaseQuit     = false;

    uint32_t m_tracksDone = 0;

    CString m_workspaceFile;
    CString m_workspacePath;

    // Component to send and receive data to/from, actually just a store for settings
    ProcExternalProcessing m_processor;
    std::string m_processorFilename;
    CString m_processorArguments;

    typedef std::pair<StreamBundle*, CTime> SourceTimePair;
    std::vector<SourceTimePair> m_playlist;
    CTime m_playlistDuration = CTime::min();

    // Text notes about the current workspace; defined as a class to be able to reuse Designer's CCommentEditorDialog
    WorkspaceNotes m_notes;

    // Which tracks/streams are currently selected to be processed?
    Selection m_selection;

    ParallelExecutor* m_executor = nullptr;

    // uint64_t m_NumRevisions = 1;
    uint64_t m_revision = 1;
};
}  // namespace Tracker
}  // namespace OpenViBE
