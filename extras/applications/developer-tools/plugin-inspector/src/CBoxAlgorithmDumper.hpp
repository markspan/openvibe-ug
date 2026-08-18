#pragma once

// 
// Dumps all registered box algorithms to a text file or cout. The intended use case
// is to check the current scenarios to see if all boxes have at least one 
// scenario where it is used. n.b. The checking was done by a shell script in 
//
// test/check-scenario-coverage.sh
//

#include "CPluginObjectDescEnum.hpp"

#include <fstream>

namespace OpenViBE {
namespace PluginInspector {
// ------------------------------------------------------------------------------------------------------------------------------------
class CBoxAlgorithmDumper final : public CPluginObjectDescEnum
{
public:
    CBoxAlgorithmDumper(const Kernel::IKernelContext& ctx, const std::string& outFile);
    ~CBoxAlgorithmDumper() override { if (m_writeToFile) { m_file.close(); } }
    bool Callback(const Plugins::IPluginObjectDesc& pluginObjectDesc) override;

protected:
    std::ofstream m_file;
    bool m_writeToFile = true;  // if false, to console
};
}  // namespace PluginInspector
}  // namespace OpenViBE
