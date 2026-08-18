//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <iostream>
#include <ctime>     // std::ctime
#include <algorithm> // std::replace, std::max
#include <iomanip>   //std::setw, setfill
#include <fs/Files.h>

#include "Workspace.h"
#include "StreamBundleImportExport.h"

#include "StimulationStreamFilter.h"

namespace OpenViBE {
namespace Tracker {

Workspace::~Workspace()
{
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i]) {
            m_tracks[i]->uninitialize();
            delete m_tracks[i];
        }
    }
    m_tracks.clear();
}

bool Workspace::setUniqueWorkingPath()
{
    const CString workspacePrefix = m_kernelCtx.getConfigurationManager().expand("${Path_UserData}/tracker-workspace-");

    const auto currentTime = std::chrono::system_clock::now();
    std::time_t t          = std::chrono::system_clock::to_time_t(currentTime);
    std::string stringTime = std::ctime(&t);
    std::replace(stringTime.begin(), stringTime.end(), ' ', '-');
    std::replace(stringTime.begin(), stringTime.end(), ':', '-');
    std::replace(stringTime.begin(), stringTime.end(), '/', '-');
    std::replace(stringTime.begin(), stringTime.end(), '\\', '-');
    std::replace(stringTime.begin(), stringTime.end(), '\n', '-');

    // This is not a super safe way to create a directory name but it is unlikely the user 
    // would run many trackers at the same time
    const std::string workspacePath = std::string(workspacePrefix.toASCIIString()) + stringTime;
    uint32_t counter                = 0;
    while (FS::Files::directoryExists((workspacePath + std::to_string(counter)).c_str())) { counter++; }

    setWorkingPath(CString((workspacePath + std::to_string(counter)).c_str()));

    return true;
}

bool Workspace::step()
{
    if (m_executor->isIdle()) {
        log() << Kernel::LogLevel_Info << "All jobs finished\n";
        stop();
        return false;
    }
    System::Time::sleep(1);
    return true;
}

bool Workspace::spoolRecordingToDisk(const size_t trackIndex)
{
    if (trackIndex >= m_tracks.size()) {
        log() << Kernel::LogLevel_Error << "Index " << trackIndex << " is outside array.\n";
        return false;
    }

    // Spool the new track to disk
    std::stringstream filename;
    filename << m_workspacePath << "/workspace"
            << "-track" << std::setw(3) << std::setfill('0') << trackIndex + 1
            << "-rev" << std::setw(3) << std::setfill('0') << m_revision << ".ov";

    saveStreamBundleToFile(m_kernelCtx, m_tracks[trackIndex], filename.str().c_str());
    m_tracks[trackIndex]->setSource(filename.str());

    if (m_memorySaveMode) {
        // Load back in memory save mode
        delete m_tracks[trackIndex];
        m_tracks[trackIndex] = readStreamBundleFromFile(m_kernelCtx, filename.str().c_str(), true);
    }

    return true;
}

std::string Workspace::getProcessorArguments(const size_t index)
{
    std::string trackSource("None");
    if (index >= 0 && m_tracks[index]->getSource().length() > 0) { trackSource = m_tracks[index]->getSource(); }

    // Append generic configuration tokens. n.b. make sure no define argument is an empty string.
    std::stringstream ss;
    ss << " ";
    ss << "--define Tracker_Workspace_File " << "\"" << (m_workspaceFile.length() > 0 ? m_workspaceFile : "None") << "\" ";
    ss << "--define Tracker_Workspace_Path " << "\"" << (m_workspacePath.length() > 0 ? m_workspacePath : "None") << "\" ";
    ss << "--define Tracker_CurrentTrack_Number " << index + 1 << " ";
    ss << "--define Tracker_CurrentTrack_Source " << "\"" + trackSource << "\" ";
    ss << "--define Tracker_CatenatePlayback " << (m_catenateMode ? "True" : "False") << " ";

    // Include those potentially set by the user in the GUI
    std::string args = std::string(m_processorArguments.toASCIIString()) + ss.str();

    return args;
}


bool Workspace::getNextTrack(size_t& nextTrack) const
{
    nextTrack = nextTrack + 1;
    while (nextTrack < getNumTracks() && !m_selection.isTrackSelected(nextTrack)) { nextTrack++; }

    if (nextTrack >= getNumTracks()) {
        nextTrack = size_t(-1);
        return false;
    }

    return true;
}

