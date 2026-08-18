///-------------------------------------------------------------------------------------------------
/// 
/// \file CAboutPluginDialog.hpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "base.hpp"

namespace OpenViBE {
namespace Designer {
class CAboutPluginDialog final
{
public:
    CAboutPluginDialog(const Kernel::IKernelContext& ctx, const CIdentifier& pluginClassID, const char* guiFilename)
        : m_kernelCtx(ctx), m_pluginClassID(pluginClassID), m_guiFilename(guiFilename) { }

    CAboutPluginDialog(const Kernel::IKernelContext& ctx, const Plugins::IPluginObjectDesc* pod, const char* guiFilename)
        : m_kernelCtx(ctx), m_pluginClassID(CIdentifier::undefined()), m_guiFilename(guiFilename), m_pods(pod) { }

    ~CAboutPluginDialog() = default;

    bool Run();

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    CIdentifier m_pluginClassID = CIdentifier::undefined();
    CString m_guiFilename;
    const Plugins::IPluginObjectDesc* m_pods = nullptr;

    CAboutPluginDialog() = delete;
};
}  // namespace Designer
}  // namespace OpenViBE
