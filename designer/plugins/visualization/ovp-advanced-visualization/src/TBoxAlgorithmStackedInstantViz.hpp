///-------------------------------------------------------------------------------------------------
/// 
/// \file TBoxAlgorithmStackedInstantViz.hpp
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

#include <memory>

#include "CBoxAlgorithmViz.hpp"
#include "VisualizationTools.hpp"

#if defined TARGET_OS_Windows
#define snprintf _snprintf
#endif

namespace OpenViBE {
namespace AdvancedVisualization {
template <bool drawBorders, class TRendererFactoryClass, class TRulerClass>
class TBoxAlgorithmStackedInstantViz final : public CBoxAlgorithmViz
{
public:
    TBoxAlgorithmStackedInstantViz(const CIdentifier& classId, const std::vector<int>& parameters);
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(CBoxAlgorithmViz, m_ClassID)

    Toolkit::TStimulationDecoder<TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>> m_StimDecoder;
    Toolkit::TStreamedMatrixDecoder<TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>> m_MatrixDecoder;

    TRendererFactoryClass m_RendererFactory;
    std::vector<IRenderer*> m_Renderers;

protected:
    void Draw() override;
};

class CBoxAlgorithmStackedInstantVizListener final : public CBoxAlgorithmVizListener
{
public:
    explicit CBoxAlgorithmStackedInstantVizListener(const std::vector<int>& parameters) : CBoxAlgorithmVizListener(parameters) { }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        if (!this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_TimeFrequency)) { box.setInputType(index, OV_TypeId_TimeFrequency); }
        box.setInputType(1, OV_TypeId_Stimulations);
        return true;
    }
};

template <bool drawBorders, class TRendererFactoryClass, class TRulerClass = IRuler>
class TBoxAlgorithmStackedInstantVizDesc final : public CBoxAlgorithmVizDesc
{
public:
    TBoxAlgorithmStackedInstantVizDesc(const CString& name, const CIdentifier& descClassID, const CIdentifier& classID,
                                       const CParameterSet& parameterSet, const CString& shortDesc, const CString& detailedDesc)
        : CBoxAlgorithmVizDesc(name, descClassID, classID, parameterSet, shortDesc, detailedDesc) { }

    Plugins::IPluginObject* create() override
    {
        return new TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>(m_ClassID, m_Parameters);
    }

    Plugins::IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStackedInstantVizListener(m_Parameters); }

    CString getCategory() const override { return CString("Advanced Visualization/") + m_CategoryName; }

    _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, m_DescClassID)
};


template <bool drawBorders, class TRendererFactoryClass, class TRulerClass>
TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>::TBoxAlgorithmStackedInstantViz(
    const CIdentifier& classId, const std::vector<int>& parameters) : CBoxAlgorithmViz(classId, parameters) { }

template <bool drawBorders, class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>::initialize()

{
    const bool res = CBoxAlgorithmViz::initialize();

    m_MatrixDecoder.initialize(*this, 0);
    m_StimDecoder.initialize(*this, 1);

    m_RendererCtx->Clear();
    m_RendererCtx->SetTranslucency(float(m_Translucency));
    m_RendererCtx->ScaleBy(float(m_DataScale));
    m_RendererCtx->SetPositiveOnly(true);
    m_RendererCtx->SetAxisDisplay(m_ShowAxis);
    m_RendererCtx->SetParentRendererContext(&getContext());

    m_SubRendererCtx->Clear();
    m_SubRendererCtx->SetParentRendererContext(m_RendererCtx);

    m_Ruler = new TRulerClass;
    m_Ruler->SetRendererContext(m_RendererCtx);

    CMatrix gradientMatrix;
    VisualizationToolkit::ColorGradient::parse(gradientMatrix, m_ColorGradient);
    for (size_t step = 0; step < gradientMatrix.getDimensionSize(1); ++step) {
        const double currentStepValue            = gradientMatrix.getBuffer()[4 * step + 0];
        gradientMatrix.getBuffer()[4 * step + 0] = (currentStepValue / 100.0) * 50.0 + 50.0;
    }
    VisualizationToolkit::ColorGradient::format(m_ColorGradient, gradientMatrix);

    return res;
}

template <bool drawBorders, class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>::uninitialize()

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

template <bool drawBorders, class TRendererFactoryClass, class TRulerClass>
bool TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>::process()

