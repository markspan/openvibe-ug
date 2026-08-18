///-------------------------------------------------------------------------------------------------
/// 
/// \file TBoxAlgorithmContinuousViz.hpp
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
class TBoxAlgorithmContinuousViz final : public CBoxAlgorithmViz
{
public:
    TBoxAlgorithmContinuousViz(const CIdentifier& classID, const std::vector<int>& parameters);
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(CBoxAlgorithmViz, m_ClassID)

    Toolkit::TStreamedMatrixDecoder<TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>> m_MatrixDecoder;
    Toolkit::TStimulationDecoder<TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>> m_StimulationDecoder;
    TRendererFactoryClass m_RendererFactory;
    IRenderer* m_Renderer = nullptr;

protected:
    void Draw() override;
};

class CBoxAlgorithmContinuousVizListener final : public CBoxAlgorithmVizListener
{
public:
    explicit CBoxAlgorithmContinuousVizListener(const std::vector<int>& parameters) : CBoxAlgorithmVizListener(parameters) { }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        if (!this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_StreamedMatrix)) { box.setInputType(index, OV_TypeId_StreamedMatrix); }
        box.setInputType(1, OV_TypeId_Stimulations);
        return true;
    }
};

template <class TRendererFactoryClass, class TRulerClass = IRuler>
class TBoxAlgorithmContinuousVizDesc final : public CBoxAlgorithmVizDesc
{
public:
    TBoxAlgorithmContinuousVizDesc(const CString& name, const CIdentifier& descClassID, const CIdentifier& classID,
                                   const CParameterSet& parameterSet, const CString& shortDesc, const CString& detailedDesc)
        : CBoxAlgorithmVizDesc(name, descClassID, classID, parameterSet, shortDesc, detailedDesc) { }

    Plugins::IPluginObject* create() override { return new TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>(m_ClassID, m_Parameters); }
    Plugins::IBoxListener* createBoxListener() const override { return new CBoxAlgorithmContinuousVizListener(m_Parameters); }
    CString getCategory() const override { return CString("Advanced Visualization/") + m_CategoryName; }

    _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, m_DescClassID)
};

template <class TRendererFactoryClass, class TRulerClass>
TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>::TBoxAlgorithmContinuousViz(
    const CIdentifier& classID, const std::vector<int>& parameters) : CBoxAlgorithmViz(classID, parameters) { }

template <class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>::initialize()
{
    const bool res = CBoxAlgorithmViz::initialize();

    m_MatrixDecoder.initialize(*this, 0);
    m_StimulationDecoder.initialize(*this, 1);

    m_Renderer = m_RendererFactory.Create();

    m_Ruler = new TRulerClass;
    m_Ruler->SetRendererContext(m_RendererCtx);
    m_Ruler->SetRenderer(m_Renderer);

    return res;
}

template <class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>::uninitialize()
{
    m_RendererFactory.Release(m_Renderer);
    m_Renderer = nullptr;

    delete m_Ruler;
    m_Ruler = nullptr;

    m_StimulationDecoder.uninitialize();
    m_MatrixDecoder.uninitialize();

    return CBoxAlgorithmViz::uninitialize();
}

