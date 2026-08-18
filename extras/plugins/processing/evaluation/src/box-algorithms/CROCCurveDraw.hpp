///-------------------------------------------------------------------------------------------------
/// 
/// \file CROCCurveDraw.hpp
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

#if defined(TARGET_HAS_ThirdPartyGTK)

#include <openvibe/ov_all.h>

#include <gtk/gtk.h>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {
typedef std::pair<double, double> CCoordinate;

//The aim of the class is to handle the graphical part of a RocCurve
class CROCCurveDraw final
{
public:
    CROCCurveDraw(GtkNotebook* notebook, const std::string& className);
    ~CROCCurveDraw() { }
    std::vector<CCoordinate>& GetCoordinateVector() { return m_coordinateList; }

    void GenerateCurve();

    //Callbak functions, should not be called
    void ResizeEvent(GdkRectangle* rectangle);
    void ExposeEnvent();

    //This function is called when the cruve should be redraw for an external reason
    void ForceRedraw() const { redraw(); }

private:
    size_t m_margin = 50;
    std::vector<GdkPoint> m_pointList;
    std::vector<CCoordinate> m_coordinateList;
    size_t m_pixelsPerLeftRulerLabel = 0;

    GtkWidget* m_drawableArea = nullptr;
    bool m_hasBeenInit        = false;

    //For a mytical reason, gtk says that the DrawableArea is not a DrawableArea unless it's been exposed at least once...
    // So we need to if the DrawableArea as been exposed
    bool m_hasBeenExposed = false;

    void redraw() const;
    void drawLeftMark(size_t w, size_t h, const char* label) const;
    void drawBottomMark(size_t w, size_t h, const char* label) const;
};
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyGTK
