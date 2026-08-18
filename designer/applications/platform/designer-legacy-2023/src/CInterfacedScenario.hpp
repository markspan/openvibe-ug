///-------------------------------------------------------------------------------------------------
/// 
/// \file CInterfacedScenario.hpp
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

#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <memory>

#include "base.hpp"

#include "CInterfacedObject.hpp"
#include "CScenarioStateStack.hpp"
#include "CSettingCollectionHelper.hpp"
#include "CBoxConfigurationDialog.hpp"
#include <visualization-toolkit/ovvizIVisualizationTree.h>

namespace OpenViBE {
namespace Designer {
class CApplication;
class CDesignerVisualization;
class CPlayerVisualization;

class CInterfacedScenario
{
public:
    CInterfacedScenario(const Kernel::IKernelContext& ctx, CApplication& application, Kernel::IScenario& scenario,
                        CIdentifier& scenarioID, GtkNotebook& notebook, const char* guiFilename, const char* guiSettingsFilename);
    ~CInterfacedScenario();

    bool IsLocked() const { return m_Player != nullptr; }
    void Redraw() const;
    void Redraw(Kernel::IBox& box);
    void Redraw(const Kernel::IComment& comment);
    void Redraw(const Kernel::ILink& link);
    void UpdateScenarioLabel();
    size_t PickInterfacedObject(int x, int y) const;
    bool PickInterfacedObject(int x, int y, int sizeX, int sizeY);

    void UndoCB(bool manageModifiedStatusFlag = true);
    void RedoCB(bool manageModifiedStatusFlag = true);
    void SnapshotCB(bool manageModifiedStatusFlag = true);
    void AddCommentCB(int x = -1, int y = -1);

    // Utility functions for scenario settings, inputs and outputs

    void AddScenarioSettingCB();
    //void editScenarioSettingCB(size_t index);
    void SwapScenarioSettings(size_t indexA, size_t indexB);

    void AddScenarioInputCB();
    void EditScenarioInputCB(size_t index);
    void SwapScenarioInputs(size_t indexA, size_t indexB);

    void AddScenarioOutputCB();
    void EditScenarioOutputCB(size_t index);
    void SwapScenarioOutputs(size_t indexA, size_t indexB);

    // Utility functions for scenario settings, inputs and outputs
    void ConfigureScenarioSettingsCB();

    // Drawing functions for scenario settings, inputs and outputs
    void RedrawConfigureScenarioSettingsDialog();
    void RedrawScenarioSettings();
    void RedrawScenarioInputSettings();
    void RedrawScenarioOutputSettings();

    void ScenarioDrawingAreaExposeCB(GdkEventExpose* event);
    void ScenarioDrawingAreaDragDataReceivedCB(GdkDragContext* dc, gint x, gint y, GtkSelectionData* selectionData, guint info, guint t);
    void ScenarioDrawingAreaMotionNotifyCB(GtkWidget* widget, GdkEventMotion* event);
    void ScenarioDrawingAreaButtonPressedCB(GtkWidget* widget, GdkEventButton* event);
    void ScenarioDrawingAreaButtonReleasedCB(GtkWidget* widget, GdkEventButton* event);
    void ScenarioDrawingAreaKeyPressEventCB(GtkWidget* widget, GdkEventKey* event);
    void ScenarioDrawingAreaKeyReleaseEventCB(GtkWidget* widget, GdkEventKey* event);

    void CopySelection() const;
    void CutSelection();
    void PasteSelection();
    void DeleteSelection();

    void DeleteBox(const CIdentifier& boxID) const; // Utility method to remove box from scenario and visualization
    void ContextMenuBoxUpdateCB(const Kernel::IBox& box);
    void ContextMenuBoxRemoveDeprecatedInterfacorsCB(const Kernel::IBox& box);
    void ContextMenuBoxRenameCB(Kernel::IBox& box);
    void ContextMenuBoxRenameAllCB();
    void ContextMenuBoxToggleEnableAllCB();
    void ContextMenuBoxAddInputCB(Kernel::IBox& box);
    void ContextMenuBoxEditInputCB(Kernel::IBox& box, const size_t index);
    void ContextMenuBoxRemoveInputCB(Kernel::IBox& box, const size_t index);
    void ContextMenuBoxAddOutputCB(Kernel::IBox& box);
    void ContextMenuBoxEditOutputCB(Kernel::IBox& box, const size_t index);
    void ContextMenuBoxRemoveOutputCB(Kernel::IBox& box, const size_t index);

