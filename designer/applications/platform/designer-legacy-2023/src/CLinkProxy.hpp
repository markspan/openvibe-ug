///-------------------------------------------------------------------------------------------------
/// 
/// \file CLinkProxy.hpp
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
class CLinkProxy final
{
public:
    explicit CLinkProxy(const Kernel::ILink& link);
    CLinkProxy(Kernel::IScenario& scenario, const CIdentifier& linkID);
    ~CLinkProxy();

    explicit operator Kernel::ILink*() const { return m_link; }
    explicit operator const Kernel::ILink*() const { return m_constLink; }

    int GetXSource() const { return m_xSrc; }
    int GetYSource() const { return m_ySrc; }
    int GetXTarget() const { return m_xDst; }
    int GetYTarget() const { return m_yDst; }

    void SetSource(int x, int y);
    void SetTarget(int x, int y);

protected:
    const Kernel::ILink* m_constLink;
    Kernel::ILink* m_link = nullptr;
    int m_xSrc            = 0;
    int m_ySrc            = 0;
    int m_xDst            = 0;
    int m_yDst            = 0;
};
}  // namespace Designer
}  // namespace OpenViBE
