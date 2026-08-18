///-------------------------------------------------------------------------------------------------
/// 
/// \file TBoxAlgorithmInstantViz.hpp
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
template <class TRendererFactoryClass, class TRulerClass>
class TBoxAlgorithmInstantViz : public CBoxAlgorithmViz
{
public:
    TBoxAlgorithmInstantViz(const CIdentifier& classID, const std::vector<int>& params);
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(CBoxAlgorithmViz, m_ClassID)

    TRendererFactoryClass m_RendererFactory;

    size_t m_NInput = 0;
    std::vector<IRenderer*> m_Renderers;
    std::vector<Toolkit::TStreamedMatrixDecoder<TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>>> m_Decoder;

    double m_LastERPFraction = 0;

protected:
    void Draw() override;
};

class CBoxAlgorithmInstantVizListener final : public CBoxAlgorithmVizListener
{
public:
    explicit CBoxAlgorithmInstantVizListener(const std::vector<int>& parameters) : CBoxAlgorithmVizListener(parameters) { }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        if (!this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_StreamedMatrix)) { box.setInputType(index, OV_TypeId_StreamedMatrix); }
        else { for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputType(i, typeID); } }
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(0, typeID);
        box.setInputType(index, typeID);
        box.setInputName(index, "Matrix");
        box.addSetting("Color", OV_TypeId_Color, "${AdvancedViz_DefaultColor}");
        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeSetting(this->GetBaseSettingCount() + index - 1);
        return true;
    }
};

template <class TRendererFactoryClass, class TRulerClass = IRuler, template < typename, typename > class TBoxAlgorithm = TBoxAlgorithmInstantViz>
class TBoxAlgorithmInstantVizDesc final : public CBoxAlgorithmVizDesc
{
public:
    TBoxAlgorithmInstantVizDesc(const CString& name, const CIdentifier& descClassID, const CIdentifier& classID,
                                const CParameterSet& parameterSet, const CString& shortDesc, const CString& detailedDesc)
        : CBoxAlgorithmVizDesc(name, descClassID, classID, parameterSet, shortDesc, detailedDesc) { }

    Plugins::IPluginObject* create() override { return new TBoxAlgorithm<TRendererFactoryClass, TRulerClass>(m_ClassID, m_Parameters); }
    Plugins::IBoxListener* createBoxListener() const override { return new CBoxAlgorithmInstantVizListener(m_Parameters); }
    CString getCategory() const override { return CString("Advanced Visualization/") + m_CategoryName; }

    _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, m_DescClassID)
};


template <class TRendererFactoryClass, class TRulerClass>
TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::TBoxAlgorithmInstantViz(const CIdentifier& classID, const std::vector<int>& params)
    : CBoxAlgorithmViz(classID, params) { }

template <class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::initialize()
{
    bool res = CBoxAlgorithmViz::initialize();

    m_LastERPFraction = 0;

    m_NInput = this->getStaticBoxContext().getInputCount();
    m_Renderers.resize(m_NInput);
    m_Decoder.resize(m_NInput);
    for (size_t i = 0; i < m_NInput; ++i) {
        m_Renderers[i] = m_RendererFactory.Create();
        m_Decoder[i].initialize(*this, i);
        if (!m_Renderers[i]) {
            this->getLogManager() << Kernel::LogLevel_Error << "Could not create renderer, it might have been disabled at compile time\n";
            res = false;
        }
    }

    m_Ruler = new TRulerClass;
    m_Ruler->SetRendererContext(m_RendererCtx);
    m_Ruler->SetRenderer(m_Renderers[0]);

    gtk_widget_set_sensitive(m_TimeScaleW, false);

    return res;
}

template <class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::uninitialize()
{
    for (size_t i = 0; i < m_NInput; ++i) {
        m_RendererFactory.Release(m_Renderers[i]);
        m_Decoder[i].uninitialize();
    }

    m_Decoder.clear();
    m_Renderers.clear();

    return CBoxAlgorithmViz::uninitialize();
}

template <class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::process()

