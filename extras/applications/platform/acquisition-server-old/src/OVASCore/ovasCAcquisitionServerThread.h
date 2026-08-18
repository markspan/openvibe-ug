/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPLL-3.0-only
*/
#pragma once

#include "ovasCAcquisitionServer.h"

#include <system/ovCTime.h>
#include <cmath> // std::abs() for Fedora

namespace OpenViBE {
namespace AcquisitionServer {
class CAcquisitionServerThread
{
public:
    enum class EStatus { Idle, Connected, Started, Finished };

    CAcquisitionServerThread(const Kernel::IKernelContext& ctx, CAcquisitionServer& acquisitionServer)
        : m_kernelCtx(ctx), m_acquisitionServer(acquisitionServer) { }  // Always call update on start

    void main()
    {
        bool finished          = false;

        while (!finished) {
            bool shouldSleep      = false;
            bool shouldDisconnect = false;
            size_t clientCount;
            {
                DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

                clientCount = m_acquisitionServer.getClientCount();

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
                    if(m_impedanceCallback) { 
                        m_impedanceCallback(m_impedances);
                    }
                }

                if (m_lastStatus != m_status || clientCount != m_nClient) {
                    m_lastStatus = m_status;
                    m_nClient    = clientCount;
                    // Todo: add mecanism to update UI with status & client count
                }

                if (shouldDisconnect) {
                    // Todo: add mecanism to perform disconnect on main thread (and update UI)
                }

                if (shouldSleep) {
                    System::Time::sleep(100);
                }
            }
        }

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread:: thread finished\n";
    }

    bool connect(IDriver& driver, const uint32_t nSamplingPerSentBlock, const uint32_t port)
    {
        DoubleLock lock(&m_acquisitionServer.m_ProtectionMutex, &m_acquisitionServer.m_ExecutionMutex);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "CAcquisitionServerThread::connect()\n";

        if (!m_acquisitionServer.connect(driver, nSamplingPerSentBlock, port)) {
            return false;
        }
        m_status = EStatus::Connected;

        {
            m_impedances.resize(driver.getHeader()->getChannelCount(), OVAS_Impedance_NotAvailable);
            m_impedancesLast.resize(driver.getHeader()->getChannelCount(), OVAS_Impedance_NotAvailable);
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

    std::function<void(const std::vector<double>&)> m_impedanceCallback;

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    CAcquisitionServer& m_acquisitionServer;
    EStatus m_status     = EStatus::Idle;
    EStatus m_lastStatus = EStatus::Idle;

    size_t m_nClient     = 0;
    double m_lastDriftMs = -1.0;
    std::vector<double> m_impedancesLast;
    std::vector<double> m_impedances;
};

}  // namespace AcquisitionServer
}  // namespace OpenViBE
