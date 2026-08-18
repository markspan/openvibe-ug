#pragma once

#include "CPluginObjectDescEnum.hpp"
#include "CBoxQmlRenderer.hpp"

#include <map>
#include <vector>
#include <string>

namespace OpenViBE {
namespace PluginInspector {
// ------------------------------------------------------------------------------------------------------------------------------------
class CBoxAlgorithmSnapshotGen final : public CPluginObjectDescEnum
{
public:
    CBoxAlgorithmSnapshotGen(const Kernel::IKernelContext& ctx, const std::string& snapshotDir, const std::string& docTemplateDir);
    ~CBoxAlgorithmSnapshotGen() override;
    bool Callback(const Plugins::IPluginObjectDesc& pod) override;

protected:
    std::string m_snapshotDir;
    std::string m_docTemplateDir;
    std::vector<std::pair<std::string, std::string>> m_categories;
    CBoxQmlRenderer m_renderer;
    CIdentifier m_scenarioID      = CIdentifier::undefined();
    Kernel::IScenario* m_scenario = nullptr;
};
}  // namespace PluginInspector
}  // namespace OpenViBE
