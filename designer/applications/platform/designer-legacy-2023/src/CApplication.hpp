///-------------------------------------------------------------------------------------------------
/// 
/// \file CApplication.hpp
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

#include <visualization-toolkit/ovvizIVisualizationManager.h>
#include "base.hpp"

#define ScenarioImportContext_OpenScenario  OpenViBE::CIdentifier(0xA180DB91, 0x19235AEE)
#define ScenarioExportContext_SaveScenario  OpenViBE::CIdentifier(0xC98C47AD, 0xCBD952B2)
#define ScenarioExportContext_SaveMetabox   OpenViBE::CIdentifier(0x529494F1, 0x6C2527D9)

#include <vector>

namespace OpenViBE {
namespace Designer {

class CInterfacedScenario;

class CLogListenerDesigner;

class CApplication
{
public:
    explicit CApplication(const Kernel::IKernelContext& ctx);
    ~CApplication();

    void Initialize(ECommandLineFlag cmdLineFlags);
    bool OpenScenario(const char* filename);

    /** \name Drag and drop management */
    //@{

    void DragDataGetCB(GtkWidget* widget, GdkDragContext* dragCtx, GtkSelectionData* selectionData, guint info, guint time) const;

    //@}

    /** \name Selection management */
    //@{

    void UndoCB() const;
    void RedoCB() const;

    void CopySelectionCB() const;
    void CutSelectionCB() const;
    void PasteSelectionCB() const;
    void DeleteSelectionCB() const;
    void PreferencesCB() const;

    //@}

    /** \name Scenario management */
    //@{

    CString GetWorkingDirectory() const;

    bool HasRunningScenario();
    bool HasUnsavedScenario();

    CInterfacedScenario* GetCurrentInterfacedScenario() const;
    void SaveOpenedScenarios();

    void TestCB() const;
    void NewScenarioCB();
    void OpenScenarioCB();
    void SaveScenarioCB(CInterfacedScenario* scenario = nullptr); // defaults to current scenario if nullptr
    void SaveScenarioAsCB(CInterfacedScenario* scenario = nullptr); // defaults to current scenario if nullptr
    void CloseScenarioCB(CInterfacedScenario* scenario);
    void RestoreDefaultScenariosCB() const;

    void StopScenarioCB();
    void PauseScenarioCB() const;
    void NextScenarioCB() const;
    void PlayScenarioCB();
    void ForwardScenarioCB();

    void ConfigureScenarioSettingsCB(CInterfacedScenario* scenario) const;

    void AddCommentCB(CInterfacedScenario* scenario) const;

    void ChangeCurrentScenario(int pageIdx);
    void ReorderCurrentScenario(size_t newPageIdx);
    void AddRecentScenario(const std::string& scenarioPath);

    static void CannotSaveScenarioBeforeUpdate();

    //@}

    /** \name Designer visualization management */
    //@{

    void DeleteDesignerVisualizationCB() const;
    void ToggleDesignerVisualizationCB() const;

    //@}

    /** \name Player management */
    //@{

    Kernel::IPlayer* GetPlayer() const;
    bool CreatePlayer() const;
    void StopInterfacedScenarioAndReleasePlayer(CInterfacedScenario* scenario) const;

    //@}

    /** \name Application management */
    //@{

    bool QuitApplicationCB();
    void AboutOpenViBECB() const;
    void AboutScenarioCB(CInterfacedScenario* scenario) const;
    void AboutLinkClickedCB(const gchar* url) const;

    void BrowseDocumentationCB() const;
    void ReportIssueCB() const;
    void WindowStateChangedCB(bool isMaximized);
    bool DisplayChangelogWhenAvailable();

    //@}

    void LogLevelCB() const;
    //void logLevelMessagesCB();
    void CpuUsageCB() const;
    void ZoomInCB() const;  //Call when a zoom in is required
    void ZoomOutCB() const; //Call when a zoom out is required
    void SpinnerZoomChangedCB(const size_t scaleDelta) const;

    const Kernel::IKernelContext& m_kernelCtx;
    Kernel::IPluginManager* m_PluginMgr                             = nullptr;
    Kernel::IScenarioManager* m_ScenarioMgr                         = nullptr;
    VisualizationToolkit::IVisualizationManager* m_VisualizationMgr = nullptr;
    Kernel::IScenario* m_ClipboardScenario                          = nullptr;

    ECommandLineFlag m_CmdLineFlags = CommandLineFlag_None;

    GtkBuilder* m_Builder   = nullptr;
    GtkWidget* m_MainWindow = nullptr;

    GtkTreeStore* m_BoxAlgorithmTreeModel        = nullptr;
    GtkTreeModel* m_BoxAlgorithmTreeModelFilter  = nullptr;
    GtkTreeModel* m_BoxAlgorithmTreeModelFilter2 = nullptr;
    GtkTreeModel* m_BoxAlgorithmTreeModelFilter3 = nullptr;
    GtkTreeModel* m_BoxAlgorithmTreeModelFilter4 = nullptr;
    GtkTreeView* m_BoxAlgorithmTreeView          = nullptr;
    GtkTreeStore* m_AlgorithmTreeModel           = nullptr;

    GtkSpinButton* m_FastForwardFactor = nullptr;

    // UI for adding inputs and outputs to a scenario
    GtkWidget* m_Inputs  = nullptr;
    GtkWidget* m_Outputs = nullptr;

    gint m_FilterTimeout      = 0;
    const gchar* m_SearchTerm = nullptr;

    uint64_t m_LastTimeRefresh = 0;
    bool m_IsQuitting          = false;
    bool m_IsNewVersion        = false;

    std::vector<CInterfacedScenario*> m_Scenarios;
    std::vector<const Plugins::IPluginObjectDesc*> m_NewBoxes;
    std::vector<const Plugins::IPluginObjectDesc*> m_UpdatedBoxes;

protected:
    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;

    CLogListenerDesigner* m_logListener = nullptr;

    GtkWidget* m_splashScreen                      = nullptr;
    GtkNotebook* m_scenarioNotebook                = nullptr;
    GtkNotebook* m_resourceNotebook                = nullptr;
    GtkTreeView* m_algorithmTreeView               = nullptr;
    GtkWidget* m_configureSettingsAddSettingButton = nullptr;
    GtkContainer* m_menuOpenRecent                 = nullptr;
    std::vector<const GtkWidget*> m_recentScenarios;

    bool m_isMaximized = false;

    size_t m_currentScenarioIdx = 0;
    std::vector<std::string> m_documentedBoxes;
};

}  //namespace Designer
}  //namespace OpenViBE
