///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommentProxy.cpp
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

#include "CCommentProxy.hpp"
#include "TAttributeHandler.hpp"

namespace OpenViBE {
namespace Designer {

CCommentProxy::CCommentProxy(const Kernel::IKernelContext& ctx, const Kernel::IComment& comment)
    : m_kernelCtx(ctx), m_constComment(&comment)
{
    if (m_constComment) {
        const TAttributeHandler handler(*m_constComment);
        m_centerX = handler.getAttributeValue<int>(OV_AttributeId_Comment_XCenterPosition);
        m_centerY = handler.getAttributeValue<int>(OV_AttributeId_Comment_YCenterPosition);
    }
}

CCommentProxy::CCommentProxy(const Kernel::IKernelContext& ctx, Kernel::IScenario& scenario, const CIdentifier& commentID)
    : m_kernelCtx(ctx), m_constComment(scenario.getCommentDetails(commentID)), m_comment(scenario.getCommentDetails(commentID))
{
    if (m_constComment) {
        const TAttributeHandler handler(*m_constComment);
        m_centerX = handler.getAttributeValue<int>(OV_AttributeId_Comment_XCenterPosition);
        m_centerY = handler.getAttributeValue<int>(OV_AttributeId_Comment_YCenterPosition);
    }
}

int CCommentProxy::GetWidth(GtkWidget* widget) const
{
    int x, y;
    updateSize(widget, GetLabel(), &x, &y);
    return x;
}

int CCommentProxy::GetHeight(GtkWidget* widget) const
{
    int x, y;
    updateSize(widget, GetLabel(), &x, &y);
    return y;
}

void CCommentProxy::SetCenter(const int centerX, const int centerY)
{
    m_centerX = centerX;
    m_centerY = centerY;
    m_applied = false;
}

void CCommentProxy::Apply()
{
    if (m_comment) {
        TAttributeHandler handler(*m_comment);

        if (handler.HasAttribute(OV_AttributeId_Comment_XCenterPosition)) { handler.setAttributeValue<int>(OV_AttributeId_Comment_XCenterPosition, m_centerX); }
        else { handler.addAttribute<int>(OV_AttributeId_Comment_XCenterPosition, m_centerX); }

        if (handler.HasAttribute(OV_AttributeId_Comment_YCenterPosition)) { handler.setAttributeValue<int>(OV_AttributeId_Comment_YCenterPosition, m_centerY); }
        else { handler.addAttribute<int>(OV_AttributeId_Comment_YCenterPosition, m_centerY); }
        m_applied = true;
    }
}

const char* CCommentProxy::GetLabel() const
{
    m_label = m_constComment->getText().toASCIIString();
    return m_label.c_str();
}

void CCommentProxy::updateSize(GtkWidget* widget, const char* text, int* xSize, int* ySize)
{
    PangoRectangle rectangle;
    PangoContext* context = gtk_widget_create_pango_context(widget);
    PangoLayout* layout   = pango_layout_new(context);

    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    if (pango_parse_markup(text, -1, 0, nullptr, nullptr, nullptr, nullptr)) { pango_layout_set_markup(layout, text, -1); }
    else { pango_layout_set_text(layout, text, -1); }
    pango_layout_get_pixel_extents(layout, nullptr, &rectangle);
    *xSize = rectangle.width;
    *ySize = rectangle.height;
    g_object_unref(layout);
    g_object_unref(context);
}

}  // namespace Designer
}  // namespace OpenViBE