    void ContextMenuBoxConnectScenarioInputCB(Kernel::IBox& box, size_t boxInputIdx, size_t scenarioInputIdx);
    void ContextMenuBoxConnectScenarioOutputCB(Kernel::IBox& box, size_t boxOutputIdx, size_t scenarioOutputIdx);
    void ContextMenuBoxDisconnectScenarioInputCB(Kernel::IBox& box, size_t boxInputIdx, size_t scenarioInputIdx);
    void ContextMenuBoxDisconnectScenarioOutputCB(Kernel::IBox& box, size_t boxOutputIdx, size_t scenarioOutputIdx);

    void ContextMenuBoxAddSettingCB(Kernel::IBox& box);
    void ContextMenuBoxEditSettingCB(Kernel::IBox& box, const size_t index);
    void ContextMenuBoxRemoveSettingCB(Kernel::IBox& box, const size_t index);
    void ContextMenuBoxConfigureCB(Kernel::IBox& box);
    void ContextMenuBoxAboutCB(Kernel::IBox& box) const;
    void ContextMenuBoxEditMetaboxCB(Kernel::IBox& box) const;

    void ContextMenuBoxEnableCB(Kernel::IBox& box);
    void ContextMenuBoxDisableCB(Kernel::IBox& box);
    void ContextMenuBoxEnableAllCB();
    void ContextMenuBoxDisableAllCB();

    void ContextMenuBoxDocumentationCB(Kernel::IBox& box) const;

    void ContextMenuScenarioAddCommentCB();
    void ContextMenuScenarioAboutCB();

    bool BrowseUrl(const CString& url, const CString& browserPrefix, const CString& browserPostfix) const;
    bool BrowseBoxDocumentation(const CIdentifier& boxID) const;

    void ToggleDesignerVisualization();
    bool IsDesignerVisualizationToggled() const { return m_designerVisualizationToggled; }

    void ShowCurrentVisualization() const;
    void HideCurrentVisualization() const;

    void CreatePlayerVisualization(VisualizationToolkit::IVisualizationTree* tree = nullptr);
    void ReleasePlayerVisualization();

    void StopAndReleasePlayer();
    bool SetModifiableSettingsWidgets() const;
    bool HasSelection() const { return !m_SelectedObjects.empty(); }
    bool CenterOnBox(const CIdentifier& identifier);
    void SetScale(double scale);
    double GetScale() const { return m_currentScale; }

    //-----------------------------
    //---------- typedef ----------
    //-----------------------------
    typedef struct SBoxContextMenuCB
    {
        EContextMenu command;
        size_t index;
        size_t secondaryIndex; // Used for connecting two streams
        Kernel::IBox* box;
        CInterfacedScenario* scenario;
    } box_ctx_menu_cb_t;

    // This struct is used for both settings inside the scenario and inside
    // the settings configurator
    typedef struct SSettingCallbackData
    {
        CInterfacedScenario* scenario;
        size_t index;
        GtkWidget* widgetValue;
        GtkWidget* widgetEntryValue;
        GtkWidget* container;
    } setting_cb_data_t;

    // This struct is used for both inputs and outputs of the scenario
    typedef struct SLinkCallbackData
    {
        CInterfacedScenario* scenario;
        size_t index;
        bool input;
    } link_cb_data_t;

    //-------------------------------
    //---------- variables ----------
    //-------------------------------
    Kernel::EPlayerStatus m_PlayerStatus;
    CIdentifier m_ScenarioID = CIdentifier::undefined();
    CIdentifier m_PlayerID   = CIdentifier::undefined();
    CIdentifier m_TreeID     = CIdentifier::undefined();

    CApplication& m_Application;
    Kernel::IScenario& m_Scenario;
    Kernel::IPlayer* m_Player                        = nullptr;
    VisualizationToolkit::IVisualizationTree* m_Tree = nullptr;
    CDesignerVisualization* m_DesignerVisualization  = nullptr;

