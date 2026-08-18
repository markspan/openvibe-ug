#include "ovkCScenarioSettingKeywordParserCallback.h"
#include <openvibe/ov_all.h>

#include <iostream>

namespace OpenViBE {
namespace Kernel {

bool CScenarioSettingKeywordParserCallback::expand(const CString& rStringToExpand, CString& rExpandedString) const
{
    // In the case there is no value present we return an empty string
    rExpandedString = "";

    // Expand the scenario directory
    if (rStringToExpand == CString("ScenarioDirectory"))
    {
        if (m_rScenario.hasAttribute(OV_AttributeId_ScenarioFilename))
        {
            const std::string filename = m_rScenario.getAttributeValue(OV_AttributeId_ScenarioFilename).toASCIIString();
            const size_t iDir          = filename.rfind('/');
            if (iDir != std::string::npos) { rExpandedString = CString(filename.substr(0, iDir).c_str()); }
        }
        return true;
    }
    // Expand settings from the scenario
    if (m_rScenario.hasSettingWithName(rStringToExpand))
    {
        m_rScenario.getSettingValue(rStringToExpand, rExpandedString);
        return true;
    }

    return false;
}

}  // namespace Kernel
}  // namespace OpenViBE
