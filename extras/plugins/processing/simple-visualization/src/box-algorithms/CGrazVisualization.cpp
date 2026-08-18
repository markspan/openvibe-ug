///-------------------------------------------------------------------------------------------------
/// 
/// \file CGrazVisualization.cpp
/// \brief Classes implementation for the Box Graz visualization.
/// \author Bruno Renier, Jussi T. Lindgren (INRIA/IRISA).
/// \version 0.2.
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

#include "CGrazVisualization.h"
#include "CGrazVisuWindow.h"
#include <tcptagging/IStimulusSender.h>
#include <visualization-toolkit/ovviz_all.h>

#include <algorithm> // std::min, max
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sys/timeb.h>

#if defined TARGET_OS_Linux
#include <unistd.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

// n.b. This reacts immediately to the received stimulation and doesn't use the date. Usually stimulations come from the upstream with
// chunks having a very narrow time range, so its alright for Graz that changes state only rarely. Note if multiple stimulations are 
// received in the same chunk, they'll be passed to TCP Tagging with the true delay between them lost.
void CGrazVisualization::setStimulation(const size_t /*stimulationIndex*/, const uint64_t identifier, const uint64_t /*stimulationDate*/)
{
    /*
    OVTK_GDF_Start_Of_Trial
    OVTK_GDF_Cross_On_Screen
    OVTK_GDF_Left
    OVTK_GDF_Right
    */
    bool stateUpdated = false;

    m_LastStimulation = identifier;
    switch (identifier) {
        case OVTK_GDF_End_Of_Trial:
            m_CurrentState = EStates::Idle;
            stateUpdated = true;
            if (m_ShowAccuracy || m_DelayFeedback) {
                const double prediction = aggregatePredictions(true);
                updateConfusionMatrix(prediction);
                m_BarScale = prediction;
            }
            break;

        case OVTK_GDF_End_Of_Session:
            m_CurrentState = EStates::Idle;
            stateUpdated = true;
            if (m_ShowFeedback) {
                m_BarScale = 0;
                if(m_window) {
                    m_window->launchQmlRedraw();
                };
            }
            break;

        case OVTK_GDF_Cross_On_Screen:
            m_CurrentState = EStates::Reference;
            stateUpdated = true;
            break;

        case OVTK_GDF_Beep:
            // gdk_beep();
            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Trace <<
                    "Beep is no more considered in 'Graz Visu', use the 'Sound player' for this!\n";
#if 0
#if defined TARGET_OS_Linux
        system("cat /local/ov_beep.wav > /dev/dsp &");
#endif
#endif
            break;

        case OVTK_GDF_Left:
            m_CurrentState = EStates::Cue;
            m_CurrentDirection = EArrowDirections::Left;
            stateUpdated       = true;
            break;

        case OVTK_GDF_Right:
            m_CurrentState = EStates::Cue;
            m_CurrentDirection = EArrowDirections::Right;
            stateUpdated       = true;
            break;

        case OVTK_GDF_Up:
            m_CurrentState = EStates::Cue;
            m_CurrentDirection = EArrowDirections::Up;
            stateUpdated       = true;
            break;

        case OVTK_GDF_Down:
            m_CurrentState = EStates::Cue;
            m_CurrentDirection = EArrowDirections::Down;
            stateUpdated       = true;
            break;

        case OVTK_GDF_Feedback_Continuous:
            // New trial starts

            m_CurrentState = EStates::ContinousFeedback;
            m_Amplitudes.clear();

        // as some trials may have artifacts and hence very high responses from e.g. LDA
        // its better to reset the max between trials
            m_MaxAmplitude = -DBL_MAX;
            m_BarScale     = 0;

            stateUpdated = true;
            break;
        default: break;
    }

    if (stateUpdated) { processState(); }

    // Queue the stimulation to be sent to TCP Tagging
    m_StimuliQueue.push_back(m_LastStimulation);
}

void CGrazVisualization::processState() const
{
    switch (m_CurrentState) {
        case EStates::Reference:
        case EStates::Cue:
        case EStates::Idle:
        case EStates::ContinousFeedback:
        //TODO

        /*  if (GTK_WIDGET(m_DrawingArea)->window) { gdk_window_invalidate_rect(GTK_WIDGET(m_DrawingArea)->window, nullptr, true); }
        */
            break;

        default:
            break;
    }
}