template <class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>::process()
{
    const Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_MatrixDecoder.decode(i);

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
            GtkTreeIter gtkTreeIt;
            gtk_list_store_clear(m_ChannelListStore);

            m_Swaps.resize(size_t(nChannel));

            m_RendererCtx->Clear();
            m_RendererCtx->SetTranslucency(float(m_Translucency));
            m_RendererCtx->SetFlowerRingCount(m_NFlowerRing);
            m_RendererCtx->SetTimeScale(m_TimeScale);
            m_RendererCtx->SetElementCount(m_NElement);
            m_RendererCtx->ScaleBy(float(m_DataScale));
            m_RendererCtx->SetAxisDisplay(m_ShowAxis);
            m_RendererCtx->SetPositiveOnly(m_IsPositive);
            m_RendererCtx->SetParentRendererContext(&getContext());
            m_RendererCtx->SetTimeLocked(m_IsTimeLocked);
            m_RendererCtx->SetXyzPlotDepth(m_XYZPlotHasDepth);

            gtk_tree_view_set_model(m_ChannelTreeView, nullptr);
            for (size_t j = 0; j < nChannel; ++j) {
                std::string name    = trim(matrix->getDimensionLabel(0, j));
                std::string subname = name;
                std::transform(name.begin(), name.end(), subname.begin(), tolower);
                const CVertex v = m_ChannelPositions[subname];

                if (name.empty()) { name = "Channel " + std::to_string(j + 1); }

                m_RendererCtx->AddChannel(name, v.x, v.y, v.z);
                gtk_list_store_append(m_ChannelListStore, &gtkTreeIt);
                gtk_list_store_set(m_ChannelListStore, &gtkTreeIt, 0, j + 1, 1, name.c_str(), -1);
            }
            gtk_tree_view_set_model(m_ChannelTreeView, GTK_TREE_MODEL(m_ChannelListStore));
            gtk_tree_selection_select_all(gtk_tree_view_get_selection(m_ChannelTreeView));

            m_Renderer->SetChannelCount(nChannel);

            if (m_TypeID == OV_TypeId_Signal) { m_RendererCtx->SetDataType(CRendererContext::EDataType::Signal); }
            else if (m_TypeID == OV_TypeId_Spectrum) { m_RendererCtx->SetDataType(CRendererContext::EDataType::Spectrum); }
            else { m_RendererCtx->SetDataType(CRendererContext::EDataType::Matrix); }

            if (nSample != 1) {
                //bool warned = false;
                if (m_TypeID == OV_TypeId_Spectrum) {
                    //warned = true;
                    this->getLogManager() << Kernel::LogLevel_Warning << "Input matrix has 'spectrum' type\n";
                    this->getLogManager() << Kernel::LogLevel_Warning << "Such configuration is uncommon for a 'continous' kind of visualization !\n";
                    this->getLogManager() << Kernel::LogLevel_Warning <<
                            "You might want to consider the 'stacked' kind of visualization for time/frequency analysis for instance\n";
                    this->getLogManager() << Kernel::LogLevel_Warning << "Please double check your scenario\n";
                }
                else {
                    if (!m_RendererCtx->IsTimeLocked()) {
                        //warned = true;
                        this->getLogManager() << Kernel::LogLevel_Warning << "Input matrix has " << nSample
                                << " elements and the box settings say the elements are independant with " << m_NElement << " elements to render\n";
                        this->getLogManager() << Kernel::LogLevel_Warning <<
                                "Such configuration is uncommon for a 'continous' kind of visualization !\n";
                        this->getLogManager() << Kernel::LogLevel_Warning << "You might want either of the following alternative :\n";
                        this->getLogManager() << Kernel::LogLevel_Warning << " - an 'instant' kind of visualization to highlight the " << m_NElement
                                << " elements of the matrix\n";
                        this->getLogManager() << Kernel::LogLevel_Warning <<
                                " - a 'time locked' kind of elements (thus the scenario must refresh the matrix on a regular basis)\n";
                        this->getLogManager() << Kernel::LogLevel_Warning << "Please double check your scenario and box settings\n";
                    }
                }
            }

            m_RebuildNeeded = true;
            m_RefreshNeeded = true;
            m_RedrawNeeded  = true;
        }
        if (m_MatrixDecoder.isBufferReceived()) {
            m_Time1                 = m_Time2;
            m_Time2                 = boxContext.getInputChunkEndTime(0, i);
            const uint64_t duration = (m_Time2 - m_Time1) / nSample;
            if ((duration & ~0xf) != (m_RendererCtx->GetSampleDuration() & ~0xf) && duration != 0) // 0xf mask avoids rounding errors
            {
                m_RendererCtx->SetSampleDuration(duration);
            }
            m_RendererCtx->SetSpectrumFrequencyRange(
                size_t((uint64_t(nSample) << 32) / (boxContext.getInputChunkEndTime(0, i) - boxContext.getInputChunkStartTime(0, i))));
            m_RendererCtx->SetMinimumSpectrumFrequency(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_FrequencyBandMin))));
            m_RendererCtx->SetMaximumSpectrumFrequency(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(m_FrequencyBandMax))));

            // Feed renderer with actual samples
            for (size_t j = 0; j < nSample; ++j) {
                for (size_t k = 0; k < nChannel; ++k) { m_Swaps[k] = float(matrix->getBuffer()[k * nSample + j]); }
                m_Renderer->Feed(&m_Swaps[0]);
            }

            // Adjust feeding depending on theoretical dates
            if (m_RendererCtx->IsTimeLocked() && m_RendererCtx->GetSampleDuration()) {
                const auto nTheoreticalSample = size_t(m_Time2 / m_RendererCtx->GetSampleDuration());
                if (nTheoreticalSample > m_Renderer->GetHistoryCount()) { m_Renderer->Prefeed(nTheoreticalSample - m_Renderer->GetHistoryCount()); }
            }

            m_RefreshNeeded = true;
            m_RedrawNeeded  = true;
        }
    }

    const size_t nInput = this->getStaticBoxContext().getInputCount();
    if (nInput > 1) {
        for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
            m_StimulationDecoder.decode(i);
            if (m_StimulationDecoder.isBufferReceived()) {
                const CStimulationSet* stimSet = m_StimulationDecoder.getOutputStimulationSet();
                for (size_t j = 0; j < stimSet->size(); ++j) {
                    m_Renderer->Feed(stimSet->getDate(j), stimSet->getId(j));
                    m_RedrawNeeded = true;
                }
            }
        }
    }

    size_t rendererSampleCount = 0;
    if (m_RendererCtx->IsTimeLocked()) {
        if (0 != m_RendererCtx->GetSampleDuration()) { rendererSampleCount = size_t(m_RendererCtx->GetTimeScale() / m_RendererCtx->GetSampleDuration()); }
    }
    else {
        rendererSampleCount = size_t(m_RendererCtx->GetElementCount()); // *nSample;
    }

    if (rendererSampleCount != 0 && rendererSampleCount != m_Renderer->GetSampleCount()) {
        m_Renderer->SetSampleCount(rendererSampleCount);
        m_RebuildNeeded = true;
        m_RefreshNeeded = true;
        m_RedrawNeeded  = true;
    }

    if (m_RebuildNeeded) { m_Renderer->Rebuild(*m_RendererCtx); }
    if (m_RefreshNeeded) { m_Renderer->Refresh(*m_RendererCtx); }
    if (m_RedrawNeeded) { this->Redraw(); }

    m_RebuildNeeded = false;
    m_RefreshNeeded = false;
    m_RedrawNeeded  = false;

    return true;
}

template <class TRendererFactoryClass, class TRulerClass>
void TBoxAlgorithmContinuousViz<TRendererFactoryClass, TRulerClass>::Draw()

{
    CBoxAlgorithmViz::PreDraw();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor4f(m_Color.r, m_Color.g, m_Color.b, m_RendererCtx->GetTranslucency());
    m_Renderer->Render(*m_RendererCtx);
    glPopAttrib();

    CBoxAlgorithmViz::PostDraw();
}
}  // namespace AdvancedVisualization
}  // namespace OpenViBE