bool Workspace::assemblePlaylist()
{
    m_playlist.clear();

    m_playlistDuration = 0;

    size_t nextTrack = size_t(-1);
    while (getNextTrack(nextTrack)) {
        StreamBundle* source = m_tracks[nextTrack];
        m_playlistDuration += source->getMaxDuration();

        SourceTimePair tmp(source, CTime::min());

        m_playlist.push_back(tmp);
    }

    return true;
}


// Construct list of streamsubsets to process
// make processing each streamsubset a job
// connect input and output to the job
// monitor until jobs are complete
//
bool Workspace::play(const bool playFast)
{
    m_playFast         = playFast;
    m_pleaseQuit       = false;
    m_tracksDone       = 0;
    m_playlistDuration = 0;

    if (!m_executor) {
        log() << Kernel::LogLevel_Error << "Need a parallel executor set\n";
        return false;
    }

    if (!m_selection.isSelectionConsistent()) {
        log() << Kernel::LogLevel_Error <<
                "For processing, the selected streams for each track must have equal types, in equal amounts, and in the same stream type order.\n";
        return false;
    }

    if (!m_processor.canPush() && !m_processor.canPull()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Please configure the processor to send, receive, or both.\n";
        return false;
    }

    if (!m_processor.canPush()) { return playReceiveOnly(); }

    // Generate streamsubsets to process
    assemblePlaylist();

    if (m_catenateMode) { return playCatenate(); }
    return playNormal();
}

bool Workspace::playCatenate()
{
    uint32_t portToUse, dummy;
    m_processor.getProcessorPorts(portToUse, dummy);

    const std::string filename = m_processorFilename;

    bool playFast = m_playFast;
    auto& refProc = m_processor;

    std::string args = getProcessorArguments(0);

    auto job = [playFast/*, portToUse*/, args, filename, &refProc, this](uint32_t /*threadNumber*/)
    {
        uint32_t playlistIndex = 0;

        StreamBundle* target = new StreamBundle(this->getKernelContext());

        ProcExternalProcessing proc(getKernelContext(), refProc);
        proc.setArguments(args.c_str());
        proc.setNewTarget(target);

        StreamBundle* loaded = nullptr;
        StreamBundle* subset = nullptr;

        auto quitCallback = [this,playlistIndex](const CTime spent)
        {
            {
                std::unique_lock<std::mutex> (m_Mutex);
                this->m_playlist[playlistIndex].second = spent;
            }
            return isQuitRequested();
        };

        auto nextTrackFun = [this,&playlistIndex,&proc,&subset, &loaded]()
        {
            delete subset;

            if (playlistIndex >= m_playlist.size()) { return false; }

            this->log() << Kernel::LogLevel_Info << "Switching to process track " << playlistIndex + 1 << " out of " << m_playlist.size() << "\n";

            subset = new StreamBundle(this->m_kernelCtx);

            const auto& ptr = m_playlist[playlistIndex];

            StreamBundle* source = ptr.first;

            if (this->m_memorySaveMode) {
                this->log() << Kernel::LogLevel_Info << "Memory save mode: Loading " << source->getSource().c_str() << " from disk\n";

                loaded = readStreamBundleFromFile(this->m_kernelCtx, source->getSource().c_str(), false);

                // copy selection from the one in memory
                for (size_t i = 0; i < source->getNumStreams(); ++i) { loaded->getStream(i)->setSelected(source->getStream(i)->getSelected()); }
                subset->copyFrom(*loaded);

                delete loaded;
            }
            else {
                // Select subset of streams to play
                subset->copyFrom(*source);
            }

            const bool isFirst = (playlistIndex == 0);
            const bool isLast  = (playlistIndex >= m_playlist.size() - 1);

            proc.setNewSource(subset, isFirst, isLast);

            {
                std::unique_lock<std::mutex>(this->getMutex());
                this->m_tracksDone++;
            }

            playlistIndex++;

            return true;
        };

        proc.play(playFast, quitCallback, nextTrackFun);

        this->log() << Kernel::LogLevel_Info << "Track complete.\n";


        if (target->getNumStreams() > 0) {
            // Single threaded here, so can get m_Tracks.size() outside mutex
            const size_t newIndex = m_tracks.size();
            this->setTrack(newIndex, target);
            // Spool result to disk, note that it might be incomplete as the user requested stop
            if (m_memorySaveMode) {
                log() << Kernel::LogLevel_Info << "Writing new track to disk.\n";
                spoolRecordingToDisk(newIndex);
            }
        }
        else { delete target; }
    };

    m_executor->pushJob(job);

    return true;
}