bool CGrazVisualization::initialize()
{
    m_StimulationDecoder.initialize(*this, 0);
    m_MatrixDecoder.initialize(*this, 1);
    m_confusionEncoder.initialize(*this, 0);
    m_oConfusion = m_confusionEncoder.getInputMatrix();

    m_ShowInstruction        = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_ShowFeedback           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_DelayFeedback          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_ShowAccuracy           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_PredictionsToIntegrate = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_PositiveFeedbackOnly   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    m_StimulusSender = nullptr;
    m_StimuliQueue.clear();

    if (m_PredictionsToIntegrate < 1) {
        this->getLogManager() << Kernel::LogLevel_Error << "Number of predictions to integrate must be at least 1!";
        return false;
    }

    m_oConfusion->resize(2, 2);

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_window = new CGrazVisuWindow(this, visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());
    std::vector<std::string> paths;
    std::string dataDir = Directories::getDataDir().toASCIIString();
    paths.push_back(dataDir + "/plugins/simple-visualization/openvibe-simple-visualization-GrazVisualization-leftArrow.png");
    paths.push_back(dataDir + "/plugins/simple-visualization/openvibe-simple-visualization-GrazVisualization-rightArrow.png");
    paths.push_back(dataDir + "/plugins/simple-visualization/openvibe-simple-visualization-GrazVisualization-upArrow.png");
    paths.push_back(dataDir + "/plugins/simple-visualization/openvibe-simple-visualization-GrazVisualization-downArrow.png");
    m_window->setImgsPaths(paths);  

    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_window->visualItem());
    } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }

    m_StimulusSender = TCPTagging::CreateStimulusSender();

    if (!m_StimulusSender->connect("localhost", "15361")) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
    }

    return true;
}

bool CGrazVisualization::uninitialize()
{
    //***** Print Confusion Matrix *****
    this->getLogManager() << Kernel::LogLevel_Info << "Confusion Matrix : \n";
    const double* buffer = m_oConfusion->getBuffer();
    std::stringstream ss;
    ss << std::setfill('0');
    size_t idx         = 0;
    size_t predictions = 0, good = 0;

    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            ss.str(std::string());
            const int val = int(buffer[idx++]);
            ss << std::setw(3) << val;
            predictions += val;
            this->getLogManager() << ss.str() << " ";
            if (i == j) { good += val; }
        }
        this->getLogManager() << "\n";
    }
    ss.str(std::string());
    ss << "Accuracy = " << std::fixed << std::setprecision(1) << (predictions == 0 ? 0.0 : 100.0 * double(good) / double(predictions)) << "\n";
    this->getLogManager() << ss.str();

    //***** Codecs *****
    m_oConfusion = nullptr;
    m_confusionEncoder.uninitialize();

    delete m_StimulusSender;

    m_StimulationDecoder.uninitialize();
    m_MatrixDecoder.uninitialize();

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }
    //destroy drawing area
    delete m_window;
    m_window = nullptr;
    return true;
}

