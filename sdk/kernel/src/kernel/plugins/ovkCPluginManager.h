#pragma once

#include "../ovkTKernelObject.h"

#include <vector>
#include <map>
#include <mutex>

namespace OpenViBE {
namespace Kernel {
class CPluginManager final : public TKernelObject<IPluginManager>
{
public:

    explicit CPluginManager(const IKernelContext& ctx) : TKernelObject<IPluginManager>(ctx) {}
    ~CPluginManager() override;
    bool addPluginsFromFiles(const CString& rFileNameWildCard) override;
    bool registerPluginDesc(const Plugins::IPluginObjectDesc& rPluginObjectDesc) override;
    CIdentifier getNextPluginObjectDescIdentifier(const CIdentifier& previousID) const override;
    CIdentifier getNextPluginObjectDescIdentifier(const CIdentifier& previousID, const CIdentifier& baseClassID) const override;
    bool canCreatePluginObject(const CIdentifier& classID) override;
    const Plugins::IPluginObjectDesc* getPluginObjectDesc(const CIdentifier& classID) const override;
    const Plugins::IPluginObjectDesc* getPluginObjectDescCreating(const CIdentifier& classID) const override;
    CIdentifier getPluginObjectHashValue(const CIdentifier& classID) const override;
    CIdentifier getPluginObjectHashValue(const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc) const override;
    bool isPluginObjectFlaggedAsDeprecated(const CIdentifier& classID) const override;
    Plugins::IPluginObject* createPluginObject(const CIdentifier& classID) override;
    bool releasePluginObject(Plugins::IPluginObject* pluginObject) override;
    Plugins::IAlgorithm* createAlgorithm(const CIdentifier& classID, const Plugins::IAlgorithmDesc** algorithmDesc) override;
    Plugins::IAlgorithm* createAlgorithm(const Plugins::IAlgorithmDesc& algorithmDesc) override;
    Plugins::IBoxAlgorithm* createBoxAlgorithm(const CIdentifier& classID, const Plugins::IBoxAlgorithmDesc** boxAlgorithmDesc) override;

    _IsDerivedFromClass_Final_(TKernelObject<IPluginManager>, OVK_ClassId_Kernel_Plugins_PluginManager)

protected:

    template <class TPluginObject, class TPluginObjectDesc>
    TPluginObject* createPluginObjectT(const CIdentifier& classID, const TPluginObjectDesc** ppPluginObjectDescT);

    std::vector<IPluginModule*> m_pluginModules;
    std::map<Plugins::IPluginObjectDesc*, IPluginModule*> m_pluginObjectDescs;
    std::map<Plugins::IPluginObjectDesc*, std::vector<Plugins::IPluginObject*>> m_pluginObjects;

    mutable std::mutex m_mutex;
};
}  // namespace Kernel
}  // namespace OpenViBE
