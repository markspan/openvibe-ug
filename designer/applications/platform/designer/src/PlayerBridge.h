/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <openvibe/ov_all.h>
#include <thread>
#include <future>
#include <atomic>
#include <functional>

namespace OpenViBE {
namespace Designer {

//bridge to play, pause, stop a player in a thread
// it uses historic CPlayer class to play a scenario but within a separate thread
// this was historically handled by gtk  (with mecanism g_idle_source_new  and g_source_set_callback )
// For the future, CPlayer could be incorporated into this class to directly call scheduler.loop
class PlayerBridge {
    public:
    PlayerBridge(Kernel::IKernelContext* ctx, Kernel::IPlayer *player = nullptr);
    ~PlayerBridge();

    bool isNull() { return m_player == nullptr; }

    Kernel::EPlayerReturnCodes initialize() { return m_player->initialize(); }
    bool uninitialize() { return m_player->uninitialize(); }
    void setStatusChangedCallback(std::function<void()> stateChangedCallback) { m_stateChangedCallback = stateChangedCallback; }

    Kernel::EPlayerStatus getStatus() const { return m_player->getStatus(); }
    double getCurrentSimulatedTime() const { return CTime(m_player->getCurrentSimulatedTime()).toSeconds(); }
    bool setScenario(const CIdentifier& scenarioID) { return m_player->setScenario(scenarioID); }
    bool setWindowManager(const CIdentifier& windowManagerID);

    void clear();

    void setPlayer(Kernel::IPlayer* player) { m_player = player; }
    double getCPUUsage() const { return m_player ? m_player->getCPUUsage() : -1.0; }

    bool start(Kernel::EPlayerStatus status = Kernel::EPlayerStatus::Play, std::function<void()> onEnd = nullptr);
    void stop();

    void pause();
    bool threadFinished=false;

protected:
    void looping();

    enum class PendingState {
        None,
        Play,
        Forward,
        Step,
        Pause,
        Stop
    };
    std::atomic<PendingState> m_pendingState = PendingState::None;
    std::function<void()> m_stateChangedCallback;

    std::thread m_thread;
    uint64_t m_lastLoopTime = uint64_t(-1);
    Kernel::IKernelContext* m_context = nullptr;
    Kernel::IPlayer* m_player = nullptr;
};

}
}