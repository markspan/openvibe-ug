///-------------------------------------------------------------------------------------------------
/// 
/// \file TBoxAlgorithmStackedContinuousViz.hpp
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

#include "CBoxAlgorithmViz.hpp"
#include "VisualizationTools.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {
template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass>
class TBoxAlgorithmStackedContinuousViz final : public CBoxAlgorithmViz
{
public:
    TBoxAlgorithmStackedContinuousViz(const CIdentifier& classID, const std::vector<int>& parameters);
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(CBoxAlgorithmViz, m_ClassID)

    Toolkit::TStimulationDecoder<TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>> m_StimDecoder;
    Toolkit::TStreamedMatrixDecoder<TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>> m_MatrixDecoder;

    TRendererFactoryClass m_RendererFactory;
    std::vector<IRenderer*> m_Renderers;

protected:
    void Draw() override;
};

class CBoxAlgorithmStackedContinuousVizListener final : public CBoxAlgorithmVizListener
{
public:
    explicit CBoxAlgorithmStackedContinuousVizListener(const std::vector<int>& parameters) : CBoxAlgorithmVizListener(parameters) { }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        if (!this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_StreamedMatrix)) { box.setInputType(index, OV_TypeId_StreamedMatrix); }
        box.setInputType(1, OV_TypeId_Stimulations);
        return true;
    }
};

template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass = IRuler>
class TBoxAlgorithmStackedContinuousVizDesc final : public CBoxAlgorithmVizDesc
{
public:
    TBoxAlgorithmStackedContinuousVizDesc(const CString& name, const CIdentifier& descClassID, const CIdentifier& classID,
                                          const CParameterSet& parameterSet, const CString& shortDesc, const CString& detailedDesc)
        : CBoxAlgorithmVizDesc(name, descClassID, classID, parameterSet, shortDesc, detailedDesc) { }

    Plugins::IPluginObject* create() override
    {
        return new TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>(m_ClassID, m_Parameters);
    }

    Plugins::IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStackedContinuousVizListener(m_Parameters); }

    CString getCategory() const override { return CString("Advanced Visualization/") + m_CategoryName; }

    _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, m_DescClassID)
};


template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass>
TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>::TBoxAlgorithmStackedContinuousViz(
    const CIdentifier& classID, const std::vector<int>& parameters) : CBoxAlgorithmViz(classID, parameters) { }

template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>::initialize()

{
    const bool res = CBoxAlgorithmViz::initialize();

    m_MatrixDecoder.initialize(*this, 0);
    m_StimDecoder.initialize(*this, 1);

    m_RendererCtx->Clear();
    m_RendererCtx->SetTranslucency(float(m_Translucency));
    // m_rendererCtx->setTranslucency(m_nFlowerRing);
    m_RendererCtx->ScaleBy(float(m_DataScale));
    m_RendererCtx->SetPositiveOnly(m_IsPositive);
    m_RendererCtx->SetAxisDisplay(m_ShowAxis);
    m_RendererCtx->SetParentRendererContext(&getContext());

    m_SubRendererCtx->Clear();
    m_SubRendererCtx->SetParentRendererContext(m_RendererCtx);

    m_Ruler = new TRulerClass;
    m_Ruler->SetRendererContext(m_RendererCtx);

    return res;
}

template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>::uninitialize()

{
    for (size_t i = 0; i < m_Renderers.size(); ++i) { m_RendererFactory.Release(m_Renderers[i]); }
    m_Renderers.clear();

    delete m_SubRendererCtx;
    m_SubRendererCtx = nullptr;

    delete m_RendererCtx;
    m_RendererCtx = nullptr;

    delete m_Ruler;
    m_Ruler = nullptr;

    m_StimDecoder.uninitialize();
    m_MatrixDecoder.uninitialize();

    return CBoxAlgorithmViz::uninitialize();
}

