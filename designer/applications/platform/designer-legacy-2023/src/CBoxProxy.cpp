///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxProxy.cpp
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

#include "CBoxProxy.hpp"
#include "TAttributeHandler.hpp"
#include <fs/Files.h>

namespace OpenViBE {
namespace Designer {

CBoxProxy::CBoxProxy(const Kernel::IKernelContext& ctx, Kernel::IScenario& scenario, const CIdentifier& boxID)
    : m_kernelCtx(ctx), m_constBox(scenario.getBoxDetails(boxID)), m_box(scenario.getBoxDetails(boxID)),
      m_isDeprecated(m_kernelCtx.getPluginManager().isPluginObjectFlaggedAsDeprecated(m_constBox->getAlgorithmClassIdentifier()))
{
    if (m_constBox) {
        if (m_constBox->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) {
            CIdentifier metaboxId;
            metaboxId.fromString(m_constBox->getAttributeValue(OVP_AttributeId_Metabox_ID));
            const CString path(m_kernelCtx.getMetaboxManager().getMetaboxFilePath(metaboxId));

            m_isBoxAlgorithmPresent = FS::Files::fileExists(path.toASCIIString());
        }
        else { m_isBoxAlgorithmPresent = m_kernelCtx.getPluginManager().canCreatePluginObject(m_constBox->getAlgorithmClassIdentifier()); }

        const TAttributeHandler handler(*m_constBox);
        m_centerX = handler.getAttributeValue<int>(OV_AttributeId_Box_XCenterPosition);
        m_centerY = handler.getAttributeValue<int>(OV_AttributeId_Box_YCenterPosition);
    }
    m_showOriginalNameWhenModified = m_kernelCtx.getConfigurationManager().expandAsBoolean("${Designer_ShowOriginalBoxName}", true);
}

int CBoxProxy::GetWidth(GtkWidget* widget) const
{
    int x, y;
    updateSize(widget, GetLabel(), GetStatusLabel(), &x, &y);
    return x;
}

int CBoxProxy::GetHeight(GtkWidget* widget) const
{
    int x, y;
    updateSize(widget, GetLabel(), GetStatusLabel(), &x, &y);
    return y;
}

void CBoxProxy::SetCenter(const int x, const int y)
{
    m_centerX = x;
    m_centerY = y;
    m_applied = false;
}

void CBoxProxy::SetBoxAlgorithmDescriptorOverride(const Plugins::IBoxAlgorithmDesc* pBoxAlgorithmDescriptor)
{
    m_boxAlgorithmDescOverride = pBoxAlgorithmDescriptor;
}

void CBoxProxy::Apply()
{
    if (m_box) {
        TAttributeHandler handler(*m_box);

        if (handler.HasAttribute(OV_AttributeId_Box_XCenterPosition)) { handler.setAttributeValue<int>(OV_AttributeId_Box_XCenterPosition, m_centerX); }
        else { handler.addAttribute<int>(OV_AttributeId_Box_XCenterPosition, m_centerX); }

        if (handler.HasAttribute(OV_AttributeId_Box_YCenterPosition)) { handler.setAttributeValue<int>(OV_AttributeId_Box_YCenterPosition, m_centerY); }
        else { handler.addAttribute<int>(OV_AttributeId_Box_YCenterPosition, m_centerY); }

        m_applied = true;
    }
}

const char* CBoxProxy::GetLabel() const
{
    const bool canChangeInput(m_constBox->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput) || m_constBox->hasAttribute(OV_AttributeId_Box_FlagCanAddInput));
    const bool canChangeOutput(
        m_constBox->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput) || m_constBox->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput));
    const bool canChangeSetting(
        m_constBox->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting) || m_constBox->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting));

    const Plugins::IPluginObjectDesc* desc = (m_boxAlgorithmDescOverride == nullptr
                                                  ? m_kernelCtx.getPluginManager().getPluginObjectDescCreating(m_constBox->getAlgorithmClassIdentifier())
                                                  : m_boxAlgorithmDescOverride);

    std::string name(m_constBox->getName());

    const std::string red("#602020"), green("#206020"), grey("#404040");

    // pango is used in the box diplay to format the box name (e.g. bold to tell it's a configurable box)
    // incidently, this makes the box name pango-enabled. If an error appears in the markup, the box display would be wrong.
    if (!pango_parse_markup(name.c_str(), -1, 0, nullptr, nullptr, nullptr, nullptr)) {
        // the name uses invalid markup characters
        // we sanitize the markup tag overture '<'
        // markup should not be used in the box name anyway (hidden feature),
        // but the character '<' may actually be useful in a valid name
        for (size_t c = 0; c < name.size(); ++c) {
            if (name[c] == '<') {
                name[c] = ';';
                name.insert(c, "&lt");
            }
            else if (name[c] == '&') {
                name[c] = ';';
                name.insert(c, "&amp");
            }
        }
    }

    m_label = name;

    if (name.empty()) { m_label = "Unnamed Box"; }

    const std::string boxNameColor = "#000000";

    if (m_constBox->getSettingCount() != 0) { m_label = "<span color=\"" + boxNameColor + R"(" weight="bold">)" + m_label + "</span>"; }

    if (m_showOriginalNameWhenModified) {
        const std::string boxOriginalName(desc ? std::string(desc->getName()) : name);
        if (boxOriginalName != name) { m_label = "<small><i><span foreground=\"" + grey + "\">" + boxOriginalName + "</span></i></small>\n" + m_label; }
    }

    if (canChangeInput || canChangeOutput || canChangeSetting) {
        m_label += "\n<span size=\"smaller\">";
        m_label += "<span foreground=\"" + (canChangeInput ? green : red) + "\">In</span>";
        m_label += "|<span foreground=\"" + (canChangeOutput ? green : red) + "\">Out</span>";
        m_label += "|<span foreground=\"" + (canChangeSetting ? green : red) + "\">Set</span>";
        m_label += "</span>";
    }

    return m_label.c_str();
}

