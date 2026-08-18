#pragma once

#include "../ovkTKernelObject.h"
#include <system/ovCChrono.h>
#include <map>
#include <list>

namespace OpenViBE {
namespace Kernel {
enum class ESchedulerInitialization { Success, BoxInitializationFailed, Failed };

class CSimulatedBox;
class CChunk;
class CPlayer;

class CScheduler final : public TKernelObject<IKernelObject>
{
public:

    CScheduler(const IKernelContext& ctx, CPlayer& player);
    ~CScheduler() override;

    bool setScenario(const CIdentifier& scenarioID);
    bool setFrequency(uint64_t frequency);

    bool isHoldingResources() const;

    ESchedulerInitialization initialize();
    bool uninitialize();
    bool loop();

    bool sendInput(const CChunk& chunk, const CIdentifier& boxId, size_t index);
    uint64_t getCurrentTime() const { return m_currentTime; }
    uint64_t getCurrentLateness() const;
    uint64_t getFrequency() const { return m_frequency; }
    uint64_t getStepDuration() const { return m_stepDuration; }
    double getCPUUsage() const { return (const_cast<System::CChrono&>(m_oBenchmarkChrono)).getStepInPercentage(); }
    double getFastForwardMaximumFactor() const;

    _IsDerivedFromClass_Final_(TKernelObject<IKernelObject>, OVK_ClassId_Kernel_Player_Scheduler)

    CPlayer& getPlayer() const { return m_rPlayer; }

protected:

    CPlayer& m_rPlayer;
    CIdentifier m_scenarioID = CIdentifier::undefined();
    IScenario* m_scenario    = nullptr;
    size_t m_steps           = 0;
    uint64_t m_frequency     = 0;
    uint64_t m_stepDuration  = 0;
    uint64_t m_currentTime   = 0;

    std::map<std::pair<int, CIdentifier>, CSimulatedBox*> m_simulatedBoxes;
    std::map<CIdentifier, System::CChrono> m_simulatedBoxChronos;
    std::map<CIdentifier, std::map<size_t, std::list<CChunk>>> m_simulatedBoxInputs;

private:

    void handleException(const CSimulatedBox* box, const char* errorHint, const std::exception& exception);
    bool processBox(CSimulatedBox* simulatedBox, const CIdentifier& boxID);
    bool flattenScenario();
    System::CChrono m_oBenchmarkChrono;
};
}  // namespace Kernel
}  // namespace OpenViBE
