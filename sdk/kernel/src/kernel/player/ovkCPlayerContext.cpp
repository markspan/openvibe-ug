#include "ovkCPlayerContext.h"
#include "ovkCSimulatedBox.h"
#include "ovkCScheduler.h"
#include "ovkCPlayer.h"

namespace OpenViBE {
namespace Kernel {

CPlayerContext::CPlayerContext(const IKernelContext& ctx, CSimulatedBox* pSimulatedBox)
    : TKernelObject<IPlayerContext>(ctx), m_simulatedBox(*pSimulatedBox), m_pluginManager(ctx.getPluginManager()),
      m_algorithmManager(ctx.getAlgorithmManager()), m_configManager(ctx.getConfigurationManager()),
      m_logManager(ctx.getLogManager()), m_errorManager(ctx.getErrorManager()), m_scenarioManager(ctx.getScenarioManager()),
      m_typeManager(ctx.getTypeManager()), m_boxLogManager(*this, m_logManager, m_simulatedBox) {}

uint64_t CPlayerContext::getCurrentTime() const { return m_simulatedBox.getScheduler().getCurrentTime(); }
uint64_t CPlayerContext::getCurrentLateness() const { return m_simulatedBox.getScheduler().getCurrentLateness(); }
double CPlayerContext::getCurrentCPUUsage() const { return m_simulatedBox.getScheduler().getCPUUsage(); }
double CPlayerContext::getCurrentFastForwardMaximumFactor() const { return m_simulatedBox.getScheduler().getFastForwardMaximumFactor(); }

bool CPlayerContext::stop() { return m_simulatedBox.getScheduler().getPlayer().stop(); }
bool CPlayerContext::pause() { return m_simulatedBox.getScheduler().getPlayer().pause(); }
bool CPlayerContext::play() { return m_simulatedBox.getScheduler().getPlayer().play(); }
bool CPlayerContext::forward() { return m_simulatedBox.getScheduler().getPlayer().forward(); }

EPlayerStatus CPlayerContext::getStatus() const { return m_simulatedBox.getScheduler().getPlayer().getStatus(); }

}  // namespace Kernel
}  // namespace OpenViBE
