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

#include "StreamRendererMatrix.h"

#include "VisualizationTools.hpp"
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

#include "ruler/CRulerRightTexture.hpp"

namespace OpenViBE {
namespace Tracker {

bool StreamRendererMatrix::initialize()
{
    const TypeMatrix::Buffer* firstChunk = m_stream->getChunk(0);
    if (firstChunk) {
        m_chunkDuration = firstChunk->m_EndTime - firstChunk->m_StartTime;

        if (firstChunk->m_buffer.getDimensionCount() == 1) {
            // Degenerate case like a feature vector
            m_nRows = firstChunk->m_buffer.getDimensionSize(0);
            m_nCols = 0;
        }
        else if (firstChunk->m_buffer.getDimensionCount() == 2) {
            m_nRows = firstChunk->m_buffer.getDimensionSize(0);
            m_nCols = firstChunk->m_buffer.getDimensionSize(1);
        }
        else if (firstChunk->m_buffer.getDimensionCount() > 2) {
            std::cout << "Warning: The matrix renderer does not work correctly if dims>2 (for tensors)\n";
            m_nRows = firstChunk->m_buffer.getDimensionSize(0);
            m_nCols = firstChunk->m_buffer.getDimensionSize(1);
        }
        else { log() << Kernel::LogLevel_Error << "Error: Dimension count " << firstChunk->m_buffer.getDimensionCount() << " not supported\n"; }
    }
    else {
        std::cout << "Stream is empty\n";
        return false;
    }


    m_rendererCtx = new AdvancedVisualization::CRendererContext();
    m_rendererCtx->Clear();
    m_rendererCtx->SetTimeScale(1);
    m_rendererCtx->SetScaleVisibility(m_isScaleVisible);
    m_rendererCtx->SetCheckBoardVisibility(true);
    m_rendererCtx->SetTimeLocked(true);
    m_rendererCtx->SetPositiveOnly(false);
    m_rendererCtx->SetDataType(AdvancedVisualization::CRendererContext::EDataType::Matrix);
    m_rendererCtx->SetSampleDuration(m_chunkDuration.time());
    m_rendererCtx->SetParentRendererContext(&AdvancedVisualization::getContext());

    m_rendererCtx->SetAxisDisplay(true);

    auto& hdr = m_stream->getHeader();

    for (uint32_t j = 0; j < m_nRows; ++j) { m_rendererCtx->AddChannel(std::string(hdr.m_Header.getDimensionLabel(0, j))); }

    m_swap.resize(m_nRows);

    m_renderers.push_back(AdvancedVisualization::IRenderer::Create(AdvancedVisualization::ERendererType::Bitmap, false));

    m_ruler = new AdvancedVisualization::TRulerPair<AdvancedVisualization::CRulerProgressV, AdvancedVisualization::TRulerPair<
                                                        AdvancedVisualization::TRulerAutoType<
                                                            AdvancedVisualization::IRuler, AdvancedVisualization::TRulerConditionalPair<
                                                                AdvancedVisualization::CRulerBottomTime, AdvancedVisualization::CRulerBottomCount,
                                                                AdvancedVisualization::CRulerConditionIsTimeLocked>, AdvancedVisualization::IRuler>,
                                                        AdvancedVisualization::TRulerPair<
                                                            AdvancedVisualization::CRulerLeftChannelNames, AdvancedVisualization::CRulerRightTexture>>>;

    //  m_pRuler = new TRulerPair < TRulerConditionalPair < CRulerBottomTime, CRulerBottomCount, CRulerConditionIsTimeLocked >, TRulerPair < TRulerAutoType < IRuler, IRuler, CRulerRightFrequency >, TRulerPair < CRulerLeftChannelNames, CRulerProgressV > > >;
    m_ruler->SetRendererContext(m_rendererCtx);
    m_ruler->SetRenderer(m_renderers[0]);

    if (!StreamRendererBase::initialize()) { return false; }

    m_gtkGLWidget.Initialize(*this, m_viewport, m_left, m_right, m_bottom);
    m_gtkGLWidget.SetPointSmoothingActive(false);

    return true;
}


bool StreamRendererMatrix::reset(const CTime startTime, const CTime endTime)
{
    m_startTime = startTime;
    m_endTime   = endTime;

    const uint32_t numBuffers = ((m_endTime - m_startTime).ceil().time() / m_chunkDuration.time());

    m_rendererCtx->SetElementCount(numBuffers);

    m_renderers[0]->Clear(0);
    m_renderers[0]->SetSampleCount(numBuffers); // $$$
    m_renderers[0]->SetChannelCount(m_nRows);

    //  @FIXME  The offset is needed to have correct numbers on the ruler; remove ifdef once the feature is in
#ifdef RENDERER_SUPPORTS_OFFSET
    m_renderers[0]->SetTimeOffset(m_startTime.time());
#endif

    return true;
}


bool StreamRendererMatrix::push(const TypeMatrix::Buffer& chunk, bool /*zeroInput*/)
{
#if 0
    static uint32_t pushed = 0;
    std::cout << "Push spec chk " << pushed << " " << chunk.m_buffer.getDimensionSize(0)
        << " " << chunk.m_buffer.getDimensionSize(1) << " "
        << CTime(chunk.m_startTime).toSeconds() << "," 
        << CTime(chunk.m_endTime).toSeconds()
        << "\n";
    std::cout << pushed << " first bytes "
        << chunk.m_buffer.getBuffer()[0]
        << chunk.m_buffer.getBuffer()[1]
        << chunk.m_buffer.getBuffer()[2]
        << chunk.m_buffer.getBuffer()[3]
        << "\n";
    pushed++;
#endif


    m_rendererCtx->SetSpectrumFrequencyRange(uint32_t((uint64_t(m_nRows) << 32) / m_chunkDuration.time()));

    // Handle the degenerate case NumCols=0
    const size_t actualCols = std::max<size_t>(m_nCols, 1);

    // Feed renderer with actual samples
    for (uint32_t j = 0; j < actualCols; ++j) {
        for (uint32_t k = 0; k < m_nRows; ++k) { m_swap[k] = float(chunk.m_buffer.getBuffer()[k * actualCols + j]); }
        m_renderers[0]->Feed(&m_swap[0]);
    }

    return true;
}


bool StreamRendererMatrix::Draw()
{
    StreamRendererMatrix::PreDraw();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor4f(m_color.r, m_color.g, m_color.b, m_rendererCtx->GetTranslucency());
    m_renderers[0]->Render(*m_rendererCtx);
    glPopAttrib();

    StreamRendererMatrix::PostDraw();

    return true;
}

bool StreamRendererMatrix::PreDraw()
{
    this->updateRulerVisibility();

    //  auto m_sColorGradient=CString("0:0,0,0; 100:100,100,100");
    const std::string gradient = "0:0, 0, 50; 25:0, 100, 100; 50:0, 50, 0; 75:100, 100, 0; 100:75, 0, 0";
    //  auto m_sColorGradient = CString("0:100, 100, 100; 12:50, 100, 100; 25:0, 50, 100; 38:0, 0, 50; 50:0, 0, 0; 62:50, 0, 0; 75:100, 50, 0; 88:100, 100, 50; 100:100, 100, 100");

    if (!m_textureID) { m_textureID = m_gtkGLWidget.CreateTexture(gradient); }
    glBindTexture(GL_TEXTURE_1D, m_textureID);

    m_rendererCtx->SetAspect(m_viewport->allocation.width * 1.0F / m_viewport->allocation.height);

    return true;
}

bool StreamRendererMatrix::finalize()
{
    m_renderers[0]->Rebuild(*m_rendererCtx);
    m_renderers[0]->Refresh(*m_rendererCtx);

    Redraw(true);

    return true;
}

CString StreamRendererMatrix::renderAsText(const size_t indent) const
{
    std::stringstream ss;
    ss << std::string(indent, ' ') << "Rows: " << m_nRows << std::endl;
    ss << std::string(indent, ' ') << "Cols: " << m_nCols << std::endl;

    //  ss << std::string(indent, ' ') << "Channels: " << m_Header.m_header.getDimensionSize(0) << std::endl;
    //  ss << std::string(indent, ' ') << "Samples per chunk: " << m_Header.m_header.getDimensionSize(1) << std::endl;
    return ss.str().c_str();
}

bool StreamRendererMatrix::MouseButton(const int x, const int y, const int button, const int status)
{
    //if (button == 3 && status == 1) { showChunkList(); }
    return StreamRendererBase::MouseButton(x, y, button, status);
}

bool StreamRendererMatrix::showChunkList() { return showMatrixList<TypeMatrix>(m_stream, &m_streamListWindow, "List of chunks for Matrix stream"); }

}  // namespace Tracker
}  // namespace OpenViBE