bool Workspace::playNormal()
{
    uint32_t firstPort, dummy;
    m_processor.getProcessorPorts(firstPort, dummy);

    const size_t lastFreeIndex = m_tracks.size();
    bool playFast              = m_playFast;
    const size_t totalTracks   = m_playlist.size();

    for (size_t playlistIndex = 0; playlistIndex < totalTracks; ++playlistIndex) {
        auto& ptr = m_playlist[playlistIndex];

        std::string args           = getProcessorArguments(playlistIndex);
        const std::string filename = m_processorFilename;

        auto& refProc = m_processor;

        auto job = [&ptr,&refProc,args, playFast,filename, lastFreeIndex, playlistIndex, firstPort, totalTracks, this](const uint32_t threadNumber)
        {
            StreamBundle* original = ptr.first;
            StreamBundle* target   = new StreamBundle(this->m_kernelCtx);
            StreamBundle* subset   = new StreamBundle(this->m_kernelCtx);

            this->log() << Kernel::LogLevel_Trace << "Switching to process track " << playlistIndex + 1 << " out of " << totalTracks << "\n";

            if (this->m_memorySaveMode) {
                this->log() << Kernel::LogLevel_Info << "Memory save mode: Loading " << original->getSource().c_str() << " from disk\n";

                StreamBundle* loaded = readStreamBundleFromFile(this->m_kernelCtx, original->getSource().c_str(), false);

                // copy selection from the one in memory
                for (size_t i = 0; i < original->getNumStreams(); ++i) { loaded->getStream(i)->setSelected(original->getStream(i)->getSelected()); }
                subset->copyFrom(*loaded);

                delete loaded;
            }
            else {
                // Select subset of streams to play
                subset->copyFrom(*original);
            }

            auto quitCallback = [this,playlistIndex](const CTime spent)
            {
                {
                    std::unique_lock<std::mutex> (m_Mutex);
                    this->m_playlist[playlistIndex].second = spent;
                }
                this->m_playlist[playlistIndex].second = spent;
                return isQuitRequested();
            };

            // Process
            ProcExternalProcessing proc(this->getKernelContext(), refProc);
            proc.setNewSource(subset, true, true);
            proc.setNewTarget(target);
            proc.setArguments(args.c_str());
            proc.setProcessorPorts(firstPort + 2 * threadNumber, firstPort + 2 * threadNumber + 1);
            proc.play(playFast, quitCallback);

            this->log() << Kernel::LogLevel_Trace << "Track " << playlistIndex + 1 << " complete.\n";
            {
                std::unique_lock<std::mutex>(this->getMutex());
                this->m_tracksDone++;
            }

            // Clean-up
            delete subset;

            if (target->getNumStreams() > 0) {
                // if not inplacemode, attempt to insert in the same order as the sources were in the list
                const size_t newIndex = (this->getInplaceMode() ? playlistIndex : lastFreeIndex + playlistIndex);

                this->setTrack(newIndex, target);

                // Spool result to disk, note that it might be incomplete as the user requested stop
                if (m_memorySaveMode) {
                    log() << Kernel::LogLevel_Info << "Writing new track to disk.\n";
                    spoolRecordingToDisk(newIndex);
                }
            }
            else {
                // this->log() << Kernel::LogLevel_Warnin
                delete target;
            }
        };

        m_executor->pushJob(job);
    }

    return true;
}

