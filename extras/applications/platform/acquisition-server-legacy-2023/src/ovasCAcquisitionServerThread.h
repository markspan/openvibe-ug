#pragma once

#include "ovasCAcquisitionServerGUI.h"
#include "ovasCAcquisitionServer.h"

#include <system/ovCTime.h>
#include <cmath> // std::abs() for Fedora

// fwd declare of gtk idle callbacks
static gboolean idle_updateimpedance_cb(void* data);
static gboolean idle_updatedrift_cb(void* data);
static gboolean idle_updatedisconnect_cb(void* data);
static gboolean idle_updatestatus_cb(void* data);

namespace OpenViBE {
namespace AcquisitionServer {
class CAcquisitionServerThread
{
public:
    enum class EStatus { Idle, Connected, Started, Finished };

    CAcquisitionServerThread(const Kernel::IKernelContext& ctx, CAcquisitionServerGUI& gui, CAcquisitionServer& acquisitionServer)
        : m_kernelCtx(ctx), m_gui(gui), m_acquisitionServer(acquisitionServer) { }  // Always call update on start 

    void main()
    {
        bool finished          = false;
        uint64_t lastGUIUpdate = 0;

        while (!finished) {
            bool shouldSleep      = false;
            bool shouldDisconnect = false;
            size_t clientCount;
            double driftMs;

            {
                DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

                clientCount = m_acquisitionServer.getClientCount();
                driftMs     = (m_status == EStatus::Started ? m_acquisitionServer.m_DriftCorrection.getDriftMs() : 0);


                switch (m_status) {
                    case EStatus::Idle: shouldSleep = true;
                        break;

                    case EStatus::Connected:
                    case EStatus::Started:
                    {
                        if (!m_acquisitionServer.loop()) { shouldDisconnect = true; }
                        else { for (size_t i = 0; i < m_impedances.size(); ++i) { m_impedances[i] = m_acquisitionServer.getImpedance(i); } }
                    }
                    break;

                    case EStatus::Finished: finished = true;
                        break;

                    default: break;
                }
            }

            if (!finished) {
                if (m_acquisitionServer.isImpedanceCheckRequested()) {
                    // In the impedance check mode, we need to yield the thread to get reliable redraw
                    shouldSleep = true;
                }

                // Update the GUI if the variables have changed. In order to avoid 
                // gdk_threads_enter()/gdk_threads_exit() calls that may not work on all 
                // backends (esp. Windows), delegate the work to g_idle_add() functions.
                // As a result, we need to protect access to the variables that the callbacks use;
                // this protection is done inside the callbacks.
                if (!std::equal(m_impedances.begin(), m_impedances.end(), m_impedancesLast.begin(),
                                [](const double a, const double b) { return (std::abs(a - b) < 0.01); })) {
                    m_impedancesLast = m_impedances;

                    gdk_threads_add_idle(idle_updateimpedance_cb, static_cast<void*>(this));
                }

                if (m_lastStatus != m_status || clientCount != m_nClient) {
                    m_lastStatus = m_status;
                    m_nClient    = clientCount;
                    gdk_threads_add_idle(idle_updatestatus_cb, static_cast<void*>(this));
                }

                if (shouldDisconnect) { gdk_threads_add_idle(idle_updatedisconnect_cb, static_cast<void*>(this)); }

                // update fast changing variables at most every 0.25 sec to avoid hammering the gui
                const uint64_t now = System::Time::zgetTime();
                if (now - lastGUIUpdate > (1LL << 32) / 4LL) {
                    if (std::abs(driftMs - m_lastDriftMs) > 0) {
                        m_lastDriftMs = driftMs;

                        gdk_threads_add_idle(idle_updatedrift_cb, static_cast<void*>(this));
                    }

                    lastGUIUpdate = now;
                }

                if (shouldSleep) { System::Time::sleep(100); }
            }
        }

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread:: thread finished\n";
    }

    bool connect()
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::connect()\n";

        if (!m_acquisitionServer.connect(m_gui.getDriver(), m_gui.getHeaderCopy(), m_gui.getSampleCountPerBuffer(), m_gui.getTCPPort())) { return false; }
        m_status = EStatus::Connected;

