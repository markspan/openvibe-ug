///-------------------------------------------------------------------------------------------------
/// 
/// \file CDisplayCueImage.cpp
/// \brief Classes implementation for the Box Display cue image.
/// \author Joan Fruitet, Jussi T. Lindgren (Inria Sophia, Inria Rennes).
/// \version 1.2.
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

#include "CDisplayCueImage.h"

#include <algorithm> // std::min

#include <tcptagging/IStimulusSender.h>
#include <visualization-toolkit/ovviz_all.h>

#if defined TARGET_OS_Linux
  #include <unistd.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

bool CDisplayCueImage::initialize()
{
    m_idleFuncTag    = 0;
    m_stimulusSender = nullptr;

    //Number of Cues:
    m_numberOfCues = (getStaticBoxContext().getSettingCount() - NON_CUE_SETTINGS_COUNT) / 2;

    //Do we display the images in full screen?
    m_fullScreen  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_scaleImages = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    //Clear screen stimulation:
    m_clearScreenStimulation = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    //Stimulation ID and images file names for each cue
    m_imageNames.resize(m_numberOfCues);
    m_stimulationsIds.resize(m_numberOfCues);
    for (size_t i = 0; i < m_numberOfCues; ++i) {
        m_imageNames[i]      = std::string(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), NON_CUE_SETTINGS_COUNT + 2 * i));
        m_stimulationsIds[i] = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), NON_CUE_SETTINGS_COUNT + 2 * i + 1);
    }

    //>>>> Initialisation
    m_stimulationDecoder.initialize(*this, 0);
    m_stimulationEncoder.initialize(*this, 0);

    m_stimuliQueue.clear();

    m_stimulusSender = TCPTagging::CreateStimulusSender();

    if (!m_stimulusSender->connect("localhost", "15361")) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
    }

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_imageDisplayObject = new CImageDisplayObject(this->getLogManager(), m_scaleImages, visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());

    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_imageDisplayObject->visualItem());
    } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }

    return true;
}

bool CDisplayCueImage::uninitialize()
{

    m_stimulationDecoder.uninitialize();
    m_stimulationEncoder.uninitialize();

    if (m_stimulusSender) {
        delete m_stimulusSender;
        m_stimulusSender = nullptr;
    }

    m_stimulationsIds.clear();
    m_imageNames.clear();

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }

    delete m_imageDisplayObject;
    m_imageDisplayObject = nullptr;

    return true;
}

bool CDisplayCueImage::processClock(Kernel::CMessageClock& /*msg*/)
{
    Kernel::IBoxIO* boxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
    m_stimulationEncoder.getInputStimulationSet()->clear();

    if (this->getPlayerContext().getCurrentTime() == 0) {
        // Always send header first
        m_stimulationEncoder.encodeHeader();
        boxIO->markOutputAsReadyToSend(0, 0, 0);
    }

    if (m_imageDrawn) {
        // this is first redraw() for that image or clear screen
        // we send a stimulation to signal it. 
        // @note this practice is deprecated, the TCP Tagging should be used. We pass the stimulus here for compatibility.

        if (m_drawnImageId >= 0) {
            // it was a image
            m_stimulationEncoder.getInputStimulationSet()->push_back(m_stimulationsIds[m_drawnImageId], this->getPlayerContext().getCurrentTime(), 0);
        }
        else {
            // it was a clear_screen
            m_stimulationEncoder.getInputStimulationSet()->push_back(m_clearScreenStimulation, this->getPlayerContext().getCurrentTime(), 0);
        }

        m_imageDrawn = false;

        if (m_drawnImageId != m_requestedImageId) {
            // We must be late...
            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning <<
                    "One image may have been skipped => we must be late...\n";
        }
    }

    m_stimulationEncoder.encodeBuffer();
    boxIO->markOutputAsReadyToSend(0, m_lastOutputChunkDate, this->getPlayerContext().getCurrentTime());
    m_lastOutputChunkDate = this->getPlayerContext().getCurrentTime();

    bool stimulusMatchedBefore = false;

    // We check if some images must be displayed
    for (size_t stim = 0; stim < m_pendingStimulationSet.size();) {
        const uint64_t date = m_pendingStimulationSet.getDate(stim);
        const uint64_t time = this->getPlayerContext().getCurrentTime();
        if (date < time) {
            const uint64_t id       = m_pendingStimulationSet.getId(stim);
            bool stimulusMatchedNow = false;
            if (id == m_clearScreenStimulation) {
                if (m_imageRequested) {
                    getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_ImportantWarning <<
                            "Clear screen was received before previous cue image in slot " << m_requestedImageId + 1 << " was displayed!!\n";
                }
                m_imageRequested      = true;
                stimulusMatchedBefore = true;
                stimulusMatchedNow    = true;
                m_requestedImageId    = -1;
            }
            else {
                for (size_t i = 0; i < m_numberOfCues; ++i) {
                    if (id == m_stimulationsIds[i]) {
                        if (m_imageRequested) {
                            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_ImportantWarning << "Previous request of slot " <<
                                    m_requestedImageId + 1 << " image was replaced by request for slot " << i + 1 <<
                                    " => Not enough time between two images!!\n";
                        }
                        m_imageRequested      = true;
                        stimulusMatchedBefore = true;
                        stimulusMatchedNow    = true;
                        m_requestedImageId    = int(i);
                        break;
                    }
                }
            }

            if (stimulusMatchedNow) {
                // Queue the recognized stimulation to TCP Tagging to be sent after rendering
                const uint64_t sentStimulation = (m_requestedImageId >= 0 ? m_stimulationsIds[m_requestedImageId] : m_clearScreenStimulation);
                m_stimuliQueue.push_back(sentStimulation);
            }
            else {
                // Pass unrecognized stimulations to TCP Tagging. Be careful when modifying the code that the
                // stimuli received by AS keep their original time order despite the delays introduced to rendered stimuli.

                if (stimulusMatchedBefore) {
                    // We have queued a cue to be drawn, so we should delay this stimulation to TCP Tagging to be processed after the cue rendering to keep the time order
                    m_stimuliQueue.push_back(id);
                }
                else {
                    // We have not yet queued anything to be drawn, so we can forward immediately.
                    m_stimulusSender->sendStimulation(id);
                }
            }

            const double delay = CTime(time - date).toSeconds() * 1000; // delay in ms
            if (delay > 50) {
                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning << "Stimulation " << id <<
                        " was late in processClock() : " << delay << " ms \n";
            }

            m_pendingStimulationSet.erase(stim);
        }
        else {
            // Stim is still in the future, skip it for now
            stim++;
        }
    }

    // We should show the cue image now.
    Redraw();
    return true;
}

