///-------------------------------------------------------------------------------------------------
/// 
/// \file utils.hpp
/// \brief Definition for some utils functions for plugin simple visualization.
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

#include <openvibe/ov_all.h>
#include <gtk/gtk.h>
#include <sstream>

//---------------------------------------------------------------------------------------------------
/// <summary> Initializes the color of the GDK. with old compiler as vs2013 we can't initialize structure easily.....</summary>
/// <param name="pixel"> For allocated colors, the pixel value used to draw this color on the screen.Not used anymore.</param>
/// <param name="r"> The red component of the color. This is a value between 0 and 65535, with 65535 indicating full intensity.</param>
/// <param name="g"> The green component of the color.</param>
/// <param name="b"> The blue component of the color.</param>
/// <returns> The initialized color (</returns>
inline GdkColor InitGDKColor(const guint32 pixel = 0, const guint16 r = 0, const guint16 g = 0, const guint16 b = 0)
{
    GdkColor c;
    c.pixel = pixel;
    c.red   = r;
    c.green = g;
    c.blue  = b;
    return c;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
class CGdkcolorAutoCast
{
public:
    CGdkcolorAutoCast(const OpenViBE::Kernel::IBox& box, OpenViBE::Kernel::IConfigurationManager& configManager, const size_t index)
        : m_configManager(configManager)
    {
        box.getSettingValue(index, m_settingValue);
        m_settingValue = m_configManager.expand(m_settingValue);
    }

    operator GdkColor() const
    {
        std::stringstream ss(m_settingValue.toASCIIString());
        int r = 0, g = 0, b = 0;
        char c;
        ss >> r >> c >> g >> c >> b;
        return InitGDKColor(0, guint16(r * 655.35), guint16(g * 655.35), guint16(b * 655.35));
    }

protected:
    OpenViBE::Kernel::IConfigurationManager& m_configManager;
    OpenViBE::CString m_settingValue;
};

//---------------------------------------------------------------------------------------------------
