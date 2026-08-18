///-------------------------------------------------------------------------------------------------
/// 
/// \file TRulerPair.hpp
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

#include "../IRuler.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
template <class T1, class T2>
class TRulerPair final : public IRuler
{
public:
    void SetRendererContext(const CRendererContext* ctx) override
    {
        IRuler::SetRendererContext(ctx);
        m_First.SetRendererContext(ctx);
        m_Second.SetRendererContext(ctx);
    }

    void SetRenderer(const IRenderer* renderer) override
    {
        IRuler::SetRenderer(renderer);
        m_First.SetRenderer(renderer);
        m_Second.SetRenderer(renderer);
    }

    void render() override
    {
        m_First.DoRender();
        m_Second.DoRender();
    }

    void renderLeft(GtkWidget* widget) override
    {
        m_First.DoRenderLeft(widget);
        m_Second.DoRenderLeft(widget);
    }

    void renderRight(GtkWidget* widget) override
    {
        m_First.DoRenderRight(widget);
        m_Second.DoRenderRight(widget);
    }

    void renderBottom(GtkWidget* widget) override
    {
        m_First.DoRenderBottom(widget);
        m_Second.DoRenderBottom(widget);
    }

    T1 m_First;
    T2 m_Second;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
