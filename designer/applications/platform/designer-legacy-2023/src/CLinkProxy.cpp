///-------------------------------------------------------------------------------------------------
/// 
/// \file CLinkProxy.cpp
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

#include "CLinkProxy.hpp"
#include "TAttributeHandler.hpp"
#include "../../../../../sdk/openvibe/include/openvibe/ov_defines.h"

namespace OpenViBE {
namespace Designer {

CLinkProxy::CLinkProxy(const Kernel::ILink& link) : m_constLink(&link)
{
    if (m_constLink) {
        const TAttributeHandler handler(*m_constLink);
        m_xSrc = handler.getAttributeValue<int>(OV_AttributeId_Link_XSrc);
        m_ySrc = handler.getAttributeValue<int>(OV_AttributeId_Link_YSrc);
        m_xDst = handler.getAttributeValue<int>(OV_AttributeId_Link_XDst);
        m_yDst = handler.getAttributeValue<int>(OV_AttributeId_Link_YDst);
    }
}

CLinkProxy::CLinkProxy(Kernel::IScenario& scenario, const CIdentifier& linkID)
    : m_constLink(scenario.getLinkDetails(linkID)), m_link(scenario.getLinkDetails(linkID))
{
    if (m_constLink) {
        const TAttributeHandler handler(*m_constLink);
        m_xSrc = handler.getAttributeValue<int>(OV_AttributeId_Link_XSrc);
        m_ySrc = handler.getAttributeValue<int>(OV_AttributeId_Link_YSrc);
        m_xDst = handler.getAttributeValue<int>(OV_AttributeId_Link_XDst);
        m_yDst = handler.getAttributeValue<int>(OV_AttributeId_Link_YDst);
    }
}

CLinkProxy::~CLinkProxy()
{
    if (m_link) {
        TAttributeHandler handler(*m_link);

        if (handler.HasAttribute(OV_AttributeId_Link_XSrc)) { handler.setAttributeValue<int>(OV_AttributeId_Link_XSrc, m_xSrc); }
        else { handler.addAttribute<int>(OV_AttributeId_Link_XSrc, m_xSrc); }

        if (handler.HasAttribute(OV_AttributeId_Link_YSrc)) { handler.setAttributeValue<int>(OV_AttributeId_Link_YSrc, m_ySrc); }
        else { handler.addAttribute<int>(OV_AttributeId_Link_YSrc, m_ySrc); }

        if (handler.HasAttribute(OV_AttributeId_Link_XDst)) { handler.setAttributeValue<int>(OV_AttributeId_Link_XDst, m_xDst); }
        else { handler.addAttribute<int>(OV_AttributeId_Link_XDst, m_xDst); }

        if (handler.HasAttribute(OV_AttributeId_Link_YDst)) { handler.setAttributeValue<int>(OV_AttributeId_Link_YDst, m_yDst); }
        else { handler.addAttribute<int>(OV_AttributeId_Link_YDst, m_yDst); }
    }
}

void CLinkProxy::SetSource(const int x, const int y)
{
    m_xSrc = x;
    m_ySrc = y;
}

void CLinkProxy::SetTarget(const int x, const int y)
{
    m_xDst = x;
    m_yDst = y;
}

}  // namespace Designer
}  // namespace OpenViBE
