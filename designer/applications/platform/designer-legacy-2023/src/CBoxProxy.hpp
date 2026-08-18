///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxProxy.hpp
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

#include <string>

namespace OpenViBE {
namespace Designer {
class CBoxProxy final
{
public:
    CBoxProxy(const Kernel::IKernelContext& ctx, Kernel::IScenario& scenario, const CIdentifier& boxID);
    ~CBoxProxy() { if (!m_applied) { this->Apply(); } }

    explicit operator Kernel::IBox*() const { return m_box; }
    explicit operator const Kernel::IBox*() const { return m_constBox; }

    int GetWidth(GtkWidget* widget) const;
    int GetHeight(GtkWidget* widget) const;

    int GetXCenter() const { return m_centerX; }
    int GetYCenter() const { return m_centerY; }
    void SetCenter(int x, int y);

    void SetBoxAlgorithmDescriptorOverride(const Plugins::IBoxAlgorithmDesc* pBoxAlgorithmDescriptor);

    void Apply();

    const char* GetLabel() const;
    const char* GetStatusLabel() const;

    bool IsBoxAlgorithmPluginPresent() const { return m_isBoxAlgorithmPresent; }
    bool IsUpToDate() const { return !m_box->hasAttribute(OV_AttributeId_Box_ToBeUpdated); }
    bool HasPendingDeprecatedInterfacors() const { return m_box->hasAttribute(OV_AttributeId_Box_PendingDeprecatedInterfacors); }
    bool IsDeprecated() const { return m_isDeprecated; }
    static bool IsUnstable() { return false; }
    bool IsDisabled() const;
    bool IsMetabox() const { return m_constBox->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox; }

protected:
    void updateSize(GtkWidget* widget, const char* label, const char* status, int* xSize, int* ySize) const;

    const Kernel::IKernelContext& m_kernelCtx;
    const Plugins::IBoxAlgorithmDesc* m_boxAlgorithmDescOverride = nullptr;
    const Kernel::IBox* m_constBox                               = nullptr;
    Kernel::IBox* m_box                                          = nullptr;
    bool m_applied                                               = false;
    bool m_showOriginalNameWhenModified                          = false;
    int m_centerX                                                = 0;
    int m_centerY                                                = 0;
    mutable std::string m_label;
    mutable std::string m_status;
    bool m_isBoxAlgorithmPresent = false;
    bool m_isDeprecated          = false;
};
}  // namespace Designer
}  // namespace OpenViBE