bool Workspace::playReceiveOnly()
{
    const size_t lastFreeIndex = m_tracks.size();
    bool playFast              = m_playFast;

    std::string args           = getProcessorArguments(-1);
    const std::string filename = m_processorFilename;

    auto& refProc = m_processor;

    m_playlist.clear();
    m_playlist.push_back(SourceTimePair(nullptr, CTime()));

    auto job = [&refProc,args, playFast,filename, lastFreeIndex, this](uint32_t /*threadNumber*/)
    {
        StreamBundle* target = new StreamBundle(this->m_kernelCtx);

        this->log() << Kernel::LogLevel_Info << "Recording a track (noSend configured)\n";

        auto quitCallback = [this](const CTime spent)
        {
            // single thread, no cc
            this->m_playlistDuration   = spent;
            this->m_playlist[0].second = spent;
            return isQuitRequested();
        };

        // Process
        ProcExternalProcessing proc(this->getKernelContext(), refProc);
        proc.setNewSource(nullptr, true, true);
        proc.setNewTarget(target);
        proc.setArguments(args.c_str());
        proc.play(playFast, quitCallback);

        this->log() << Kernel::LogLevel_Info << "Recording complete.\n";

        if (target->getNumStreams() > 0) {
            // if not inplacemode, attempt to insert in the same order as the sources were in the list
            const size_t newIndex = lastFreeIndex;

            this->setTrack(newIndex, target);

            // Spool result to disk, note that it might be incomplete as the user requested stop
            if (m_memorySaveMode) {
                log() << Kernel::LogLevel_Info << "Writing new track to disk.\n";
                spoolRecordingToDisk(newIndex);
            }
        }
        else { delete target; }
    };

    m_executor->pushJob(job);

    return true;
}

bool Workspace::stop(const bool stopProcessor)
{
    if (stopProcessor) {
        std::unique_lock<std::mutex> m_Mutex;
        m_executor->clearPendingJobs();
        m_pleaseQuit = true;
    }
    return true;
}

bool Workspace::clearTracks()
{
    for (size_t i = 0; i < m_tracks.size(); ++i) { delete m_tracks[i]; }
    m_tracks.clear();

    return true;
}

bool Workspace::removeTrack(const size_t idx)
{
    if (idx >= m_tracks.size()) { return false; }
    delete m_tracks[idx];
    m_tracks.erase(m_tracks.begin() + idx);

    return true;
}

bool Workspace::removeStream(const size_t track, const size_t stream)
{
    if (track >= getNumTracks() || stream >= getTrack(track)->getNumStreams()) { return false; }
    if (getTrack(track)->deleteStream(stream)) { return true; }
    return false;
}

bool Workspace::addTrack(const char* filename)
{
    if (!filename || !filename[0]) { return false; }

    StreamBundle* newTrack = readStreamBundleFromFile(m_kernelCtx, filename, m_memorySaveMode);
    if (!newTrack) { return false; }

    log() << Kernel::LogLevel_Debug << "The loaded track has " << newTrack->getNumStreams() << " streams\n";
    for (size_t i = 0; i < newTrack->getNumStreams(); ++i) {
        if (newTrack->getStream(i)) {
            log() << Kernel::LogLevel_Debug << "  Stream " << i << " has type " << newTrack->getStream(i)->getTypeIdentifier().str()
                    << " == " << m_kernelCtx.getTypeManager().getTypeName(newTrack->getStream(i)->getTypeIdentifier()) << "\n";
        }
        else {
            // @fixme this has the issue that even though the stream may have a definition, its lost currently if there's not even a header chunk in the stream
            log() << Kernel::LogLevel_Info << "  Stream " << i << " has a type the Tracker couldn't decode (or the stream was empty)\n";
        }
    }

    m_tracks.push_back(newTrack);

    for (auto& str : newTrack->getAllStreams()) { str->setSelected(true); }

    return true;
}

bool Workspace::moveStream(const size_t sourceTrack, const size_t sourceStream, const size_t targetTrack, const size_t targetStream)
{
    if (sourceTrack >= getNumTracks() || targetTrack >= getNumTracks()) { return false; }
    if (sourceTrack == targetTrack && sourceStream == targetStream) { return true; }
    if (sourceStream >= m_tracks[sourceTrack]->getNumStreams() || targetStream >= m_tracks[targetTrack]->getNumStreams()) { return false; }

    if (sourceTrack != targetTrack) {
        const auto oldPtr = m_tracks[sourceTrack]->getStream(sourceStream);

        // Move pointer, do not free memory
        auto& allStreams = m_tracks[sourceTrack]->getAllStreams();
        allStreams.erase(allStreams.begin() + sourceStream);
        m_tracks[sourceTrack]->setDirtyBit(true);

        m_tracks[targetTrack]->setStream(m_tracks[targetTrack]->getNumStreams(), oldPtr);    // append to the end
        return m_tracks[targetTrack]->moveStream(m_tracks[targetTrack]->getNumStreams() - 1, targetStream);
    }
    return m_tracks[sourceTrack]->moveStream(sourceStream, targetStream);
}

