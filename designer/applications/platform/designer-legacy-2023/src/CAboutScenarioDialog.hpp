///-------------------------------------------------------------------------------------------------
/// 
/// \file CAboutScenarioDialog.hpp
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
class CAboutScenarioDialog final
{
public:
    CAboutScenarioDialog(const Kernel::IKernelContext& ctx, Kernel::IScenario& scenario, const char* guiFilename)
        : m_kernelCtx(ctx), m_scenario(scenario), m_guiFilename(guiFilename) { }

    ~CAboutScenarioDialog() = default;

    bool Run() const;

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    Kernel::IScenario& m_scenario;
    CString m_guiFilename;

    CAboutScenarioDialog() = delete;
};
}  // namespace Designer
}  // namespace OpenViBE