template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>::process()
{
    const Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput              = this->getStaticBoxContext().getInputCount();
    size_t i, j;

    for (i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_MatrixDecoder.decode(size_t(i));

        CMatrix* matrix = m_MatrixDecoder.getOutputMatrix();
        size_t nChannel = matrix->getDimensionSize(0);
        size_t nSample  = matrix->getDimensionSize(1);

        if (nChannel == 0) {
            this->getLogManager() << Kernel::LogLevel_Error << "Input stream " << i << " has 0 channels\n";
            return false;
        }

        if (matrix->getDimensionCount() == 1) {
            nChannel = matrix->getDimensionSize(0);
            nSample  = 1;
        }

        if (m_MatrixDecoder.isHeaderReceived()) {
            GtkTreeIter gtkTreeIter;
            gtk_list_store_clear(m_ChannelListStore);

            m_Swaps.resize(nSample);

            for (j = 0; j < m_Renderers.size(); ++j) { m_RendererFactory.Release(m_Renderers[j]); }
            m_Renderers.clear();
            m_Renderers.resize(nChannel);

            m_SubRendererCtx->Clear();
            m_SubRendererCtx->SetParentRendererContext(m_RendererCtx);
            m_SubRendererCtx->SetTimeLocked(m_IsTimeLocked);
            m_SubRendererCtx->SetStackCount(nChannel);
            for (j = 0; j < nSample; ++j) {
                std::string name    = trim(matrix->getDimensionLabel(1, size_t(j)));
                std::string subname = name;
                std::transform(name.begin(), name.end(), subname.begin(), tolower);
                const CVertex v = m_ChannelPositions[subname];
                m_SubRendererCtx->AddChannel(name, v.x, v.y, v.z);
            }

            m_RendererCtx->Clear();
            m_RendererCtx->SetTranslucency(float(m_Translucency));
            m_RendererCtx->SetTimeScale(m_TimeScale);
            m_RendererCtx->SetElementCount(m_NElement);
            m_RendererCtx->ScaleBy(float(m_DataScale));
            m_RendererCtx->SetParentRendererContext(&getContext());
            m_RendererCtx->SetTimeLocked(m_IsTimeLocked);
            m_RendererCtx->SetXyzPlotDepth(m_XYZPlotHasDepth);

            gtk_tree_view_set_model(m_ChannelTreeView, nullptr);
            for (j = 0; j < nChannel; ++j) {
                std::string name    = trim(matrix->getDimensionLabel(0, size_t(j)));
                std::string subname = name;
                std::transform(name.begin(), name.end(), subname.begin(), tolower);
                const CVertex v = m_ChannelPositions[subname];

                if (name.empty()) { name = "Channel " + std::to_string(j + 1); }

                m_Renderers[j] = m_RendererFactory.Create();
                m_Renderers[j]->SetChannelCount(nSample);
                m_Renderers[j]->SetSampleCount(size_t(m_NElement)); // $$$
                // m_Renderers[j]->setSampleCount(size_t(m_TimeScaleW)); // $$$

                m_RendererCtx->AddChannel(name, v.x, v.y, v.z);
                // m_rendererCtx->addChannel(sanitize(matrix->getDimensionLabel(0, j)));
                gtk_list_store_append(m_ChannelListStore, &gtkTreeIter);
                gtk_list_store_set(m_ChannelListStore, &gtkTreeIter, 0, j + 1, 1, name.c_str(), -1);
            }
            gtk_tree_view_set_model(m_ChannelTreeView, GTK_TREE_MODEL(m_ChannelListStore));
            gtk_tree_selection_select_all(gtk_tree_view_get_selection(m_ChannelTreeView));

            if (m_TypeID == OV_TypeId_Signal) {
                m_RendererCtx->SetDataType(CRendererContext::EDataType::Signal);
                m_SubRendererCtx->SetDataType(CRendererContext::EDataType::Signal);
            }
            else if (m_TypeID == OV_TypeId_Spectrum) {
                m_RendererCtx->SetDataType(CRendererContext::EDataType::Spectrum);
                m_SubRendererCtx->SetDataType(CRendererContext::EDataType::Spectrum);
            }
            else {
                m_RendererCtx->SetDataType(CRendererContext::EDataType::Matrix);
                m_SubRendererCtx->SetDataType(CRendererContext::EDataType::Matrix);
            }

            m_Ruler->SetRenderer(nChannel ? m_Renderers[0] : nullptr);

            m_RebuildNeeded = true;
            m_RefreshNeeded = true;
            m_RedrawNeeded  = true;
        }
        if (m_MatrixDecoder.isBufferReceived()) {
            m_Time1                           = m_Time2;
            m_Time2                           = boxContext.getInputChunkEndTime(0, size_t(i));
            const uint64_t interChunkDuration = m_Time2 - m_Time1;
            const uint64_t chunkDuration      = (boxContext.getInputChunkEndTime(0, size_t(i)) - boxContext.getInputChunkStartTime(0, size_t(i)));
            const uint64_t sampleDuration     = chunkDuration / m_NElement;
            if (m_RendererCtx->IsTimeLocked()) {
                if ((interChunkDuration & ~0xf) != (m_RendererCtx->GetSampleDuration() & ~0xf) && interChunkDuration != 0) { // 0xf mask avoids rounding errors
                    m_SubRendererCtx->SetSampleDuration(interChunkDuration);
                    m_RendererCtx->SetSampleDuration(interChunkDuration);
                }
            }
            else {
                m_SubRendererCtx->SetSampleDuration(sampleDuration);
                m_RendererCtx->SetSampleDuration(sampleDuration);
            }

            m_RendererCtx->SetSpectrumFrequencyRange(size_t((uint64_t(nSample) << 32) / chunkDuration));
            m_RendererCtx->SetMinimumSpectrumFrequency(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_FrequencyBandMin))));
            m_RendererCtx->SetMaximumSpectrumFrequency(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_FrequencyBandMax))));

            for (j = 0; j < nChannel; ++j) {
                // Feed renderer with actual samples
                for (size_t k = 0; k < nSample; ++k) { m_Swaps[nSample - k - 1] = float(matrix->getBuffer()[j * nSample + k]); }
                m_Renderers[j]->Feed(&m_Swaps[0]);

                // Adjust feeding depending on theoretical dates
                if (m_RendererCtx->IsTimeLocked() && m_RendererCtx->GetSampleDuration()) {
                    const auto nTheoreticalSample = size_t(m_Time2 / m_RendererCtx->GetSampleDuration());
                    if (nTheoreticalSample > m_Renderers[j]->GetHistoryCount()) {
                        m_Renderers[j]->Prefeed(nTheoreticalSample - m_Renderers[j]->GetHistoryCount());
                    }
                }
            }

            m_RefreshNeeded = true;
            m_RedrawNeeded  = true;
        }
    }

    if (nInput > 1) {
        for (i = 0; i < boxContext.getInputChunkCount(1); ++i) {
            m_StimDecoder.decode(size_t(i));
            if (m_StimDecoder.isBufferReceived()) {
                const CStimulationSet* stimSet = m_StimDecoder.getOutputStimulationSet();
                for (j = 0; j < stimSet->size(); ++j) {
                    m_Renderers[0]->Feed(stimSet->getDate(j), stimSet->getId(j));
                    m_RedrawNeeded = true;
                }
            }
        }
    }

    size_t rendererSampleCount = 0;
    if (m_RendererCtx->IsTimeLocked()) {
        if (0 != m_RendererCtx->GetSampleDuration()) { rendererSampleCount = size_t(m_RendererCtx->GetTimeScale() / m_RendererCtx->GetSampleDuration()); }
    }
    else { rendererSampleCount = size_t(m_RendererCtx->GetElementCount()); }

    if (rendererSampleCount != 0) {
        for (j = 0; j < m_Renderers.size(); ++j) {
            if (rendererSampleCount != m_Renderers[j]->GetSampleCount()) {
                m_Renderers[j]->SetSampleCount(rendererSampleCount);
                m_RebuildNeeded = true;
                m_RefreshNeeded = true;
                m_RedrawNeeded  = true;
            }
        }
    }

    if (m_RebuildNeeded) { for (const auto& renderer : m_Renderers) { renderer->Rebuild(*m_SubRendererCtx); } }
    if (m_RefreshNeeded) { for (const auto& renderer : m_Renderers) { renderer->Refresh(*m_SubRendererCtx); } }
    if (m_RedrawNeeded) { this->Redraw(); }

    m_RebuildNeeded = false;
    m_RefreshNeeded = false;
    m_RedrawNeeded  = false;

    return true;
}

