#pragma once

#include "CPluginObjectDescEnum.hpp"

#include <fstream>
#include <map>

namespace OpenViBE {
namespace PluginInspector {
class CAlgorithmGlobalDefinesGen final : public CPluginObjectDescEnum
{
public:
    CAlgorithmGlobalDefinesGen(const Kernel::IKernelContext& ctx, const std::string& filename);
    ~CAlgorithmGlobalDefinesGen() override;
    bool Callback(const Plugins::IPluginObjectDesc& pluginObjectDesc) override;

protected:
    std::ofstream m_file;

    std::map<CIdentifier, std::string> m_usedIDs;
    // Adds the define to m_usedIdentifiers and m_File
    void addIdentifier(const std::string& objectName, const CIdentifier& candidate, const std::string& spaces);
};
}  // namespace PluginInspector
}  // namespace OpenViBE
