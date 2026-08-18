//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include <mensia/advanced-visualization.hpp>
#include <TGtkGLWidget.hpp>
#include <GtkGL.hpp>

#include "StreamRendererSignal.h"

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

namespace OpenViBE {
namespace Tracker {

bool StreamRendererSignal::initialize()
{
    // TRendererStimulation < false, CRendererLine >:new CRendererLine
    AdvancedVisualization::IRenderer* pRenderer = AdvancedVisualization::IRenderer::Create(AdvancedVisualization::ERendererType::Line, true);
    if (pRenderer == nullptr) { return false; }

    const TypeSignal::Buffer* firstChunk = m_stream->getChunk(0);
    if (!firstChunk) { return false; }

    m_nChannel        = firstChunk->m_buffer.getDimensionSize(0);
    m_samplesPerChunk = firstChunk->m_buffer.getDimensionSize(1);
    m_chunkDuration   = firstChunk->m_EndTime - firstChunk->m_StartTime;

    // Creates renderer context
    m_rendererCtx = new AdvancedVisualization::CRendererContext();
    m_rendererCtx->Clear();
    m_rendererCtx->SetScale(1.0);
    m_rendererCtx->SetTimeScale(1);
    m_rendererCtx->SetCheckBoardVisibility(true);
    m_rendererCtx->SetScaleVisibility(m_isScaleVisible);
    m_rendererCtx->SetDataType(AdvancedVisualization::CRendererContext::EDataType::Signal);
    const CTime sampleDuration = CTime(m_chunkDuration.time() / m_samplesPerChunk);
    m_rendererCtx->SetSampleDuration(sampleDuration.time());

    const TypeSignal::Header& hdr = m_stream->getHeader();
    for (uint32_t i = 0; i < m_nChannel; ++i) {
        const char* label = hdr.m_Header.getDimensionLabel(0, i);
        m_rendererCtx->AddChannel(std::string(label));
    }

    m_ruler = new AdvancedVisualization::TRulerPair<AdvancedVisualization::CRulerProgressV, AdvancedVisualization::TRulerPair<
                                                        AdvancedVisualization::TRulerAutoType<
                                                            AdvancedVisualization::IRuler, AdvancedVisualization::TRulerConditionalPair<
                                                                AdvancedVisualization::CRulerBottomTime, AdvancedVisualization::CRulerBottomCount,
                                                                AdvancedVisualization::CRulerConditionIsTimeLocked>, AdvancedVisualization::IRuler>,
                                                        AdvancedVisualization::TRulerPair<
                                                            AdvancedVisualization::CRulerLeftChannelNames, AdvancedVisualization::CRulerRightScale>>>;

    m_ruler->SetRendererContext(m_rendererCtx);
    m_ruler->SetRenderer(pRenderer);

    if (!StreamRendererBase::initialize()) { return false; }

    m_gtkGLWidget.Initialize(*this, m_viewport, m_left, m_right, m_bottom);
    m_gtkGLWidget.SetPointSmoothingActive(false);

    m_renderers.push_back(pRenderer);

    return true;
}

bool StreamRendererSignal::reset(const CTime startTime, const CTime endTime)
{
    m_startTime = startTime;
    m_endTime   = endTime;

    //  std::cout << "Overridden signal renderer reset\n";

    m_renderers[0]->Clear(0);

    //  std::cout << "Start time is " << CTime(m_StartTime).toSeconds()
    //      << " end is " << CTime(m_EndTime).toSeconds() << "\n";

    const uint64_t chunkCount = (m_endTime - m_startTime).ceil().time() / m_chunkDuration.time();
    const uint32_t numSamples = uint32_t(m_samplesPerChunk * chunkCount);

    m_renderers[0]->SetSampleCount(numSamples);
    m_renderers[0]->SetChannelCount(m_nChannel);

    // @FIXME The offset is needed to have correct numbers on the ruler; remove ifdef once the feature is in
#ifdef RENDERER_SUPPORTS_OFFSET
    m_renderers[0]->SetTimeOffset(m_startTime.time());
#endif

    // m_renderers[0]->setHistoryDrawIndex(samplesBeforeStart);
    m_renderers[0]->Rebuild(*m_rendererCtx);

    return true;
}

bool StreamRendererSignal::push(const TypeSignal::Buffer& chunk, const bool zeroInput /* = false */)
{
    std::vector<float> tmp;
    if (!zeroInput) {
        tmp.resize(chunk.m_buffer.getBufferElementCount());
        for (size_t i = 0; i < chunk.m_buffer.getBufferElementCount(); ++i) { tmp[i] = float(chunk.m_buffer.getBuffer()[i]); }
    }
    else { tmp.resize(chunk.m_buffer.getBufferElementCount(), 0); }

    m_renderers[0]->Feed(&tmp[0], chunk.m_buffer.getDimensionSize(1));

    return true;
}

CString StreamRendererSignal::renderAsText(const size_t indent) const
{
    auto& hdr = m_stream->getHeader();

    std::stringstream ss;
    ss << std::string(indent, ' ') << "Sampling rate: " << hdr.m_Sampling << "hz" << std::endl;
    ss << std::string(indent, ' ') << "Channels: " << hdr.m_Header.getDimensionSize(0) << std::endl;
    ss << std::string(indent, ' ') << "Samples per chunk: " << hdr.m_Header.getDimensionSize(1) << std::endl;

    return ss.str().c_str();
}

bool StreamRendererSignal::MouseButton(const int x, const int y, const int button, const int status)
{
    //if (button == 3 && status == 1) { showChunkList(); }
    return StreamRendererBase::MouseButton(x, y, button, status);
}

bool StreamRendererSignal::showChunkList() { return showMatrixList<TypeSignal>(m_stream, &m_streamListWindow, "List of chunks for Signal stream"); }

}  // namespace Tracker
}  // namespace OpenViBE