template <bool horizontalStack, bool drawBorders, class TRendererFactoryClass, class TRulerClass>
void TBoxAlgorithmStackedContinuousViz<horizontalStack, drawBorders, TRendererFactoryClass, TRulerClass>::Draw()

{
    CBoxAlgorithmViz::PreDraw();

    if (m_RendererCtx->GetSelectedCount() != 0) {
        glPushMatrix();
        glScalef(1, 1.0F / float(m_RendererCtx->GetSelectedCount()), 1);
        for (size_t i = 0; i < m_RendererCtx->GetSelectedCount(); ++i) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPushMatrix();
            glColor4f(m_Color.r, m_Color.g, m_Color.b, m_RendererCtx->GetTranslucency());
            glTranslatef(0, float(m_RendererCtx->GetSelectedCount() - i) - 1.0F, 0);
            if (!horizontalStack) {
                glScalef(1, -1, 1);
                glRotatef(-90, 0, 0, 1);
            }
            m_SubRendererCtx->SetAspect(m_RendererCtx->GetAspect());
            m_SubRendererCtx->SetStackCount(m_RendererCtx->GetSelectedCount());
            m_SubRendererCtx->SetStackIndex(i);
            m_Renderers[m_RendererCtx->GetSelected(i)]->Render(*m_SubRendererCtx);
            if (drawBorders) {
                glDisable(GL_TEXTURE_1D);
                glDisable(GL_BLEND);
                glColor3f(0, 0, 0);
                glBegin(GL_LINE_LOOP);
                glVertex2f(0, 0);
                glVertex2f(1, 0);
                glVertex2f(1, 1);
                glVertex2f(0, 1);
                glEnd();
            }
            glPopMatrix();
            glPopAttrib();
        }
        glPopMatrix();
    }

    CBoxAlgorithmViz::PostDraw();
}
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
