///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
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

//Attributes of visualisation window :
#define AttributeId_VisualisationWindow_Width                   OpenViBE::CIdentifier(0x7B814CCA, 0x271DF6DD)
#define AttributeId_VisualisationWindow_Height                  OpenViBE::CIdentifier(0x4C90D4AD, 0x7A2554EC)

//Attributes of visualisation paned :
#define AttributeId_VisualisationWidget_DividerPosition         OpenViBE::CIdentifier(0x54E45F5B, 0x76C036E2)
#define AttributeId_VisualisationWidget_MaxDividerPosition      OpenViBE::CIdentifier(0x237E56D2, 0x10CD68AE)

#define OV_AttributeId_Box_Disabled                             OpenViBE::CIdentifier(0x341D3912, 0x1478DE86)

namespace OpenViBE {
namespace Designer {

/**
 * @brief Prefix of the drag payload used by metabox entries of the box palette.
 * Every metabox shares the same algorithm class identifier (OVP_ClassId_BoxAlgorithm_Metabox), so the palette
 * carries "metabox:<metabox identifier>" instead of the class identifier to tell which metabox to instantiate.
 */
constexpr const char* METABOX_DRAG_PREFIX = "metabox:";

enum class EContextMenu
{
    SelectionCopy, SelectionCut, SelectionPaste, SelectionDelete,

    BoxUpdate, BoxRemoveDeprecatedInterfacors, BoxRename, BoxDelete,
    BoxAddInput, BoxEditInput, BoxRemoveInput,
    BoxAddOutput, BoxEditOutput, BoxRemoveOutput,

    BoxConnectScenarioInput, BoxConnectScenarioOutput,
    BoxDisconnectScenarioInput, BoxDisconnectScenarioOutput,

    BoxAddSetting, BoxRemoveSetting, BoxEditSetting,
    BoxConfigure, BoxAbout, BoxEnable, BoxDisable,

    BoxAddMessageInput, BoxRemoveMessageInput,
    BoxAddMessageOutput, BoxRemoveMessageOutput,
    BoxEditMessageInput, BoxEditMessageOutput,

    BoxEditMetabox,
    BoxDocumentation,
    ScenarioAddComment,
    ScenarioAbout
};

enum
{
    Resource_StringName,
    Resource_StringShortDescription,
    Resource_StringIdentifier,
    Resource_StringStockIcon,
    Resource_StringColor,
    Resource_StringFont,
    Resource_BooleanIsPlugin,
    Resource_BooleanIsUnstable,
    Resource_BackGroundColor,
    Resource_BooleanIsGhost = 8001,
    Resource_BooleanIsMensia = 8002
};

enum
{
    Color_BackgroundPlayerStarted,
    Color_BoxBackground,
    Color_BoxBackgroundSelected,
    Color_BoxBackgroundMissing,
    Color_BoxBackgroundDisabled,
    Color_BoxBackgroundDeprecated,
    Color_BoxBackgroundUnstable,
    Color_BoxBackgroundOutdated,
    Color_BoxBackgroundMetabox,
    Color_BoxBorder,
    Color_BoxBorderSelected,
    Color_BoxInputBackground,
    Color_BoxInputBorder,
    Color_BoxOutputBackground,
    Color_BoxOutputBorder,
    Color_BoxSettingBackground,
    Color_BoxSettingBorder,
    Color_CommentBackground,
    Color_CommentBackgroundSelected,
    Color_CommentBorder,
    Color_CommentBorderSelected,
    Color_Link,
    Color_LinkSelected,
    Color_LinkDownCast,
    Color_LinkUpCast,
    Color_LinkInvalid,
    Color_SelectionArea,
    Color_SelectionAreaBorder,
};

enum { Box_None, Box_Input, Box_Output, Box_Setting, Box_Link, Box_ScenarioInput, Box_ScenarioOutput, Box_Update };

enum { Mode_None, Mode_Selection, Mode_SelectionAdd, Mode_MoveScenario, Mode_MoveSelection, Mode_Connect, Mode_EditSettings };
}  // namespace Designer
}  // namespace OpenViBE

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