    uint64_t m_LastLoopTime = 0;
    bool m_HasFileName      = false;
    bool m_HasBeenModified  = false;
    bool m_DebugCPUUsage    = false;

    std::string m_Filename;

    std::set<CIdentifier> m_SelectedObjects;

    std::unique_ptr<CScenarioStateStack> m_StateStack;

    // Objects necessary for holding settings GUI
    std::map<std::string, CIdentifier> m_SettingTypes;
    CSettingCollectionHelper* m_SettingHelper = nullptr;
    std::string m_SerializedSettingGUIXML;

private:
    const Kernel::IKernelContext& m_kernelCtx;
    GtkNotebook& m_notebook;
    bool m_designerVisualizationToggled         = false;
    CPlayerVisualization* m_playerVisualization = nullptr;
    GtkBuilder* m_guiBuilder                    = nullptr;
    GtkWidget* m_notebookPageTitle              = nullptr;
    GtkWidget* m_notebookPageContent            = nullptr;
    GtkViewport* m_scenarioViewport             = nullptr;
    GtkDrawingArea* m_scenarioDrawingArea       = nullptr;
    GdkPixmap* m_stencilBuffer                  = nullptr;
    GdkPixbuf* m_mensiaLogoPixbuf               = nullptr;

    bool m_buttonPressed  = false;
    bool m_shiftPressed   = false;
    bool m_controlPressed = false;
    bool m_altPressed     = false;
    bool m_aPressed       = false;
    bool m_wPressed       = false;
    std::string m_guiFilename;
    std::string m_guiSettingsFilename;

    double m_pressMouseX   = 0;
    double m_pressMouseY   = 0;
    double m_releaseMouseX = 0;
    double m_releaseMouseY = 0;
    double m_currentMouseX = 0;
    double m_currentMouseY = 0;
    int m_viewOffsetX      = 0;
    int m_viewOffsetY      = 0;
    size_t m_currentMode   = 0;

    size_t m_nBox     = 0;
    size_t m_nComment = 0;
    size_t m_nLink    = 0;

    size_t m_interfacedObjectId = 0;
    std::map<size_t, CInterfacedObject> m_interfacedObjects;
    CInterfacedObject m_currentObject;

    GtkWidget* m_configureSettingsDialog                 = nullptr;
    GtkWidget* m_settingsVBox                            = nullptr;
    GtkWidget* m_noHelpDialog                            = nullptr;
    GtkWidget* m_errorPendingDeprecatedInterfacorsDialog = nullptr;

    std::map<std::string, CIdentifier> m_streamTypes;

    std::map<size_t, box_ctx_menu_cb_t> m_boxCtxMenuCBs;
    std::vector<setting_cb_data_t> m_settingConfigCBDatas;
    std::vector<setting_cb_data_t> m_settingCBDatas;
    std::vector<link_cb_data_t> m_scenarioInputCBDatas;
    std::vector<link_cb_data_t> m_scenarioOutputCBDatas;

    double m_currentScale = 1;
    gint m_normalFontSize = 0;

    std::vector<CBoxConfigurationDialog*> m_boxConfigDialogs;

    typedef void (*menu_cb_function_t)(GtkMenuItem*, box_ctx_menu_cb_t*);
    GtkImageMenuItem* addNewImageMenuItemWithCBGeneric(GtkMenu* menu, const char* icon, const char* label, menu_cb_function_t cb,
                                                       Kernel::IBox* box, EContextMenu command, const size_t index, const size_t index2);

    GtkImageMenuItem* addNewImageMenuItemWithCB(GtkMenu* menu, const char* icon, const char* label, const menu_cb_function_t cb,
                                                Kernel::IBox* box, const EContextMenu command, const size_t index)
    {
        return addNewImageMenuItemWithCBGeneric(menu, icon, label, cb, box, command, index, 0);
    }

    void redrawScenarioLinkSettings(GtkWidget* links, bool isInput, std::vector<link_cb_data_t>& linkCBDatas,
                                    size_t (Kernel::IScenario::* getNLink)() const,
                                    bool (Kernel::IScenario::* getLinkName)(size_t, CString&) const,
                                    bool (Kernel::IScenario::* getLinkType)(size_t, CIdentifier&) const);
};
}  // namespace Designer
}  // namespace OpenViBE