{
    const Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput              = this->getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            m_Decoder[i].decode(j);

            CMatrix* matrix = m_Decoder[i].getOutputMatrix();
            size_t nChannel = matrix->getDimensionSize(0);
            size_t nSample  = matrix->getDimensionSize(1);

            if (nChannel == 0) {
                this->getLogManager() << Kernel::LogLevel_Error << "Input stream " << i << " has 0 channels\n";
                return false;
            }

            if (matrix->getDimensionCount() == 1) {
                nChannel = 1;
                nSample  = matrix->getDimensionSize(0);
            }

            if (m_Decoder[i].isHeaderReceived()) {
                // TODO
                // Check dimension coherence
                // Only apply renderer context when first header is received

                GtkTreeIter gtkTreeIter;
                gtk_list_store_clear(m_ChannelListStore);

                m_Swaps.resize(nChannel);

                m_RendererCtx->Clear();
                m_RendererCtx->SetTranslucency(float(m_Translucency));
                m_RendererCtx->SetFlowerRingCount(m_NFlowerRing);
                // m_rendererCtx->setTimeScale(1); // Won't be used
                m_RendererCtx->ScaleBy(float(m_DataScale));
                m_RendererCtx->SetPositiveOnly(m_IsPositive);
                m_RendererCtx->SetAxisDisplay(m_ShowAxis);
                m_RendererCtx->SetParentRendererContext(&getContext());
                m_RendererCtx->SetXyzPlotDepth(m_XYZPlotHasDepth);

                gtk_tree_view_set_model(m_ChannelTreeView, nullptr);
                for (j = 0; j < nChannel; ++j) {
                    std::string name    = trim(matrix->getDimensionLabel(0, j));
                    std::string subname = name;
                    std::transform(name.begin(), name.end(), subname.begin(), tolower);
                    const CVertex v = m_ChannelPositions[subname];

                    if (name.empty()) { name = "Channel " + std::to_string(j + 1); }

                    m_RendererCtx->AddChannel(name, v.x, v.y, v.z);
                    gtk_list_store_append(m_ChannelListStore, &gtkTreeIter);
                    gtk_list_store_set(m_ChannelListStore, &gtkTreeIter, 0, j + 1, 1, name.c_str(), -1);
                }
                gtk_tree_view_set_model(m_ChannelTreeView, GTK_TREE_MODEL(m_ChannelListStore));
                gtk_tree_selection_select_all(gtk_tree_view_get_selection(m_ChannelTreeView));

                m_Renderers[i]->SetChannelCount(nChannel);
                m_Renderers[i]->SetSampleCount(nSample);

                if (nSample > 1 && m_TypeID != OV_TypeId_Spectrum) { gtk_widget_show(m_ERPPlayer); }

                if (m_TypeID == OV_TypeId_Signal) { m_RendererCtx->SetDataType(CRendererContext::EDataType::Signal); }
                else if (m_TypeID == OV_TypeId_Spectrum) { m_RendererCtx->SetDataType(CRendererContext::EDataType::Spectrum); }
                else { m_RendererCtx->SetDataType(CRendererContext::EDataType::Matrix); }

                m_RebuildNeeded = true;
                m_RefreshNeeded = true;
                m_RedrawNeeded  = true;
            }
            if (m_Decoder[i].isBufferReceived()) {
                const uint64_t chunkDuration = (boxContext.getInputChunkEndTime(i, j) - boxContext.getInputChunkStartTime(i, j));

                m_RendererCtx->SetSampleDuration(chunkDuration / nSample);
                m_RendererCtx->SetSpectrumFrequencyRange(size_t((uint64_t(nSample) << 32) / chunkDuration));
                m_RendererCtx->SetMinimumSpectrumFrequency(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_FrequencyBandMin))));
                m_RendererCtx->SetMaximumSpectrumFrequency(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_FrequencyBandMax))));

                // Sets time scale
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(m_Builder, "spinbutton_time_scale")), (chunkDuration >> 22) / 1024.);

                m_Renderers[i]->Clear(0); // Drop last samples as they will be fed again
                for (size_t k = 0; k < nSample; ++k) {
                    for (size_t l = 0; l < nChannel; ++l) { m_Swaps[l] = float(matrix->getBuffer()[l * nSample + k]); }
                    m_Renderers[i]->Feed(&m_Swaps[0]);
                }

                m_RefreshNeeded = true;
                m_RedrawNeeded  = true;
            }
        }
    }

    double erpFraction = double(getContext().GetERPFraction());
    if (m_RendererCtx->IsERPPlayerActive()) {
        erpFraction += .0025;
        if (erpFraction > 1) { erpFraction = 0; }
    }
    if (std::fabs(m_LastERPFraction - erpFraction) > DBL_EPSILON) {
        gtk_range_set_value(GTK_RANGE(m_ERPRange), erpFraction);
        m_LastERPFraction = erpFraction;
        m_RefreshNeeded   = true;
        m_RedrawNeeded    = true;
    }

    if (m_RebuildNeeded) { for (const auto& renderer : m_Renderers) { renderer->Rebuild(*m_RendererCtx); } }
    if (m_RefreshNeeded) { for (const auto& renderer : m_Renderers) { renderer->Refresh(*m_RendererCtx); } }
    if (m_RedrawNeeded) { this->Redraw(); }

    m_RebuildNeeded = false;
    m_RefreshNeeded = false;
    m_RedrawNeeded  = false;

    if (m_RendererCtx->IsERPPlayerActive()) { gtk_button_set_label(GTK_BUTTON(m_ERPPlayerButton), GTK_STOCK_MEDIA_PAUSE); }
    else { gtk_button_set_label(GTK_BUTTON(m_ERPPlayerButton), GTK_STOCK_MEDIA_PLAY); }

    return true;
}

template <class TRendererFactoryClass, class TRulerClass>
void TBoxAlgorithmInstantViz<TRendererFactoryClass, TRulerClass>::Draw()

{
    CBoxAlgorithmViz::PreDraw();

    for (size_t i = 0; i < m_NInput; ++i) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        if (i < m_Colors.size()) { glColor4f(m_Colors[i].r, m_Colors[i].g, m_Colors[i].b, m_RendererCtx->GetTranslucency()); }
        else { glColor4f(m_Color.r, m_Color.g, m_Color.b, m_RendererCtx->GetTranslucency()); }
        if (m_Renderers[i]) { m_Renderers[i]->Render(*m_RendererCtx); }
        glPopAttrib();
    }

    CBoxAlgorithmViz::PostDraw();
}
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
