///-------------------------------------------------------------------------------------------------
/// 
/// \file TRulerAutoType.hpp
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
template <class TRulerMatrix, class TRulerSignal, class TRulerSpectrum>
class TRulerAutoType final : public IRuler
{
public:
    void SetRendererContext(const CRendererContext* ctx) override
    {
        IRuler::SetRendererContext(ctx);
        m_rulerSignal.SetRendererContext(ctx);
        m_rulerSpectrum.SetRendererContext(ctx);
        m_rulerMatrix.SetRendererContext(ctx);
    }

    void SetRenderer(const IRenderer* renderer) override
    {
        IRuler::SetRenderer(renderer);
        m_rulerSignal.SetRenderer(renderer);
        m_rulerSpectrum.SetRenderer(renderer);
        m_rulerMatrix.SetRenderer(renderer);
    }

    void render() override
    {
        const CRendererContext::EDataType dataType = m_rendererCtx->GetDataType();
        if (dataType == CRendererContext::EDataType::Signal) { m_rulerSignal.DoRender(); }
        else if (dataType == CRendererContext::EDataType::Spectrum) { m_rulerSpectrum.DoRender(); }
        else { m_rulerMatrix.DoRender(); }
    }

    void renderLeft(GtkWidget* widget) override
    {
        const CRendererContext::EDataType dataType = m_rendererCtx->GetDataType();
        if (dataType == CRendererContext::EDataType::Signal) { m_rulerSignal.DoRenderLeft(widget); }
        else if (dataType == CRendererContext::EDataType::Spectrum) { m_rulerSpectrum.DoRenderLeft(widget); }
        else { m_rulerMatrix.DoRenderLeft(widget); }
    }

    void renderRight(GtkWidget* widget) override
    {
        const CRendererContext::EDataType dataType = m_rendererCtx->GetDataType();
        if (dataType == CRendererContext::EDataType::Signal) { m_rulerSignal.DoRenderRight(widget); }
        else if (dataType == CRendererContext::EDataType::Spectrum) { m_rulerSpectrum.DoRenderRight(widget); }
        else { m_rulerMatrix.DoRenderRight(widget); }
    }

    void renderBottom(GtkWidget* widget) override
    {
        const CRendererContext::EDataType dataType = m_rendererCtx->GetDataType();

        if (dataType == CRendererContext::EDataType::Signal) { m_rulerSignal.DoRenderBottom(widget); }
        else if (dataType == CRendererContext::EDataType::Spectrum) { m_rulerSpectrum.DoRenderBottom(widget); }
        else { m_rulerMatrix.DoRenderBottom(widget); }
    }

protected:
    TRulerMatrix m_rulerMatrix;
    TRulerSignal m_rulerSignal;
    TRulerSpectrum m_rulerSpectrum;
};
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
