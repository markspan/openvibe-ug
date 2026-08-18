#pragma once

#include <ov_common_defines.h>

namespace OpenViBE {
class CString;

namespace Kernel {
/**
 * \class IConfigurationKeywordExpandCallback
 * \author Jozef Legeny (Mensia Technologies)
 * \date 2014-05-06
 * \brief Callback used for overriding a keyword in IConfigurationManager
 * \ingroup Group_Config
 * \ingroup Group_Kernel
 */
class OV_API IConfigurationKeywordExpandCallback
{
public:

    virtual ~IConfigurationKeywordExpandCallback() {}
    virtual bool expand(const CString& in, CString& out) const = 0;
};
}  // namespace Kernel
}  // namespace OpenViBE