        {
            m_impedances.resize(m_gui.getHeaderCopy().getChannelCount(), OVAS_Impedance_NotAvailable);
            m_impedancesLast.resize(m_gui.getHeaderCopy().getChannelCount(), OVAS_Impedance_NotAvailable);
        }
        return true;
    }

    bool start()
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::start()\n";
        if (!m_acquisitionServer.start()) {
            m_acquisitionServer.stop();
            return false;
        }
        m_status = EStatus::Started;
        return true;
    }

    bool stop()
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::stop()\n";
        m_acquisitionServer.stop();
        m_status = EStatus::Connected;
        return true;
    }

    bool disconnect()
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::disconnect()\n";

        if (m_status == EStatus::Started) { m_acquisitionServer.stop(); }

        m_acquisitionServer.disconnect();

        m_impedances.clear();
        m_impedancesLast.clear();

        m_status = EStatus::Idle;
        return true;
    }

    bool terminate()
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::terminate()\n";

        switch (m_status) {
            case EStatus::Started: m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::stop()\n";
                m_acquisitionServer.stop();
            case EStatus::Connected: m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::disconnect()\n";
                m_acquisitionServer.disconnect();
            default: break;
        }

        m_status = EStatus::Finished;
        return true;
    }

    // GTK C callbacks call these from the main thread to update the GUI
    void updateGUIImpedance()
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);
        for (size_t i = 0; i < m_impedancesLast.size(); ++i) { m_gui.setImpedance(i, m_impedancesLast[i]); }
    }

    void updateGUIDrift() const
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);
        m_gui.setDriftMs(m_lastDriftMs);
    }

    void updateGUIStatus() const
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        std::string state;
        std::string text;

        switch (m_status) {
            case EStatus::Started: state = (m_nClient == 0) ? "Receiving..." : "Receiving and sending...";
                text = std::to_string(m_nClient) + " client" + (m_nClient != 1 ? "s" : "") + " connected";
                break;
            case EStatus::Idle: state = "Disconnected";
                break;
            case EStatus::Connected: state = "Connected to device!";
                text = "Press play to start...";
                break;
            default: state = "Unknown";
                break;
        }
        m_gui.setStateText(state.c_str());
        m_gui.setClientText(text.c_str());
    }

    void updateGUIDisconnect() const
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);
        m_gui.disconnect();
    }

    /*
    uint32_t getStatus()
    {
        DoubleLock lock(&m_AcquisitionServer.m_ProtectionMutex, &m_AcquisitionServer.m_ExecutionMutex); 
        return m_status;
    }
    */
protected:
    const Kernel::IKernelContext& m_kernelCtx;
    CAcquisitionServerGUI& m_gui;
    CAcquisitionServer& m_acquisitionServer;
    EStatus m_status     = EStatus::Idle;
    EStatus m_lastStatus = EStatus::Idle;

    size_t m_nClient     = 0;
    double m_lastDriftMs = -1.0;
    std::vector<double> m_impedancesLast;
    std::vector<double> m_impedances;
};

class CAcquisitionServerThreadHandle
{
public:
    explicit CAcquisitionServerThreadHandle(CAcquisitionServerThread& thread) : m_thread(thread) { }
    void operator()() const { m_thread.main(); }

protected:
    CAcquisitionServerThread& m_thread;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

static gboolean idle_updateimpedance_cb(void* data)
{
    static_cast<OpenViBE::AcquisitionServer::CAcquisitionServerThread*>(data)->updateGUIImpedance();
    return FALSE; // don't call again
}

static gboolean idle_updatedrift_cb(void* data)
{
    static_cast<OpenViBE::AcquisitionServer::CAcquisitionServerThread*>(data)->updateGUIDrift();
    return FALSE; // don't call again
}

static gboolean idle_updatestatus_cb(void* data)
{
    static_cast<OpenViBE::AcquisitionServer::CAcquisitionServerThread*>(data)->updateGUIStatus();
    return FALSE; // don't call again
}

static gboolean idle_updatedisconnect_cb(void* data)
{
    static_cast<OpenViBE::AcquisitionServer::CAcquisitionServerThread*>(data)->updateGUIDisconnect();
    return FALSE; // don't call again
}