bool Workspace::moveTrack(const size_t sourceIdx, const size_t targetIdx)
{
    if (sourceIdx >= getNumTracks() || targetIdx >= getNumTracks()) { return false; }
    if (sourceIdx == targetIdx) { return true; }

    const auto oldPtr = m_tracks[sourceIdx];
    m_tracks.erase(m_tracks.begin() + sourceIdx);
    m_tracks.insert(m_tracks.begin() + targetIdx, oldPtr);

    return true;
}


bool Workspace::reloadTrack(const size_t index)
{
    if (index >= m_tracks.size()) { return false; }
    StreamBundle* newTrack = readStreamBundleFromFile(m_kernelCtx, m_tracks[index]->getSource().c_str(), m_memorySaveMode);

    delete m_tracks[index];

    m_tracks[index] = newTrack;

    return true;
}

CTime Workspace::getMaxDuration() const
{
    CTime maxDuration = CTime::min();
    for (size_t i = 0; i < m_tracks.size(); ++i) { if (m_tracks[i]) { maxDuration = std::max<CTime>(maxDuration, m_tracks[i]->getMaxDuration()); } }
    return maxDuration;
}


bool Workspace::setProcessor(const char* scenarioXml)
{
    m_kernelCtx.getConfigurationManager().addOrReplaceConfigurationToken("Tracker_Workspace_Processor", scenarioXml);
    m_processorFilename = scenarioXml;
    return m_processor.initialize(scenarioXml);
}

bool Workspace::setProcessorFlags(const bool noGUI, const bool doSend, const bool doReceive) { return m_processor.setProcessorFlags(noGUI, doSend, doReceive); }
bool Workspace::getProcessorFlags(bool& noGUI, bool& doSend, bool& doReceive) const { return m_processor.getProcessorFlags(noGUI, doSend, doReceive); }
bool Workspace::setProcessorPorts(const uint32_t sendPort, const uint32_t recvPort) { return m_processor.setProcessorPorts(sendPort, recvPort); }

bool Workspace::saveAll()
{
    if (m_workspacePath.length() == 0) {
        log() << Kernel::LogLevel_Error << "Error: Workspace path not set...\n";
        return false;
    }
    if (!FS::Files::directoryExists(m_workspacePath.toASCIIString())) {
        if (!FS::Files::createPath(m_workspacePath.toASCIIString())) {
            log() << Kernel::LogLevel_Error << "Error: Unable to create directory " << m_workspacePath << "\n";
            return false;
        }
    }

    bool retVal = true;

    if (getNumTracks() > 0) { log() << Kernel::LogLevel_Info << "Saving modified tracks to " << m_workspacePath << " ...\n"; }

    uint32_t tracksSaved = 0;
    for (size_t i = 0; i < getNumTracks(); ++i) {
        if (!getTrack(i)->getDirtyBit()) {
            log() << Kernel::LogLevel_Trace << "Skipping track " << i + 1 << " / " << getNumTracks() << ", no modifications ...\n";
            continue;
        }
        log() << Kernel::LogLevel_Trace << "Saving track " << i + 1 << " / " << getNumTracks() << " ...\n";

        std::stringstream filename;
        filename << m_workspacePath << "/workspace"
                << "-track" << std::setw(3) << std::setfill('0') << i + 1
                << "-rev" << std::setw(3) << std::setfill('0') << m_revision << ".ov";

        // If we're in the memory save mode, we need to copy contents of the track to the new location
        StreamBundle* track = (m_memorySaveMode ? readStreamBundleFromFile(m_kernelCtx, getTrack(i)->getSource().c_str(), false) : getTrack(i));

        retVal &= saveStreamBundleToFile(m_kernelCtx, track, filename.str().c_str());

        tracksSaved++;

        if (m_memorySaveMode) {
            getTrack(i)->setSource(track->getSource());
            getTrack(i)->setDirtyBit(false);
            delete track;
        }
    }

    log() << Kernel::LogLevel_Info << "Done, " << tracksSaved << " had changed and needed to be saved.\n";

    return retVal;
}

