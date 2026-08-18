///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxConfigurationDialog.hpp
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

#include "dynamic_settings/CAbstractSettingView.hpp"
#include "dynamic_settings/CSettingViewFactory.hpp"

namespace OpenViBE {
namespace Designer {
class CBoxConfigurationDialog final : public IObserver
{
public:
    CBoxConfigurationDialog(const Kernel::IKernelContext& ctx, Kernel::IBox& box, const char* guiFilename,
                            const char* guiSettingsFilename, const bool isScenarioRunning = false);
    ~CBoxConfigurationDialog() override;

    bool Run();
    void update(CObservable& o, void* data) override;

    void SaveConfig() const;
    void LoadConfig() const;
    void OnOverrideBrowse() const;

    void StoreState();
    void RestoreState() const;

    CIdentifier GetBoxID() const { return m_box.getIdentifier(); }
    GtkWidget* GetWidget() const { return m_settingDialog; }
protected:
    void generateSettingsTable();
    bool addSettingsToView(const size_t settingIdx, const size_t tableIdx);
    void updateSize() const;
    void settingChange(const size_t index);
    void addSetting(const size_t index);
    void removeSetting(const size_t index, bool shift = true);
    int getTableIndex(const size_t index);

    const Kernel::IKernelContext& m_kernelCtx;
    Kernel::IBox& m_box;
    CString m_guiFilename;
    CString m_guiSettingsFilename;

    Setting::CSettingViewFactory m_settingFactory;
    std::vector<Setting::CAbstractSettingView*> m_settingViews;
    GtkTable* m_settingsTable           = nullptr;
    GtkViewport* m_viewPort             = nullptr;
    GtkScrolledWindow* m_scrolledWindow = nullptr;
    GtkEntry* m_overrideEntry           = nullptr;
    GtkWidget* m_overrideEntryContainer = nullptr;
    GtkWidget* m_settingDialog          = nullptr;
    GtkCheckButton* m_fileOverrideCheck = nullptr;
    bool m_isScenarioRunning            = false;

    std::vector<CString> m_settingsMemory;
};
}  // namespace Designer
}  // namespace OpenViBE