const char* CBoxProxy::GetStatusLabel() const
{
    const bool toBeUpdated(m_box->hasAttribute(OV_AttributeId_Box_ToBeUpdated));
    const bool pendingDeprecatedInterfacors(m_box->hasAttribute(OV_AttributeId_Box_PendingDeprecatedInterfacors));
    const bool isDeprecated(this->IsBoxAlgorithmPluginPresent() && this->IsDeprecated());
    const bool isDisabled(this->IsDisabled());

    const std::string blue("#202060");

    m_status = "";
    if (isDeprecated || toBeUpdated || isDisabled || pendingDeprecatedInterfacors) {
        m_status += R"(<span size="smaller" foreground=")" + blue + "\">";
        if (isDeprecated) { m_status += " <span style=\"italic\">deprecated</span>"; }
        if (toBeUpdated) { m_status += " <span style=\"italic\">update</span>"; }
        if (isDisabled) { m_status += " <span style=\"italic\">disabled</span>"; }
        if (pendingDeprecatedInterfacors) { m_status += " <span style=\"italic\">deprecated I/O/S</span>"; }
        m_status += " </span>";
    }
    return m_status.c_str();
}

bool CBoxProxy::IsDisabled() const
{
    const TAttributeHandler handler(*m_constBox);
    return handler.HasAttribute(OV_AttributeId_Box_Disabled);
}

void CBoxProxy::updateSize(GtkWidget* widget, const char* label, const char* status, int* xSize, int* ySize) const
{
    PangoRectangle labelRect;
    PangoRectangle statusRect;
    PangoContext* context = gtk_widget_create_pango_context(widget);
    PangoLayout* layout   = pango_layout_new(context);
    pango_layout_set_markup(layout, label, -1);
    pango_layout_get_pixel_extents(layout, nullptr, &labelRect);
    pango_layout_set_markup(layout, status, -1);
    pango_layout_get_pixel_extents(layout, nullptr, &statusRect);

    if (!strlen(status)) {
        statusRect.width  = 0;
        statusRect.height = 0;
    }

    *xSize = std::max(labelRect.width, statusRect.width);
    *ySize = labelRect.height + statusRect.height;

    g_object_unref(layout);
    g_object_unref(context);
}

}  // namespace Designer
}  // namespace OpenViBE