bool Workspace::setMemorySaveMode(const bool active)
{
    if (active != m_memorySaveMode) {
        m_memorySaveMode = active;
        if (active == false) {
            // Going from memory save mode to the normal mode
            log() << Kernel::LogLevel_Info << "Switching to full mode. Loading tracks from disk.\n";
        }
        else {
            // Going from normal mode to memory save mode

            // We need to spool all the modifications to the disk so the other mode can get them
            log() << Kernel::LogLevel_Info << "Switching to memory save mode. Saving all modified tracks to disk.\n";

            saveAll();
        }

        // Read tracks back in the new mode
        for (size_t i = 0; i < m_tracks.size(); ++i) {
            const std::string filename = m_tracks[i]->getSource();
            delete m_tracks[i];
            m_tracks[i] = readStreamBundleFromFile(m_kernelCtx, filename.c_str(), m_memorySaveMode);
        }
    }

    return true;
}

bool Workspace::clear()
{
    clearTracks();
    setFilename("");
    setUniqueWorkingPath();

    return true;
}

std::vector<std::pair<std::string, std::string>> Workspace::getConfigurationTokens() const
{
    std::vector<std::pair<std::string, std::string>> tokens; // For sorting

    CIdentifier iter = CIdentifier::undefined();
    while ((iter = m_kernelCtx.getConfigurationManager().getNextConfigurationTokenIdentifier(iter)) != CIdentifier::undefined()) {
        std::string prefix("Tracker_Workspace_");
        std::string token(m_kernelCtx.getConfigurationManager().getConfigurationTokenName(iter).toASCIIString());

        // See if token has the prefix?
        auto res = std::mismatch(prefix.begin(), prefix.end(), token.begin());
        if (res.first == prefix.end()) {
            std::string value(m_kernelCtx.getConfigurationManager().getConfigurationTokenValue(iter));
            tokens.push_back(std::pair<std::string, std::string>(token, value));

            // ::fprintf(file, "%s = %s\n", token.c_str(), value.toASCIIString());
        }
    }

    std::sort(tokens.begin(), tokens.end());

    return tokens;
}

// @fixme for multiple workspaces this solution needs to be reworked
bool Workspace::save(const CString& filename)
{
    if (!m_tracks.empty() && !saveAll()) { return false; }

    // Save selection to configuration manager
    m_selection.save("Tracker_Workspace_");

    // Save processor configuration to manager
    m_processor.save();

    // Set workspaces own configuration tokens to manager
    // @todo Why not already do this in the setters?
    std::stringstream trackCount;
    trackCount << m_tracks.size();
    std::stringstream revision;
    revision << m_revision;

    auto& mgr = m_kernelCtx.getConfigurationManager();
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Path", m_workspacePath.toASCIIString());
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_CatenatePlayback", (m_catenateMode ? "true" : "false"));
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_MemorySaveMode", (m_memorySaveMode ? "true" : "false"));
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_InplaceMode", (m_inplaceMode ? "true" : "false"));
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Track_Count", trackCount.str().c_str());
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor_Arguments", m_processorArguments.toASCIIString());
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Revision", revision.str().c_str());

    FILE* file = fopen(filename.toASCIIString(), "wt");
    if (file) {
        fprintf(file, "# Workspace configuration file generated by OpenViBE Tracker\n");
        fprintf(file, "#\n");
        fprintf(file, "\n");

        for (size_t i = 0; i < m_tracks.size(); ++i) {
            std::stringstream token;
            token << "Tracker_Workspace_Track_" << std::setw(3) << std::setfill('0') << (i + 1);
            mgr.addOrReplaceConfigurationToken(token.str().c_str(), m_tracks[i]->getSource().c_str());
        }

        // Spool all generic Tracker tokens from the manager to the file
        auto tokens = getConfigurationTokens();
        for (auto& token : tokens) { fprintf(file, "%s = %s\n", token.first.c_str(), token.second.c_str()); }

        fprintf(file, "\n");

        fclose(file);
    }
    else {
        log() << Kernel::LogLevel_Error << "Error: Couldn't open " << filename.toASCIIString() << " for writing\n";
        return false;
    }

    m_workspaceFile = filename;

    // Save workspace notes
    if (m_notes.getText().length() > 0 && m_workspacePath.length() > 0) {
        CString notesFile = m_workspacePath + CString("/workspace-notes.txt");
        m_notes.save(notesFile);
    }

    return true;
}

