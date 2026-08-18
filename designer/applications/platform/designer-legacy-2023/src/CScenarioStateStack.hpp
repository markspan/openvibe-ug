///-------------------------------------------------------------------------------------------------
/// 
/// \file CScenarioStateStack.hpp
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

#include <list>

#include "base.hpp"

namespace OpenViBE {
namespace Designer {
class CInterfacedScenario;

class CScenarioStateStack
{
public:
    CScenarioStateStack(const Kernel::IKernelContext& ctx, CInterfacedScenario& interfacedScenario, Kernel::IScenario& scenario);
    ~CScenarioStateStack() { for (const auto& state : m_states) { delete state; } }

    bool IsUndoPossible() { return m_currentState != m_states.begin(); }
    bool Undo();
    bool IsRedoPossible();
    bool Redo();
    void DropLastState() { m_states.pop_back(); }

    bool Snapshot();

private:
    bool restoreState(const CMemoryBuffer& state) const;
    bool dumpState(CMemoryBuffer& state) const;

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    CInterfacedScenario& m_interfacedScenario;
    Kernel::IScenario& m_scenario;

    std::list<CMemoryBuffer*> m_states;
    std::list<CMemoryBuffer*>::iterator m_currentState;

    size_t m_nMaximumState = 0;
};
}  // namespace Designer
}  // namespace OpenViBE
