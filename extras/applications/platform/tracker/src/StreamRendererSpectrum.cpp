//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
// @todo add horizontal scaling support
// @todo add event handlers
// @todo add ruler, stimulations, channel names, a million of other things

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <mensia/advanced-visualization.hpp>
#include <GtkGL.hpp>

#include <system/ovCTime.h>

#include "StreamRendererSpectrum.h"

#include "ruler/TRulerAutoType.hpp"
#include "ruler/TRulerPair.hpp"
#include "ruler/TRulerConditionalPair.hpp"
#include "ruler/CRulerConditionIsTimeLocked.hpp"

#include "ruler/CRulerProgressV.hpp"

#include "ruler/CRulerBottomCount.hpp"
#include "ruler/CRulerBottomTime.hpp"

#include "ruler/CRulerLeftChannelNames.hpp"

#include "ruler/CRulerRightCount.hpp"
#include "ruler/CRulerRightScale.hpp"
#include "ruler/CRulerRightLabels.hpp"
#include "ruler/CRulerRightFrequency.hpp"

namespace OpenViBE {
namespace Tracker {

bool StreamRendererSpectrum::initialize()
{
    const TypeSpectrum::Buffer* firstChunk = m_stream->getChunk(0);
    if (!firstChunk) { return false; }

    m_nChannel         = firstChunk->m_buffer.getDimensionSize(0);
    m_spectrumElements = firstChunk->m_buffer.getDimensionSize(1);
    m_chunkDuration    = firstChunk->m_EndTime - firstChunk->m_StartTime;

    m_rendererCtx = new AdvancedVisualization::CRendererContext();
    m_rendererCtx->Clear();
    m_rendererCtx->SetTimeScale(1);
    m_rendererCtx->SetScaleVisibility(m_isScaleVisible);
    m_rendererCtx->SetCheckBoardVisibility(true);
    m_rendererCtx->SetTimeLocked(true);
    m_rendererCtx->SetDataType(AdvancedVisualization::CRendererContext::EDataType::Spectrum);
    m_rendererCtx->SetSampleDuration(m_chunkDuration.time());

    const TypeSpectrum::Header& hdr = m_stream->getHeader();
    for (uint32_t j = 0; j < m_nChannel; ++j) {
        const char* name = hdr.m_Header.getDimensionLabel(0, j);
        m_rendererCtx->AddChannel(std::string(name));
    }

    m_subRendererCtx = new AdvancedVisualization::CRendererContext();
    m_subRendererCtx->Clear();
    m_subRendererCtx->SetParentRendererContext(m_rendererCtx);
    m_subRendererCtx->SetTimeLocked(true);
    m_subRendererCtx->SetStackCount(m_nChannel);
    m_subRendererCtx->SetDataType(AdvancedVisualization::CRendererContext::EDataType::Spectrum);
    m_subRendererCtx->SetSampleDuration(m_chunkDuration.time());

    for (uint32_t j = 0; j < m_spectrumElements; ++j) {
        std::stringstream ss;
        ss << j;
        m_subRendererCtx->AddChannel(ss.str());
    }

    m_swaps.resize(m_spectrumElements);

    for (size_t i = 0; i < m_renderers.size(); ++i) { AdvancedVisualization::IRenderer::Release(m_renderers[i]); }
    m_renderers.clear();
    m_renderers.resize(m_nChannel);

    for (uint32_t j = 0; j < m_nChannel; ++j) {
        m_renderers[j] = AdvancedVisualization::IRenderer::Create(AdvancedVisualization::ERendererType::Bitmap, false);
        m_renderers[j]->SetChannelCount(m_spectrumElements);
    }

    m_ruler = new AdvancedVisualization::TRulerPair<AdvancedVisualization::TRulerConditionalPair<
                                                        AdvancedVisualization::CRulerBottomTime, AdvancedVisualization::CRulerBottomCount,
                                                        AdvancedVisualization::CRulerConditionIsTimeLocked>, AdvancedVisualization::TRulerPair<
                                                        AdvancedVisualization::TRulerAutoType<
                                                            AdvancedVisualization::IRuler, AdvancedVisualization::IRuler,
                                                            AdvancedVisualization::CRulerRightFrequency>, AdvancedVisualization::TRulerPair<
                                                            AdvancedVisualization::CRulerLeftChannelNames, AdvancedVisualization::CRulerProgressV>>>;
    m_ruler->SetRendererContext(m_rendererCtx);
    m_ruler->SetRenderer(m_renderers[0]);

    if (!StreamRendererBase::initialize()) { return false; }

    m_gtkGLWidget.Initialize(*this, m_viewport, m_left, m_right, m_bottom);
    m_gtkGLWidget.SetPointSmoothingActive(false);

    m_rotate = true;

    return true;
}


bool StreamRendererSpectrum::reset(const CTime startTime, const CTime endTime)
{
    m_startTime = startTime;
    m_endTime   = endTime;

    // Each spectrum buffer has one spectrum per channel, so numBuffers is just:
    // ( @todo: is this really needed in addition to setSampleCount? )
    const size_t numBuffers = (m_endTime - m_startTime).ceil().time() / m_chunkDuration.time();
    m_rendererCtx->SetElementCount(numBuffers);

    for (size_t j = 0; j < m_nChannel; ++j) {
        m_renderers[j]->Clear(0);
        m_renderers[j]->SetSampleCount(numBuffers); // $$$
    }

    //  @FIXME  The offset is needed to have correct numbers on the ruler; remove ifdef once the feature is in
#ifdef RENDERER_SUPPORTS_OFFSET
    m_renderers[0]->SetTimeOffset(m_startTime.time());
#endif

    return true;
}


// Spectrum chunks are organized as [channel x freqs], so to push
// with freqs on the y axis for each subrenderer, we transpose
bool StreamRendererSpectrum::push(const TypeSpectrum::Buffer& chunk, const bool zeroInput /* = false */)
{
#if 0
    std::cout << "Push spec chk " << m_Pushed << " " << chunk.m_buffer.getDimensionSize(0) << " " << chunk.m_buffer.getDimensionSize(1) << " "
        << CTime(chunk.m_startTime).toSeconds() << ","  << CTime(chunk.m_endTime).toSeconds() << "\n";
    std::cout << m_Pushed << " first bytes " << chunk.m_buffer.getBuffer()[0] << chunk.m_buffer.getBuffer()[1]
        << chunk.m_buffer.getBuffer()[2] << chunk.m_buffer.getBuffer()[3] << "\n";
    m_Pushed++;
#endif

    const size_t numFreq = chunk.m_buffer.getDimensionSize(1);

    m_rendererCtx->SetSpectrumFrequencyRange(uint32_t((uint64_t(numFreq) << 32) / m_chunkDuration.time()));

    for (uint32_t j = 0; j < m_renderers.size(); ++j) {
        if (!zeroInput) {
            // Feed renderer with actual samples
            for (uint32_t k = 0; k < numFreq; ++k) { m_swaps[numFreq - k - 1] = float(chunk.m_buffer.getBuffer()[j * numFreq + k]); }
        }
        else { std::fill(m_swaps.begin(), m_swaps.end(), 0.0F); }
        m_renderers[j]->Feed(&m_swaps[0]);
    }

    return true;
}


bool StreamRendererSpectrum::Draw()
{
    StreamRendererSpectrum::PreDraw();

    if (m_rendererCtx->GetSelectedCount() != 0) {
        glPushMatrix();
        glScalef(1, 1.0F / m_rendererCtx->GetSelectedCount(), 1);
        for (size_t i = 0; i < m_rendererCtx->GetSelectedCount(); ++i) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glPushMatrix();
            glColor4f(m_color.r, m_color.g, m_color.b, m_rendererCtx->GetTranslucency());
            glTranslatef(0, float(m_rendererCtx->GetSelectedCount() - i) - 1.0F, 0);
            if (!m_rotate) {
                glScalef(1, -1, 1);
                glRotatef(-90, 0, 0, 1);
            }
            m_subRendererCtx->SetAspect(m_rendererCtx->GetAspect());
            m_subRendererCtx->SetStackCount(m_rendererCtx->GetSelectedCount());
            m_subRendererCtx->SetStackIndex(i);
            m_renderers[m_rendererCtx->GetSelected(i)]->Render(*m_subRendererCtx);

            /*
            if (0)      //if (bDrawBorders)
            {
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
            */
            glPopMatrix();
            glPopAttrib();
        }
        glPopMatrix();
    }