bool Workspace::load(const CString& filename)
{
    // @todo might wipe only specific tokens in the future, esp. if multiple workspaces become supported
    wipeConfigurationTokens("Tracker_Workspace_");

    auto& mgr = m_kernelCtx.getConfigurationManager();

    if (!mgr.addConfigurationFromFile(filename)) { return false; }

    clearTracks();

    m_workspaceFile = filename;

    const CString savedWorkspacePath = mgr.expand("${Tracker_Workspace_Path}");
    if (savedWorkspacePath.length() > 0) { m_workspacePath = savedWorkspacePath; }

    setCatenateMode(mgr.expandAsBoolean("${Tracker_Workspace_CatenatePlayback}", m_catenateMode));

    // Here we don't use the setter as we're loading from scratch
    m_memorySaveMode = mgr.expandAsBoolean("${Tracker_Workspace_MemorySaveMode}", m_memorySaveMode);
    m_inplaceMode    = mgr.expandAsBoolean("${Tracker_Workspace_InplaceMode}", m_inplaceMode);

    m_revision = mgr.expandAsUInteger("${Tracker_Workspace_Revision}", m_revision);
    // m_NumRevisions = mgr.expandAsUInteger("${Tracker_Workspace_NumRevisions}", m_NumRevisions);

    m_processorArguments = mgr.expand("${Tracker_Workspace_Processor_Arguments}");

    const uint32_t trackCount = uint32_t(mgr.expandAsUInteger("${Tracker_Workspace_Track_Count}", 0));
    for (uint32_t i = 0; i < trackCount; ++i) {
        std::stringstream token;
        token << "Tracker_Workspace_Track_" << std::setw(3) << std::setfill('0') << (i + 1);
        if (mgr.lookUpConfigurationTokenIdentifier(token.str().c_str()) != CIdentifier::undefined()) {
            CString tokenValue = mgr.lookUpConfigurationTokenValue(token.str().c_str());
            CString trackFile  = mgr.expand(tokenValue);
            log() << Kernel::LogLevel_Debug << "Loading track " << i + 1 << " : " << trackFile << "\n";
            addTrack(trackFile.toASCIIString());
        }
    }
    log() << Kernel::LogLevel_Info << "Loaded " << m_tracks.size() << " tracks of the workspace\n";

    // Load processor config
    m_processor.load();

    // Load workspace notes
    const CString notesFile = m_workspacePath + CString("/workspace-notes.txt");
    m_notes.load(notesFile);

    // Load selection
    m_selection.load("Tracker_Workspace_");

    return true;
}

bool Workspace::incrementRevisionAndSave(const CString& /*filename*/)
{
    if (m_workspacePath.length() == 0) {
        log() << Kernel::LogLevel_Error << "Please set workspace path before saving revision\n";
        return false;
    }

    for (auto& ptr : m_tracks) { ptr->setDirtyBit(true); }

    m_revision++;
    //  m_NumRevisions++;

    log() << Kernel::LogLevel_Info << "Revision updated to " << m_revision << "\n";

    const bool retVal = save(m_workspaceFile);

    std::stringstream ss;
    ss << m_workspacePath + "/revision-" << std::setw(3) << std::setfill('0') << m_revision << "-backup.ovw";
    const std::string fn = ss.str();
    FS::Files::remove(fn.c_str());
    if (FS::Files::copyFile(m_workspaceFile.toASCIIString(), fn.c_str())) { log() << Kernel::LogLevel_Info << "Revision backup saved to " << ss.str() << "\n"; }
    else { log() << Kernel::LogLevel_Info << "Error saving backup to " << ss.str() << "\n"; }

    return retVal;
}


bool Workspace::wipeConfigurationTokens(const std::string& prefix) const
{
    CIdentifier it = CIdentifier::undefined(), prev = CIdentifier::undefined();
    while ((it = m_kernelCtx.getConfigurationManager().getNextConfigurationTokenIdentifier(it)) != CIdentifier::undefined()) {
        std::string token(m_kernelCtx.getConfigurationManager().getConfigurationTokenName(it).toASCIIString());

        // See if token has the prefix?
        auto res = std::mismatch(prefix.begin(), prefix.end(), token.begin());
        if (res.first == prefix.end()) {
            m_kernelCtx.getConfigurationManager().releaseConfigurationToken(it);
            it = prev;
        }
        else { prev = it; }
    }
    return true;
}


CTime Workspace::getProcessedTime() const
{
    std::unique_lock<std::mutex> m_Mutex;

    CTime processed = CTime::min();
    for (auto& ptr : m_playlist) { processed += ptr.second; }
    return processed;
}

}  // namespace Tracker
}  // namespace OpenViBE