bool CGrazVisualization::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CGrazVisualization::process()
{
    Kernel::IBoxIO* boxIO = getBoxAlgorithmContext()->getDynamicBoxContext();
    bool needRedraw = false;
    for (size_t chunk = 0; chunk < boxIO->getInputChunkCount(0); ++chunk) {
        m_StimulationDecoder.decode(chunk);
        if (m_StimulationDecoder.isBufferReceived()) {
            needRedraw = true;
            const CStimulationSet* stimSet = m_StimulationDecoder.getOutputStimulationSet();
            for (size_t s = 0; s < stimSet->size(); ++s) { setStimulation(s, stimSet->getId(s), stimSet->getDate(s)); }
        }
    }

    for (size_t chunk = 0; chunk < boxIO->getInputChunkCount(1); ++chunk) {
        m_MatrixDecoder.decode(chunk);
        needRedraw = true;
        if (m_MatrixDecoder.isHeaderReceived()) {
            const CMatrix* matrix = m_MatrixDecoder.getOutputMatrix();

            if (matrix->getDimensionCount() == 0) {
                this->getLogManager() << Kernel::LogLevel_Error << "Error, dimension count is 0 for Amplitude input !\n";
                return false;
            }

            if (matrix->getDimensionCount() > 1) {
                for (size_t k = 1; k < matrix->getDimensionSize(k); ++k) {
                    if (matrix->getDimensionSize(k) > 1) {
                        this->getLogManager() << Kernel::LogLevel_Error << "Error, only column vectors supported as Amplitude!\n";
                        return false;
                    }
                }
            }

            if (matrix->getDimensionSize(0) == 0) {
                this->getLogManager() << Kernel::LogLevel_Error << "Error, need at least 1 dimension in Amplitude input !\n";
                return false;
            }
            if (matrix->getDimensionSize(0) >= 2) {
                this->getLogManager() << Kernel::LogLevel_Trace <<
                        "Got 2 or more dimensions for feedback, feedback will be the difference between the first two.\n";
                m_TwoValueInput = true;
            }
            m_confusionEncoder.encodeHeader();
        }

        if (m_MatrixDecoder.isBufferReceived()) {
            setMatrixBuffer(m_MatrixDecoder.getOutputMatrix()->getBuffer());
            m_confusionEncoder.encodeBuffer();
        }
        if (m_MatrixDecoder.isEndReceived()) { m_confusionEncoder.encodeBuffer(); }
        boxIO->markOutputAsReadyToSend(0, boxIO->getInputChunkStartTime(1, chunk), boxIO->getInputChunkEndTime(1, chunk));
    }

    if(needRedraw) {
        if (m_window) { m_window->launchQmlRedraw(); }
    }
    return true;
}

double CGrazVisualization::aggregatePredictions(const bool includeAll)
{
    double voteAggregate = 0;

    // Do we have enough predictions to integrate a result?
    if (m_Amplitudes.size() >= m_PredictionsToIntegrate) {
        // step backwards with rev iter to take the latest samples
        uint64_t count = 0;
        for (auto a = m_Amplitudes.rbegin(); a != m_Amplitudes.rend() && (includeAll || count < m_PredictionsToIntegrate); ++a, ++count) {
            voteAggregate += *a;
            m_MaxAmplitude = std::max<double>(m_MaxAmplitude, abs(*a));
        }

        voteAggregate /= m_MaxAmplitude;
        voteAggregate /= double(count);
    }

    return voteAggregate;
}

// @fixme for >2 classes
void CGrazVisualization::updateConfusionMatrix(const double prediction)
{
    if (m_CurrentDirection == EArrowDirections::Left || m_CurrentDirection == EArrowDirections::Right) {
        const size_t direction = size_t(m_CurrentDirection) - 1;
        const size_t vote      = (prediction < 0 ? 0 : 1);

        (m_oConfusion->getBuffer())[direction * 2 + vote]++;
        // std::cout << "Now " << trueDirection  << " vote " << thisVote << "\n";
    }
}

void CGrazVisualization::setMatrixBuffer(const double* buffer)
{
    if (m_CurrentState != EStates::ContinousFeedback) {
        // We're not inside a trial, discard the prediction
        return;
    }

    double predictedAmplitude;
    if (m_TwoValueInput) {
        // Ad-hoc forcing to probability (range [0,1], sum to 1). This will make scaling easier 
        // if run forever in a continuous mode. If the input is already scaled this way, no effect.
        // 
        double v0        = std::abs(buffer[0]), v1 = std::abs(buffer[1]);
        const double sum = v0 + v1;
        if (std::fabs(sum) > 0) {
            v0 = v0 / sum;
            v1 = v1 / sum;
        }
        else {
            v0 = 0.5;
            v1 = 0.5;
        }
        predictedAmplitude = v1 - v0;
    }
    else { predictedAmplitude = buffer[0]; }
    m_Amplitudes.push_back(predictedAmplitude);

    if (m_ShowFeedback && !m_DelayFeedback) {
        m_BarScale = aggregatePredictions(false);
        if(m_window) {
            m_window->launchQmlRedraw();
        }
    }
}

// Note that we don't need concurrency control here as gtk callbacks run in the main thread
void CGrazVisualization::FlushQueue()
{
    for (const auto& stimulation : m_StimuliQueue) { m_StimulusSender->sendStimulation(stimulation); }
    m_StimuliQueue.clear();
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