    StreamRendererSpectrum::PostDraw();

    return true;
}

bool StreamRendererSpectrum::PreDraw()
{
    this->updateRulerVisibility();

    //  auto m_sColorGradient=CString("0:0,0,0; 100:100,100,100");
    const std::string gradient =
            "0:100, 100, 100; 12:50, 100, 100; 25:0, 50, 100; 38:0, 0, 50; 50:0, 0, 0; 62:50, 0, 0; 75:100, 50, 0; 88:100, 100, 50; 100:100, 100, 100";

    if (!m_textureID) { m_textureID = m_gtkGLWidget.CreateTexture(gradient); }
    glBindTexture(GL_TEXTURE_1D, m_textureID);

    m_rendererCtx->SetAspect(m_viewport->allocation.width * 1.0F / m_viewport->allocation.height);

    return true;
}

bool StreamRendererSpectrum::finalize()
{
    for (size_t i = 0; i < m_renderers.size(); ++i) {
        m_renderers[i]->Rebuild(*m_subRendererCtx);
        m_renderers[i]->Refresh(*m_subRendererCtx);
    }
    Redraw(true);

    return true;
}

CString StreamRendererSpectrum::renderAsText(const size_t indent) const
{
    auto& hdr = m_stream->getHeader();

    std::stringstream ss;
    ss << std::string(indent, ' ') << "Sampling rate: " << hdr.m_Sampling << "hz" << std::endl;
    ss << std::string(indent, ' ') << "Channels: " << hdr.m_Header.getDimensionSize(0) << std::endl;
    ss << std::string(indent, ' ') << "Abscissas per spectrum: " << hdr.m_Abscissas.getBufferElementCount() << std::endl;

    //  ss << std::string(indent, ' ') << "Channels: " << m_Header.m_header.getDimensionSize(0) << std::endl;
    //  ss << std::string(indent, ' ') << "Samples per chunk: " << m_Header.m_header.getDimensionSize(1) << std::endl;
    return ss.str().c_str();
}

bool StreamRendererSpectrum::MouseButton(const int x, const int y, const int button, const int status)
{
    //if (button == 3 && status == 1) { showChunkList(); }
    return StreamRendererBase::MouseButton(x, y, button, status);
}

bool StreamRendererSpectrum::showChunkList() { return showMatrixList<TypeSpectrum>(m_stream, &m_streamListWindow, "List of chunks for Spectrum stream"); }

}  // namespace Tracker
}  // namespace OpenViBE
