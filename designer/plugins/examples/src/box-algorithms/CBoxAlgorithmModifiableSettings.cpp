#include "CBoxAlgorithmModifiableSettings.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Examples {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmModifiableSettings::processClock(Kernel::CMessageClock& /*msg*/)
{
    updateSettings();
    //print settings values
    for (size_t i = 0; i < m_settingsValue.size(); ++i) {
        this->getLogManager() << Kernel::LogLevel_Info << "Setting " << i << " value is " << m_settingsValue[i] << "\n";
    }
    this->getLogManager() << Kernel::LogLevel_Info << "\n";

    return true;
}

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmModifiableSettings::updateSettings()
{
    m_settingsValue.clear();
    const size_t nSetting = this->getStaticBoxContext().getSettingCount();
    for (size_t i = 0; i < nSetting; ++i) {
        CString value = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        m_settingsValue.push_back(value);
    }
    return true;
}

}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