{
    const Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
    const size_t nInput          = this->getStaticBoxContext().getInputCount();

    for (size_t chunk = 0; chunk < boxCtx.getInputChunkCount(0); ++chunk) {
        m_MatrixDecoder.decode(chunk);

        CMatrix* inputMatrix  = m_MatrixDecoder.getOutputMatrix();
        const size_t nChannel = inputMatrix->getDimensionSize(0);

        if (nChannel == 0) {
            this->getLogManager() << Kernel::LogLevel_Error << "Input stream " << chunk << " has 0 channels\n";
            return false;
        }

        if (m_MatrixDecoder.isHeaderReceived()) {
            for (auto renderer : m_Renderers) { m_RendererFactory.Release(renderer); }
            m_Renderers.clear();
            m_Renderers.resize(nChannel);

            m_SubRendererCtx->Clear();
            m_SubRendererCtx->SetParentRendererContext(m_RendererCtx);
            m_SubRendererCtx->SetTimeLocked(false);
            m_SubRendererCtx->SetStackCount(nChannel);
            m_SubRendererCtx->SetPositiveOnly(true);


            m_RendererCtx->Clear();
            m_RendererCtx->SetTranslucency(float(m_Translucency));
            m_RendererCtx->SetTimeScale(1);
            m_RendererCtx->ScaleBy(float(m_DataScale));
            m_RendererCtx->SetParentRendererContext(&getContext());
            m_RendererCtx->SetTimeLocked(false);
            m_RendererCtx->SetXyzPlotDepth(false);
            m_RendererCtx->SetPositiveOnly(true);

            if (m_TypeID == OV_TypeId_TimeFrequency) {
                GtkTreeIter gtkTreeIter;
                gtk_list_store_clear(m_ChannelListStore);

                const size_t frequencyCount = inputMatrix->getDimensionSize(1);
                const size_t nSample        = inputMatrix->getDimensionSize(2);

                // I do not know what this is for...
                for (size_t frequency = 0; frequency < frequencyCount; ++frequency) {
                    try {
                        const double frequencyValue = std::stod(inputMatrix->getDimensionLabel(1, frequency), nullptr);
                        const int stringSize        = snprintf(nullptr, 0, "%.2f", frequencyValue) + 1;
                        if (stringSize > 0) {
                            std::unique_ptr<char[]> buffer(new char[stringSize]);
                            snprintf(buffer.get(), size_t(stringSize), "%.2f", frequencyValue);
                            m_RendererCtx->SetDimensionLabel(1, frequencyCount - frequency - 1, buffer.get());
                        }
                    }
                    catch (...) { m_RendererCtx->SetDimensionLabel(1, frequencyCount - frequency - 1, "NaN"); }
                    m_SubRendererCtx->AddChannel("", 0, 0, 0);
                }


                m_RendererCtx->SetDataType(CRendererContext::EDataType::TimeFrequency);
                m_SubRendererCtx->SetDataType(CRendererContext::EDataType::TimeFrequency);

                m_RendererCtx->SetElementCount(nSample);
                m_SubRendererCtx->SetElementCount(nSample);
                gtk_tree_view_set_model(m_ChannelTreeView, nullptr);

                for (size_t channel = 0; channel < nChannel; ++channel) {
                    std::string channelName          = trim(inputMatrix->getDimensionLabel(0, channel));
                    std::string lowercaseChannelName = channelName;
                    std::transform(channelName.begin(), channelName.end(), lowercaseChannelName.begin(), tolower);
                    const CVertex v = m_ChannelPositions[lowercaseChannelName];

                    if (channelName.empty()) { channelName = "Channel " + std::to_string(channel + 1); }

                    m_Renderers[channel] = m_RendererFactory.Create();

                    // The channels in the sub-renderer are the frequencies in the spectrum
                    m_Renderers[channel]->SetChannelCount(frequencyCount);
                    m_Renderers[channel]->SetSampleCount(nSample);

                    m_RendererCtx->AddChannel(channelName, v.x, v.y, v.z);
                    gtk_list_store_append(m_ChannelListStore, &gtkTreeIter);
                    gtk_list_store_set(m_ChannelListStore, &gtkTreeIter, 0, channel + 1, 1, channelName.c_str(), -1);
                }
                gtk_tree_view_set_model(m_ChannelTreeView, GTK_TREE_MODEL(m_ChannelListStore));
                gtk_tree_selection_select_all(gtk_tree_view_get_selection(m_ChannelTreeView));
            }
            else {
                this->getLogManager() << Kernel::LogLevel_Error << "Input stream type is not supported\n";
                return false;
            }

            m_Ruler->SetRenderer(nChannel ? m_Renderers[0] : nullptr);

            m_RebuildNeeded = true;
            m_RefreshNeeded = true;
            m_RedrawNeeded  = true;
        }

        if (m_MatrixDecoder.isBufferReceived()) {
            if (m_TypeID == OV_TypeId_TimeFrequency) {
                m_Time1 = m_Time2;
                m_Time2 = boxCtx.getInputChunkEndTime(0, chunk);

                const size_t frequencyCount = inputMatrix->getDimensionSize(1);
                const size_t nSample        = inputMatrix->getDimensionSize(2);

                const uint64_t chunkDuration  = boxCtx.getInputChunkEndTime(0, chunk) - boxCtx.getInputChunkStartTime(0, chunk);
                const uint64_t sampleDuration = chunkDuration / nSample;

                m_SubRendererCtx->SetSampleDuration(sampleDuration);
                m_RendererCtx->SetSampleDuration(sampleDuration);

                for (size_t channel = 0; channel < nChannel; ++channel) {
                    // Feed renderer with actual samples
                    for (size_t sample = 0; sample < nSample; ++sample) {
                        m_Swaps.resize(frequencyCount);
                        for (size_t frequency = 0; frequency < frequencyCount; ++frequency) {
                            m_Swaps[frequencyCount - frequency - 1] = float(
                                inputMatrix->getBuffer()[sample + frequency * nSample + channel * nSample * frequencyCount]);
                        }
                        m_Renderers[channel]->Feed(&m_Swaps[0]);
                    }
                }

                m_RefreshNeeded = true;
                m_RedrawNeeded  = true;
            }
        }
    }

    if (nInput > 1) {
        for (size_t i = 0; i < boxCtx.getInputChunkCount(1); ++i) {
            m_StimDecoder.decode(i);
            if (m_StimDecoder.isBufferReceived()) {
                const CStimulationSet* stimSet = m_StimDecoder.getOutputStimulationSet();
                for (size_t j = 0; j < stimSet->size(); ++j) {
                    m_Renderers[0]->Feed(stimSet->getDate(j), stimSet->getId(j));
                    m_RedrawNeeded = true;
                }
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

template <bool drawBorders, class TRendererFactoryClass, class TRulerClass>
void TBoxAlgorithmStackedInstantViz<drawBorders, TRendererFactoryClass, TRulerClass>::Draw()

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
