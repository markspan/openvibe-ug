#include "CPluginObjectDescEnum.hpp"

#include <cctype>

namespace OpenViBE {
namespace PluginInspector {

bool CPluginObjectDescEnum::EnumeratePluginObjectDesc()
{
    CIdentifier id;
    while ((id = m_kernelCtx.getPluginManager().getNextPluginObjectDescIdentifier(id)) != CIdentifier::undefined()) { this->Callback(*m_kernelCtx.getPluginManager().getPluginObjectDesc(id)); }
    return true;
}

bool CPluginObjectDescEnum::EnumeratePluginObjectDesc(const CIdentifier& parentClassID)
{
    CIdentifier id;
    while ((id = m_kernelCtx.getPluginManager().getNextPluginObjectDescIdentifier(id, parentClassID)) !=
           CIdentifier::undefined()) { this->Callback(*m_kernelCtx.getPluginManager().getPluginObjectDesc(id)); }
    return true;
}

std::string CPluginObjectDescEnum::Transform(const std::string& in, const bool removeSlash)
{
    std::string out;
    bool lastWasSeparator = true;

    for (std::string::size_type i = 0; i < in.length(); ++i) {
        if (std::isalpha(in[i]) || std::isdigit(in[i]) || (!removeSlash && in[i] == '/')) {
            if (in[i] == '/') { out += "_"; }
            else { out += lastWasSeparator ? std::toupper(in[i]) : in[i]; }
            lastWasSeparator = false;
        }
        else {
            //if(!lastWasSeparator) { dst+="_"; }
            lastWasSeparator = true;
        }
    }
    return out;
}

}  // namespace PluginInspector
}  // namespace OpenViBE
