///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMouseTracking.cpp
/// \author Alison Cellard (Inria)
/// \version 1.0.
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

#if defined(TARGET_HAS_ThirdPartyGTK)

#include "CBoxAlgorithmMouseTracking.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Tools {

void MotionEventHandler(GtkWidget* widget, const GdkEventMotion* motion, gpointer data);

bool CBoxAlgorithmMouseTracking::initialize()
{
    // Feature vector stream encoder
    m_algo0SignalEncoder.initialize(*this, 0);
    // Feature vector stream encoder
    m_algo1SignalEncoder.initialize(*this, 1);

    m_algo0SignalEncoder.getInputMatrix().setReferenceTarget(m_absoluteCoordinateBuffer);
    m_algo1SignalEncoder.getInputMatrix().setReferenceTarget(m_relativeCoordinateBuffer);

    // Allocates coordinates Matrix
    m_absoluteCoordinateBuffer = new CMatrix();
    m_relativeCoordinateBuffer = new CMatrix();

    // Retrieves settings
    m_sampling              = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_nGeneratedEpochSample = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_nSentSample = 0;
    m_MouseX      = 0;
    m_MouseY      = 0;
    m_prevX       = 0;
    m_prevY       = 0;

    // Creates empty window to get mouse position
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_add_events(m_window, GDK_POINTER_MOTION_MASK);
    gtk_window_maximize(GTK_WINDOW(m_window));
    gtk_widget_show_all(m_window);

    g_signal_connect(m_window, "motion-notify-event", G_CALLBACK(MotionEventHandler), this);

    m_headerSent = false;

    m_algo0SignalEncoder.getInputSamplingRate() = m_sampling;
    m_algo1SignalEncoder.getInputSamplingRate() = m_sampling;

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmMouseTracking::uninitialize()
{
    m_algo0SignalEncoder.uninitialize();
    m_algo1SignalEncoder.uninitialize();

    delete m_absoluteCoordinateBuffer;
    m_absoluteCoordinateBuffer = nullptr;

    delete m_relativeCoordinateBuffer;
    m_relativeCoordinateBuffer = nullptr;

    gtk_widget_destroy(m_window);
    m_window = nullptr;

    return true;
}
/*******************************************************************************/


bool CBoxAlgorithmMouseTracking::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
/*******************************************************************************/


uint64_t CBoxAlgorithmMouseTracking::getClockFrequency()
{
    // Intentional parameter swap to get the frequency
    m_clockFrequency = CTime(m_nGeneratedEpochSample, m_sampling).time();

    return m_clockFrequency;
}

/*******************************************************************************/

bool CBoxAlgorithmMouseTracking::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //Send header and initialize Matrix
    if (!m_headerSent) {
        m_absoluteCoordinateBuffer->resize(2, m_nGeneratedEpochSample);
        m_absoluteCoordinateBuffer->setDimensionLabel(0, 0, "x");
        m_absoluteCoordinateBuffer->setDimensionLabel(0, 1, "y");

        m_relativeCoordinateBuffer->resize(2, m_nGeneratedEpochSample);
        m_relativeCoordinateBuffer->setDimensionLabel(0, 0, "x");
        m_relativeCoordinateBuffer->setDimensionLabel(0, 1, "y");


        m_algo0SignalEncoder.encodeHeader();
        m_algo1SignalEncoder.encodeHeader();

        m_headerSent = true;

        boxContext.markOutputAsReadyToSend(0, 0, 0);
        boxContext.markOutputAsReadyToSend(1, 0, 0);
    }
    else //Do the process and send coordinates to output
    {
        const size_t nSentSample = m_nSentSample;


        for (size_t i = 0; i < m_nGeneratedEpochSample; ++i) {
            m_absoluteCoordinateBuffer->getBuffer()[0 * m_nGeneratedEpochSample + i] = m_MouseX;
            m_absoluteCoordinateBuffer->getBuffer()[1 * m_nGeneratedEpochSample + i] = m_MouseY;

            m_relativeCoordinateBuffer->getBuffer()[0 * m_nGeneratedEpochSample + i] = m_MouseX - m_prevX;
            m_relativeCoordinateBuffer->getBuffer()[1 * m_nGeneratedEpochSample + i] = m_MouseY - m_prevY;
        }


        m_prevX = m_MouseX;
        m_prevY = m_MouseY;

        m_nSentSample += m_nGeneratedEpochSample;

        const uint64_t tStart = CTime(m_sampling, nSentSample).time();
        const uint64_t tEnd   = CTime(m_sampling, m_nSentSample).time();

        m_algo0SignalEncoder.encodeBuffer();
        m_algo1SignalEncoder.encodeBuffer();

        boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
        boxContext.markOutputAsReadyToSend(1, tStart, tEnd);
    }
    return true;
}

// CALLBACK
// Get mouse position
void MotionEventHandler(GtkWidget* /*widget*/, const GdkEventMotion* motion, gpointer data)
{
    CBoxAlgorithmMouseTracking* box = static_cast<CBoxAlgorithmMouseTracking*>(data);
    box->m_MouseX                   = double(motion->x);
    box->m_MouseY                   = double(motion->y);
}

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
#endif
