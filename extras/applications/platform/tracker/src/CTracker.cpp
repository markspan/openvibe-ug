//
// OpenViBE Tracker
// 
// @todo catch way more errors, currently most return values are ignored by callers
// @todo Change error handling to use the macros introduced in certivibe
// @todo clean the code to be more conforming to coding rules
// @todo Implement vertical slider / scale
// @todo Fix time units
// @todo Add track renderer only when the track appears
//
// @todo add dataset manager (workspace == datasets)
// @todo enable a view where only 'current track' is shown and the rest are minimized (unloaded?)
// @todo add option to collapse tracks to take less visual space
// @todo add record button
// @todo implement undo
// @todo fix issues with empty tracks/streams and if the processing fails (e.g. incompatible processor)
// @todo fix creation of noncontinuous streams in the catenate mode e.g. by padding all source streams to equal length?
// @todo write some tutorial processors, e.g. erp analysis, file export ... ? 
// @todo allow running several scenarios sequentially
// @todo selection could be saved in the .ovw file simply as track/stream tokens (small perf hit)?

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <system/ovCTime.h>

#include "CTracker.h"
#include "StreamBundleImportExport.h"

#include "Workspace.h"

namespace OpenViBE {
namespace Tracker {

CTracker::CTracker(const Kernel::IKernelContext& ctx) : Contexted(ctx), m_workspace(ctx), m_boxPlugins(ctx), m_trackerPlugins(ctx)
{
    // Default config load deferred until initialize() which can be called after booting the GUI
    // --> so we can get the possible log messages to the GUI
}

CTracker::~CTracker()
{
    const CString configFile = m_kernelCtx.getConfigurationManager().expand("${Path_UserData}/openvibe-tracker.conf");
    saveConfig(configFile);
    m_executor.uninitialize();
}

bool CTracker::initialize()
{
    const CString configFile = m_kernelCtx.getConfigurationManager().expand("${Path_UserData}/openvibe-tracker.conf");
    // n.b. Initing the executor is in loadConfig so its not done twice
    m_workspace.setParallelExecutor(&m_executor);
    return loadConfig(configFile);
}

bool CTracker::play(const bool playFast)
{
    switch (m_state) {
        case EStates::Stopped: if (!m_workspace.play(playFast)) {
                log() << Kernel::LogLevel_Error << "Error: play failed\n";
                return false;
            }
            m_state = EStates::Playing;
            break;
        case EStates::Playing: m_state = EStates::Paused;
            break;
        case EStates::Paused: m_state = EStates::Playing;
            break;
        default: break;
    }

    return true;
}

bool CTracker::stop()
{
    // Request workspace to stop, do not change state until it has stopped
    return m_workspace.stop();
}

CTracker::EStates CTracker::step()
{
    if (m_state == EStates::Playing) { if (!m_workspace.step()) { m_state = EStates::Stopping; } }
    else if (m_state == EStates::Stopping) { if (m_executor.isIdle()) { m_state = EStates::Stopped; } }
    else { System::Time::sleep(1); }

    return m_state;
}

struct Workpackage
{
    StreamBundle* source = nullptr;
    std::mutex oMutex;
    size_t trackIndex;
    std::deque<size_t> streamsToProcess;
};


bool CTracker::applyBoxPlugin(const size_t index)
{
    auto& plugins = getBoxPlugins().getBoxPlugins();
    if (index >= plugins.size()) {
        log() << Kernel::LogLevel_Error << "Plugin index exceeds plugin array size\n";
        return false;
    }

    if (getWorkspace().getMemorySaveMode() && getWorkspace().getWorkingPath().length() == 0) {
        log() << Kernel::LogLevel_Error << "Memory save mode requires a workspace path\n";
        return false;
    }

    const CTime startTime = CTime(System::Time::zgetTime());

    BoxAdapterStream* box = plugins[index];

    const CString name = box->getBox().getName();
    log() << Kernel::LogLevel_Info << "Applying method " << name << " (" << (m_workspace.getInplaceMode() ? "Inplace" : "Normal") << " mode)\n";

    for (size_t trackIndex = 0; trackIndex < getWorkspace().getNumTracks(); ++trackIndex) {
        Workpackage* wptr = new Workpackage;
        Workspace& ws     = getWorkspace();

        StreamBundle* sourceTrack = ws.getTrack(trackIndex);

        // Count
        const size_t numStreams = ws.getTrack(trackIndex)->getNumStreams();
        for (size_t j = 0; j < numStreams; ++j) {
            if (sourceTrack->getStream(j)->getSelected() && box->getBox().hasInputSupport(sourceTrack->getStream(j)->getTypeIdentifier())) {
                wptr->streamsToProcess.push_back(j);
            }
        }
        const size_t nToProcess = wptr->streamsToProcess.size();
        if (nToProcess == 0) { continue; }

        wptr->trackIndex = trackIndex;
        wptr->source     = (m_workspace.getMemorySaveMode() ? nullptr : sourceTrack);

        for (size_t j = 0; j < nToProcess; ++j) {
            const bool inplaceMode = m_workspace.getInplaceMode();
            const bool multithread = (m_executor.getNumThreads() > 1);
            const bool memorySave  = m_workspace.getMemorySaveMode();

            const Kernel::IKernelContext& ctx = getKernelContext();

            auto job = [&ws, wptr, &ctx, box, multithread, inplaceMode, memorySave](uint32_t /*threadNumber*/)
            {
                size_t sourceStreamIndex;
                {
                    std::unique_lock<std::mutex>(wptr->oMutex);
                    if (memorySave && !wptr->source) {
                        wptr->source = readStreamBundleFromFile(ctx, ws.getTrack(wptr->trackIndex)->getSource().c_str(), false);
                    }
                    sourceStreamIndex = wptr->streamsToProcess.front();
                    wptr->streamsToProcess.pop_front();
                }

                StreamBundle* sourceTrack    = wptr->source;
                const StreamPtr sourceStream = sourceTrack->getStream(sourceStreamIndex);

                // process

                if (!multithread) {
                    ctx.getLogManager() << Kernel::LogLevel_Info << "Processing track " << (wptr->trackIndex + 1)
                            << " stream " << (sourceStreamIndex + 1) << "\n";
                }

                std::unique_lock<std::mutex>(wptr->oMutex);

                const size_t targetStreamIndex = sourceTrack->getNumStreams();
                sourceTrack->createStream(targetStreamIndex, sourceStream->getTypeIdentifier());
                StreamPtr targetStream = sourceTrack->getStream(targetStreamIndex);

                // We need a copy of the box since otherwise different threads init the same box differently
                BoxAdapterStream* boxCopy = new BoxAdapterStream(ctx, box->getAlgorithmId());

                boxCopy->getBox().initializeFromExistingBox(box->getBox());

                boxCopy->setSource(sourceStream);
                boxCopy->setTarget(targetStream);

                if (!boxCopy->initialize()) {
                    std::unique_lock<std::mutex>(wptr->oMutex);

                    sourceTrack->deleteStream(targetStreamIndex);
                    delete boxCopy;

                    return;
                }

                boxCopy->spool(!multithread);
                boxCopy->uninitialize();
                delete boxCopy;

                if (inplaceMode) {
                    std::unique_lock<std::mutex>(wptr->oMutex);

                    sourceTrack->swapStreams(sourceStreamIndex, targetStreamIndex);
                    sourceTrack->deleteStream(targetStreamIndex);
                }
                else { targetStream->setSelected(false); }

                // Spool resources to disk and free memory
                {
                    std::unique_lock<std::mutex>(wptr->oMutex);

                    if (memorySave) {
                        // Free resources if this thread is the last
                        if (wptr->streamsToProcess.empty()) {
                            if (wptr->source->getDirtyBit()) {
                                // @fixme not a good solution with the filenaming; rethink the whole thing

                                std::stringstream ss;
                                ss << ws.getWorkingPath() << "/workspace-track-" << (wptr->trackIndex + 1) << ".ov";

                                // In this mode, sourceTrack is not from the track array. We spool it to disk, then
                                // set the filename of the track in the array, and reload it back.
                                saveStreamBundleToFile(ctx, wptr->source, ss.str().c_str());
                                ws.getTrack(wptr->trackIndex)->setSource(ss.str());
                                ws.reloadTrack(wptr->trackIndex);
                            }
                            delete wptr->source;
                        }
                    }
                    if (wptr->streamsToProcess.empty()) { delete wptr; }
                }
            };

            m_executor.pushJob(job);

            if (!multithread) { m_executor.waitForAll(); }
        }
    }

    m_executor.waitForAll();

    // pBoxAlgorithmDescriptor->release();

    const CTime elapsed = CTime(System::Time::zgetTime()) - startTime;
    log() << Kernel::LogLevel_Info << "Applying plugin took " << elapsed.toSeconds() << " sec.\n";

    return true;
}

bool CTracker::applyTrackerPlugin(const size_t index)
{
    auto& plugins = getTrackerPlugins().getTrackerPlugins();
    if (index >= plugins.size()) {
        log() << Kernel::LogLevel_Error << "Plugin index exceeds plugin array size\n";
        return false;
    }

    if (getWorkspace().getMemorySaveMode() && getWorkspace().getWorkingPath().length() == 0) {
        log() << Kernel::LogLevel_Error << "Memory save mode requires a workspace path\n";
        return false;
    }

    ITrackerPlugin* plugin = plugins[index];

    const CString name(plugin->getName().c_str());
    log() << Kernel::LogLevel_Info << "Applying method " << name << " (" << (m_workspace.getInplaceMode() ? "Inplace" : "Normal") << " mode)\n";

    bool retVal = true;

    if (plugin->hasCapability(ITrackerPlugin::ECapabilities::Workspace)) {
        retVal = plugin->process(getWorkspace(), m_executor);
        if (!retVal) { log() << Kernel::LogLevel_Error << "Error processing workspace with the plugin\n"; }
    }
    else if (plugin->hasCapability(ITrackerPlugin::ECapabilities::Tracks)) {
        // @note since the different jobs handle different tracks, we don't do locking here.
        for (size_t trackIndex = 0; trackIndex < getWorkspace().getNumTracks(); ++trackIndex) {
            ITrackerPlugin* pluginCopy = getTrackerPlugins().getPluginCopy(index);

            Workspace& ws                     = getWorkspace();
            const Kernel::IKernelContext& ctx = getKernelContext();

            auto job = [pluginCopy,&ws,&ctx,trackIndex](uint32_t /*threadNumber*/)
            {
                StreamBundle* sourceTrack;
                if (ws.getMemorySaveMode()) {
                    ctx.getLogManager() << Kernel::LogLevel_Info << "Loading " << ws.getTrack(trackIndex)->getSource().c_str() << " from file\n";

                    sourceTrack = readStreamBundleFromFile(ctx, ws.getTrack(trackIndex)->getSource().c_str(), false);
                }
                else { sourceTrack = ws.getTrack(trackIndex); }


                if (!pluginCopy->process(*sourceTrack)) {
                    // log() << Kernel::LogLevel_Error << "Error processing track << " << (i + 1) << " with the plugin\n";
                    // retVal = false;
                }
                delete pluginCopy;

                if (ws.getMemorySaveMode()) {
                    if (sourceTrack->getDirtyBit()) {
                        // @fixme not a good solution with the filenaming; rethink the whole thing

                        std::stringstream ss;
                        ss << ws.getWorkingPath() << "/workspace-track-" << (trackIndex + 1) << ".ov";

                        // In this mode, sourceTrack is not from the track array. We spool it to disk, then
                        // set the filename of the track in the array, and reload it back.
                        saveStreamBundleToFile(ctx, sourceTrack, ss.str().c_str());
                        ws.getTrack(trackIndex)->setSource(ss.str());
                        ws.reloadTrack(trackIndex);
                    }

                    delete sourceTrack;
                }
            };

            m_executor.pushJob(job);

            // @todo copy sourcetrack, process the copy, if the processing is
            // successful then replace the original (or not) depending on inplacemode setting

            if (m_executor.getNumThreads() == 1) { m_executor.waitForAll(); }
        }
        // @todo in principle there's no need to freeze the GUI meanwhile, but allowing it to run in the bg would require locking
        m_executor.waitForAll();
    }
    else { log() << Kernel::LogLevel_Error << "Plugin does not have any known capabilities and cannot be run.\n"; }

    // pBoxAlgorithmDescriptor->release();

    return retVal;
}

bool CTracker::setNumThreads(uint32_t numThreads)
{
    if (numThreads != m_executor.getNumThreads()) {
        if (numThreads < 1) {
            log() << Kernel::LogLevel_Warning << "Minimum number of threads is 1, setting that.\n";
            numThreads = 1;
        }
        else if (numThreads > 1) {
            log() << Kernel::LogLevel_Info << "Using " << numThreads <<
                    " threads. Concurrency control has not been carefully tested. If you notice issues, switch to 1 thread.\n";
        }
        m_executor.uninitialize();
        return m_executor.initialize(numThreads);
    }

    return true;
}

bool CTracker::loadConfig(const CString& filename)
{
    if (!m_kernelCtx.getConfigurationManager().addConfigurationFromFile(filename)) {
        m_executor.initialize(1);

        return false;
    }

    const uint32_t numThreads = uint32_t(m_kernelCtx.getConfigurationManager().expandAsUInteger("${Tracker_NumThreads}", 1));
    m_executor.initialize(numThreads);
    //  m_Executor.launchTest();

    const CString workspaceFile = m_kernelCtx.getConfigurationManager().expand("${Tracker_Last_Workspace}");

    //if (m_kernelCtx.getConfigurationManager().lookUpConfigurationTokenIdentifier("Tracker_Last_Workspace"))
    //  != CIdentifier::undefined())
    //{
    //  m_Workspace.load(m_kernelCtx.getConfigurationManager().expand("${Tracker_Last_Workspace}");
    //  Tracker_Last_Workspace
    // )
    if (workspaceFile.length() != 0) { return m_workspace.load(workspaceFile.toASCIIString()); }

    return false;
}

// @note : does not save the workspace itself
bool CTracker::saveConfig(const CString& filename) const
{
    FILE* file = fopen(filename, "wt");
    if (file) {
        fprintf(file, "# Configuration file for OpenViBE Tracker, autosaved on Tracker exit\n");
        fprintf(file, "#\n");
        fprintf(file, "\n");
        fprintf(file, "# Last settings used in the Tracker\n");
        fprintf(file, "Tracker_Last_Workspace = %s\n", m_workspace.getFilename().toASCIIString());
        fprintf(file, "Tracker_NumThreads = %d\n", m_executor.getNumThreads());

        fclose(file);
    }
    else { return false; }

    return true;
}

#if 0
void testCode()
{
    Workspace wp(*kernelWrapper.m_kernelCtx);

//  TestClass tmp(*kernelWrapper.m_kernelCtx);

/*

    const CString eegFile = Directories::getDataDir() + CString("/scenarios/signals/bci-motor-imagery.ov");
//  const CString eegFile = CString("E:/jl/noise-test.ov");
    const CString scenarioFile = Directories::getDataDir() + CString("/applications/tracker/tracker-debug-display.xml");
    
    if(!wp.setTrack(eegFile.toASCIIString())) { return 2; }
    if(!wp.setprocessor(scenarioFile.toASCIIString())) { return 3; }

    // Push some chunks to selection
    Selection& selection = wp.m_track.m_Selection;
    selection.addRange(Range(3,5));
    selection.addRange(Range(9,11));

    if(!wp.play()) { return 4; }
*/  
}
#endif

}  // namespace Tracker
}  // namespace OpenViBE