bool CDisplayCueImage::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CDisplayCueImage::process()
{
    const Kernel::IBoxIO* boxIO = getBoxAlgorithmContext()->getDynamicBoxContext();

    // We decode and save the received stimulations.
    for (size_t input = 0; input < getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount(); ++input) {
        for (size_t chunk = 0; chunk < boxIO->getInputChunkCount(input); ++chunk) {
            m_stimulationDecoder.decode(chunk, true);
            if (m_stimulationDecoder.isHeaderReceived()) {
                // nop
            }
            if (m_stimulationDecoder.isBufferReceived()) {
                for (size_t stim = 0; stim < m_stimulationDecoder.getOutputStimulationSet()->size(); ++stim) {
                    // We always add the stimulations to the set to allow passing them to TCP Tagging in order in processClock()
                    const uint64_t id       = m_stimulationDecoder.getOutputStimulationSet()->getId(stim);
                    const uint64_t date     = m_stimulationDecoder.getOutputStimulationSet()->getDate(stim);
                    const uint64_t duration = m_stimulationDecoder.getOutputStimulationSet()->getDuration(stim);

                    const uint64_t time = this->getPlayerContext().getCurrentTime();
                    if (date < time) {
                        const double delay = CTime(time - date).toSeconds() * 1000; //delay in ms
                        if (delay > 50) {
                            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning << "Stimulation " << id <<
                                    " was received late: " << delay << " ms \n";
                        }
                    }

                    if (date < boxIO->getInputChunkStartTime(input, chunk)) {
                        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Input Stimulation Date before beginning of the buffer\n";
                    }

                    m_pendingStimulationSet.push_back(id, date, duration);
                }
            }
        }
    }

    return true;
}

//Callback called by GTK
void CDisplayCueImage::Redraw()
{
    if (m_requestedImageId < 0) {
        m_imageDisplayObject->setImage("");
    } else {
        m_imageDisplayObject->setImage(m_imageNames[m_requestedImageId]);
    }

    if (m_imageRequested) {
        m_imageRequested = false;
        m_imageDrawn     = true;
        m_drawnImageId   = m_requestedImageId;
    }
}

// Note that we don't need concurrency control here as gtk callbacks run in the main thread
void CDisplayCueImage::FlushQueue()
{
    for (const auto& stimulation : m_stimuliQueue) { m_stimulusSender->sendStimulation(stimulation); }
    m_stimuliQueue.clear();

    // This function will be automatically removed after completion, so set to 0
    m_idleFuncTag = 0;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
