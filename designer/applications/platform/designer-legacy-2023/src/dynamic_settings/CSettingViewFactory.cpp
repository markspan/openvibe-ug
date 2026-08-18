///-------------------------------------------------------------------------------------------------
/// 
/// \file CSettingViewFactory.cpp
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

#include "CSettingViewFactory.hpp"

#include "CBooleanSettingView.hpp"
#include "CIntegerSettingView.hpp"
#include "CFloatSettingView.hpp"
#include "CStringSettingView.hpp"
#include "CFilenameSettingView.hpp"
#include "CScriptSettingView.hpp"
#include "CColorSettingView.hpp"
#include "CColorGradientSettingView.hpp"
#include "CEnumerationSettingView.hpp"
#include "CBitMaskSettingView.hpp"

namespace OpenViBE {
namespace Designer {
namespace Setting {

CAbstractSettingView* CSettingViewFactory::GetSettingView(Kernel::IBox& box, const size_t index) const
{
    CIdentifier type;
    box.getSettingType(index, type);

    if (type == OV_TypeId_Boolean) { return new CBooleanSettingView(box, index, m_builderName); }
    if (type == OV_TypeId_Integer) { return new CIntegerSettingView(box, index, m_builderName, m_kernelCtx); }
    if (type == OV_TypeId_Float) { return new CFloatSettingView(box, index, m_builderName, m_kernelCtx); }
    if (type == OV_TypeId_String) { return new CStringSettingView(box, index, m_builderName); }
    if (type == OV_TypeId_Filename) { return new CFilenameSettingView(box, index, m_builderName, m_kernelCtx); }
    if (type == OV_TypeId_Script) { return new CScriptSettingView(box, index, m_builderName, m_kernelCtx); }
    if (type == OV_TypeId_Color) { return new CColorSettingView(box, index, m_builderName, m_kernelCtx); }
    if (type == OV_TypeId_ColorGradient) { return new CColorGradientSettingView(box, index, m_builderName, m_kernelCtx); }
    if (m_kernelCtx.getTypeManager().isEnumeration(type)) { return new CEnumerationSettingView(box, index, m_builderName, m_kernelCtx, type); }
    if (m_kernelCtx.getTypeManager().isBitMask(type)) { return new CBitMaskSettingView(box, index, m_builderName, m_kernelCtx, type); }

    //By default we consider every settings as a string
    return new CStringSettingView(box, index, m_builderName);
}

}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
