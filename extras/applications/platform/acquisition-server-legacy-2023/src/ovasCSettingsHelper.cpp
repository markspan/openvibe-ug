#include "ovasCSettingsHelper.h"

namespace OpenViBE {
namespace AcquisitionServer {

// Save all registered variables to the configuration manager
void SettingsHelper::save()
{
    for (auto it = m_properties.begin(); it != m_properties.end(); ++it) {
        std::stringstream ss;

        ss << *(it->second);

        // m_rContext.getLogManager() << Kernel::LogLevel_Info << "Token " << it->first << " wrote [" << ss.str() << "]\n";
        const CString name = m_prefix + CString("_") + it->first;
        CIdentifier id     = m_configManager.lookUpConfigurationTokenIdentifier(name);
        if (id == CIdentifier::undefined()) { m_configManager.createConfigurationToken(m_prefix + CString("_") + it->first, CString(ss.str().c_str())); }
        else {
            // replacing token value
            m_configManager.setConfigurationTokenValue(id, CString(ss.str().c_str()));
        }
    }
}

// Load all registered variables from the configuration manager
void SettingsHelper::load()
{
    for (auto it = m_properties.begin(); it != m_properties.end(); ++it) {
        const CString name = m_prefix + CString("_") + it->first;
        if (m_configManager.lookUpConfigurationTokenIdentifier(name) != CIdentifier::undefined()) {
            const CString value = m_configManager.expand(CString("${") + name + CString("}"));

            // m_rContext.getLogManager() << Kernel::LogLevel_Info << "Token " << it->first << " found, mgr setting = [" << value << "]\n";

            // Note that we have to accept empty strings as the user may have intended to keep the token empty. So we do not check here.
            std::stringstream ss;
            ss << value.toASCIIString();
            ss >> *(it->second);

            // m_rContext.getLogManager() << Kernel::LogLevel_Info << "Token " << it->first << " inserted as [" << *(it->second) << "]\n";
        }
        // else { } // token not found
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
