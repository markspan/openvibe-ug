/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "PlayerBridge.h"

#include <system/ovCTime.h>
#include <openvibe/ov_all.h>
#include <clocale>
#include <thread>
#include <future>

namespace OpenViBE {
namespace Designer {

PlayerBridge::PlayerBridge(Kernel::IKernelContext* ctx, Kernel::IPlayer *player) :
    m_context(ctx),
    m_player(player)
{
}

PlayerBridge::~PlayerBridge() 
{
    clear();
}

void PlayerBridge::looping() {
    uint64_t lastLoopTime = uint64_t(-1);
    m_context->getLogManager() << Kernel::LogLevel_Debug << "PlayerBridge::looping: while loop starting, player status: " << toString(m_player->getStatus()) << "\n";

    Kernel::EPlayerStatus lastKnownStatus = m_player->getStatus();

    while(true) {
        // Update player state before next loop call.
        // Avoids race conditions between UI and this thread.
        PendingState pendingState = m_pendingState.exchange(PendingState::None);
        bool stopRequested = false;
        switch(pendingState) {
            case PendingState::Play:
                m_player->play();
                break;
            case PendingState::Forward:
                m_player->setFastForwardMaximumFactor(100.0);
                m_player->forward();
                break;
            case PendingState::Step:
                m_player->setFastForwardMaximumFactor(1.0);
                m_player->step();
                break;
            case PendingState::Pause:
                m_player->pause();
                break;
            case PendingState::Stop:
                m_player->stop();
                stopRequested = true;
                break;
            default:
                break;
        }

        if (!stopRequested) {
            const uint64_t time = System::Time::zgetTime();
            if (lastLoopTime == uint64_t(-1)) {
                lastLoopTime = time;
            }
            if (!m_player->loop(time - lastLoopTime)) {
                m_context->getLogManager() << Kernel::LogLevel_Error << "The player loop failed. Check the above log messages to get the issue.\n";
                // CPlayer::loop() has already forced m_status to Stop on scheduler failure;
                // the status check below will catch it.
            }
            lastLoopTime = time;
        }

        // Single reconciliation point: catches UI-driven transitions (pendingState above)
        // AND status changes that happen *inside* m_player->loop() without going through the
        // bridge at all -- e.g. a box algorithm calling IPlayerContext::stop() synchronously
        // during the scheduler tick, or CPlayer::loop() forcing Stop on scheduler failure.
        const Kernel::EPlayerStatus currentStatus = m_player->getStatus();
        if (currentStatus != lastKnownStatus) {
            lastKnownStatus = currentStatus;
            m_stateChangedCallback();
        }

        if (currentStatus == Kernel::EPlayerStatus::Stop) {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool PlayerBridge::setWindowManager(const CIdentifier& windowManagerID)
{
    if(!m_player) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "Player is not set! \n";
        return false;
    }
    m_player->getRuntimeConfigurationManager().createConfigurationToken("VisualizationContext_WindowManagerID", windowManagerID.toString());
    return true;
}

bool PlayerBridge::start(Kernel::EPlayerStatus status, std::function<void()> onEnd)
{
    if(!m_player) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "Player is not set! \n";
        return false;
    }

    switch(status) {
        case Kernel::EPlayerStatus::Play:
            m_pendingState.store(PendingState::Play);
            break;
        case Kernel::EPlayerStatus::Step:
            m_pendingState.store(PendingState::Step);
            break;
        case Kernel::EPlayerStatus::Forward:
    //between 1 and 1000 , increment 0.1
    //TODO it's a GUI parameter
            m_pendingState.store(PendingState::Forward);
            break;
        default:
            m_context->getLogManager() << Kernel::LogLevel_Error << "Player status not recognized! " 
                                       << toString(status) << "  \n";
            return false;
            break;
    }

    // If thread is already running, do nothing
    if (!m_thread.joinable()) {
        threadFinished = false;
        m_thread = std::thread([this, onEnd]() {
            std::setlocale(LC_ALL, "C"); // C uses '.' as decimal separator
            this->looping();
            if(onEnd) {
                onEnd();
            }
            threadFinished = true;
        });
    }
    return true;

}

void PlayerBridge::stop()
{
    m_context->getLogManager() << Kernel::LogLevel_Debug << "PlayerBridge::stop\n";
    m_pendingState.store(PendingState::Stop);
    if(m_thread.joinable()) {
        m_thread.join();
    }
}

void PlayerBridge::clear()
{
    stop();
    m_player = nullptr;
}

void PlayerBridge::pause()
{
    m_pendingState.store(PendingState::Pause);
}

}
}