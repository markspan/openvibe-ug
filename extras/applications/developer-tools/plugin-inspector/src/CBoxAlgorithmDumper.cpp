#include "CBoxAlgorithmDumper.hpp"

#include <iostream>
#include <fstream>

namespace OpenViBE {
namespace PluginInspector {
// ------------------------------------------------------------------------------------------------------------------------------------
CBoxAlgorithmDumper::CBoxAlgorithmDumper(const Kernel::IKernelContext& ctx, const std::string& outFile)
    : CPluginObjectDescEnum(ctx)
{
    if (outFile.empty()) { m_writeToFile = false; }
    if (m_writeToFile) { m_file.open(outFile.c_str()); }
}
// ------------------------------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------------------------------
bool CBoxAlgorithmDumper::Callback(const Plugins::IPluginObjectDesc& pluginObjectDesc)
{
    const CIdentifier boxID = pluginObjectDesc.getCreatedClassIdentifier();
    (m_writeToFile ? m_file : std::cout) << "BoxAlgorithm " << Transform(pluginObjectDesc.getName().toASCIIString()) << " " << boxID.str() << "\n";
    return true;
}

}  // namespace PluginInspector
}  // namespace OpenViBE
