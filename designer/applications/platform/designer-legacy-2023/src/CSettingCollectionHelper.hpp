///-------------------------------------------------------------------------------------------------
/// 
/// \file CSettingCollectionHelper.hpp
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
class CSettingCollectionHelper final
{
public:
    CSettingCollectionHelper(const Kernel::IKernelContext& ctx, const char* guiFilename) : m_KernelCtx(ctx), m_GUIFilename(guiFilename) { }
    ~CSettingCollectionHelper() = default;

    CString GetSettingWidgetName(const CIdentifier& typeID) const;
    CString GetSettingEntryWidgetName(const CIdentifier& typeID) const;

    CString GetValue(const CIdentifier& typeID, GtkWidget* widget) const;
    static CString GetValueBoolean(GtkWidget* widget);
    static CString GetValueInteger(GtkWidget* widget);
    static CString GetValueFloat(GtkWidget* widget);
    static CString GetValueString(GtkWidget* widget);
    static CString GetValueFilename(GtkWidget* widget);
    static CString GetValueFoldername(GtkWidget* widget);
    static CString GetValueScript(GtkWidget* widget);
    static CString GetValueColor(GtkWidget* widget);
    static CString GetValueColorGradient(GtkWidget* widget);
    static CString GetValueEnumeration(const CIdentifier& typeID, GtkWidget* widget);
    static CString GetValueBitMask(const CIdentifier& typeID, GtkWidget* widget);

    void SetValue(const CIdentifier& typeID, GtkWidget* widget, const CString& value);
    void SetValueBoolean(GtkWidget* widget, const CString& value);
    void SetValueInteger(GtkWidget* widget, const CString& value);
    void SetValueFloat(GtkWidget* widget, const CString& value);
    static void SetValueString(GtkWidget* widget, const CString& value);
    void SetValueFilename(GtkWidget* widget, const CString& value);
    void SetValueFoldername(GtkWidget* widget, const CString& value);
    void SetValueScript(GtkWidget* widget, const CString& value);
    void SetValueColor(GtkWidget* widget, const CString& value);
    void SetValueColorGradient(GtkWidget* widget, const CString& value);
    void SetValueEnumeration(const CIdentifier& typeID, GtkWidget* widget, const CString& value) const;
    void SetValueBitMask(const CIdentifier& typeID, GtkWidget* widget, const CString& value) const;

    const Kernel::IKernelContext& m_KernelCtx;
    CString m_GUIFilename;

private:
    CSettingCollectionHelper() = delete;
};
}  // namespace Designer
}  // namespace OpenViBE
