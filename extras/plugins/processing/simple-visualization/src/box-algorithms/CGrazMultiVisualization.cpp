///-------------------------------------------------------------------------------------------------
/// 
/// \file CGrazMultiVisualization.cpp
/// \brief Classes implementation for the Generalized Graz Visualization box.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 28/05/2019.
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

#include "CGrazMultiVisualization.h"
#include "CGrazMultiVisuWindow.h"
#include <visualization-toolkit/ovviz_all.h>

#include <algorithm> // std::min, max
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional> // greater
#include <iomanip>
#include <sys/timeb.h>
#include <tcptagging/IStimulusSender.h>
#include <vector>

#if defined TARGET_OS_Linux
#include <unistd.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

//******************************        
//***** OPENVIBE FUNCTIONS *****    
//******************************    
//---------------------------------------------------------------------------------------------------
bool CGrazMultiVisualization::initialize()
{
    //***** Codecs *****
    m_stimDecoder.initialize(*this, 0);
    m_classifDecoder.initialize(*this, 1);
    m_barSizeEncoder.initialize(*this, 0);
    m_confusionEncoder.initialize(*this, 1);
    m_iStim      = m_stimDecoder.getOutputStimulationSet();
    m_iMatrix    = m_classifDecoder.getOutputMatrix();
    m_oBarSize   = m_barSizeEncoder.getInputMatrix();
    m_oConfusion = m_confusionEncoder.getInputMatrix();

    //***** Settings *****
    m_showInstruction  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_feedbackMode     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_delayFeedback    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_showAccuracy     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_nbPredictionsMin = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    m_nbModality = (getStaticBoxContext().getSettingCount() - NON_MODALITY_SETTINGS_COUNT) / 2;
    m_barScales.resize(m_nbModality);
    for (auto& s : m_barScales) { s = 0; }
    std::vector<std::string> paths;
    paths.reserve(m_nbModality + 1);
    m_stimlist.reserve(m_nbModality);

    size_t idx = NON_MODALITY_SETTINGS_COUNT - 1; //the last non modality setting is for the none instruction feedback image
    paths.emplace_back(CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), idx++)).toASCIIString());

    //Stimulation ID and images file names for each modality
    for (size_t i = 0; i < m_nbModality; ++i) {
        m_stimlist.emplace_back(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), idx++)));
        paths.emplace_back(CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), idx++)).toASCIIString());
    }

    m_oConfusion->resize(m_nbModality, m_nbModality);   // Confusion Matrix
    m_oBarSize->resize(m_nbModality);                   // Bar Size Matrix

    OV_ERROR_UNLESS_KRF(m_nbPredictionsMin > 0, "Number of predictions to integrate  : " << m_nbPredictionsMin << " (expected value > 1)\n",
                        Kernel::ErrorType::BadSetting);

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_window = new CGrazMultiVisuWindow(this, visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());

    
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_window->visualItem());
    } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }

    OV_ERROR_UNLESS_KRF(initImages(paths), "Error: couldn't load resource files!\n", Kernel::ErrorType::BadProcessing);
    paths.clear();


    //***** TCP Tagging *****
    m_stimulusSender = nullptr;
    m_stimuliQueue.clear();
    m_stimulusSender = TCPTagging::CreateStimulusSender();

    if (!m_stimulusSender->connect("localhost", "15361")) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
    }
    //this->getLogManager() << Kernel::LogLevel_Warning << infos() << "\n";
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CGrazMultiVisualization::uninitialize()
{
    //***** Print Confusion Matrix *****
    this->getLogManager() << Kernel::LogLevel_Info << "Confusion Matrix : \n";
    const double* buffer = m_oConfusion->getBuffer();
    std::stringstream ss;
    ss << std::setfill('0');
    size_t idx         = 0;
    size_t predictions = 0, good = 0;

    for (size_t i = 0; i < m_nbModality; ++i) {
        for (size_t j = 0; j < m_nbModality; ++j) {
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
    m_iStim      = nullptr;
    m_iMatrix    = nullptr;
    m_oBarSize   = nullptr;
    m_oConfusion = nullptr;
    m_stimDecoder.uninitialize();
    m_classifDecoder.uninitialize();
    m_barSizeEncoder.uninitialize();
    m_confusionEncoder.uninitialize();

    //***** Window *****
    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }

    delete m_window;
    m_window = nullptr;

    m_stimlist.clear();
    delete m_stimulusSender;
    m_amplitudes.clear();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CGrazMultiVisualization::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CGrazMultiVisualization::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    bool needRedraw = false;
    //**** Stimulations *****
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        needRedraw = true;
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isBufferReceived()) { for (size_t s = 0; s < m_iStim->size(); ++s) { setStimulation(m_iStim->getId(s)); } }
    }

    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        const uint64_t start = boxContext.getInputChunkStartTime(1, i),     // Time Code Chunk Start
                       end   = boxContext.getInputChunkEndTime(1, i);       // Time Code Chunk End
        needRedraw         = true;
        m_classifDecoder.decode(i);
        if (m_classifDecoder.isHeaderReceived()) {
            if (m_iMatrix->getBufferElementCount() != m_nbModality) {
                this->getLogManager() << Kernel::LogLevel_Error << "Error, the vector/matrix do not contain the same number of values as modalities! ("
                        << m_iMatrix->getBufferElementCount() << " VS " << m_nbModality << ")\n";
                return false;
            }
            m_confusionEncoder.encodeHeader();
            m_barSizeEncoder.encodeHeader();
        }
        if (m_classifDecoder.isBufferReceived()) {
            setMatrixBuffer(m_iMatrix->getBuffer());
            m_confusionEncoder.encodeBuffer();
            m_barSizeEncoder.encodeBuffer();
        }
        if (m_classifDecoder.isEndReceived()) {
            m_confusionEncoder.encodeEnd();
            m_barSizeEncoder.encodeEnd();
        }
        boxContext.markOutputAsReadyToSend(0, start, end);
        boxContext.markOutputAsReadyToSend(1, start, end);
    }

    if(needRedraw) {
        m_window->launchQmlRedraw();
    }

    return true;
}
//---------------------------------------------------------------------------------------------------

//********************      
//***** COMPUTES *****  
//********************
//---------------------------------------------------------------------------------------------------
void CGrazMultiVisualization::aggregatePredictions(const bool all)
{
    if (m_amplitudes.size() >= m_nbPredictionsMin) {
        m_vote = 0;
        // step backwards with rev iter to take the mean of the latest samples
        uint64_t count = 0;
        double sum     = 0;
        std::vector<double> amp(m_nbModality, 0);
        //double maxA = -DBL_MAX;
        for (auto a = m_amplitudes.rbegin(); a != m_amplitudes.rend() && (all || count < m_nbPredictionsMin); ++a, ++count) {
            for (size_t i = 0; i < a->size(); ++i) { amp[i] += a->at(i); }
        }
        for (const auto& a : amp) { sum += a; }
        // Computes bar scale for each modality as a probability (other possibility substract the min/max value or the mean of the values)
        // Then we put this probability between 0-1 to 0-1 but when 0 is equal to 1/m_nbModality 
        const double minV    = 1.0 / double(m_nbModality),
                     factorV = 1.0 / (1 - minV);
        for (size_t i = 0; i < amp.size(); ++i) {
            m_barScales[i] = (amp[i] / sum - minV) * factorV;
            if (amp[m_vote] < amp[i]) { m_vote = int(i); }
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
void CGrazMultiVisualization::updateConfusionMatrix() const
{
    // Col = Expected modality, Row = Computed modality
    if (m_modality != -1 && m_vote != -1) { (m_oConfusion->getBuffer())[m_vote * m_nbModality + m_modality]++; }
    m_oBarSize->resetBuffer();
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
void CGrazMultiVisualization::setMatrixBuffer(const double* buffer)
{
    if (m_state != EStates::Feedback) { return; }                   // No continuous feedback
    // Ad-hoc forcing to probability (range [0,1], sum to 1). This will make scaling easier 
    // if run forever in a continuous mode. If the input is already scaled this way, no effect.
    double sum = 0;
    std::vector<double> values;
    values.reserve(m_nbModality);
    for (size_t i = 0; i < m_nbModality; ++i) {
        const double v = std::abs(buffer[i]);
        values.emplace_back(v);
        sum += v;
    }

    if (sum != 0.0) { for (auto& v : values) { v /= sum; } }
    else { for (auto& v : values) { v = 1.0 / double(m_nbModality); } }

    m_amplitudes.emplace_back(values);      // Add this buffer to the list
    if (m_feedbackMode != 3 && !m_delayFeedback) {
        aggregatePredictions(false);

        if(m_window) { 
            m_window->launchQmlRedraw();
        }
    }
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
void CGrazMultiVisualization::setStimulation(const uint64_t stimulation)
{
    bool needRedraw = true;
    switch (stimulation) {
        // Classical stimulations
        case OVTK_GDF_End_Of_Session:
            m_state = EStates::Black;                                   // Idle State Black Screen
            break;

        case OVTK_GDF_End_Of_Trial:
            m_state = EStates::Black;                                   // Idle State Black Screen
            aggregatePredictions(true);
            updateConfusionMatrix();
            break;

        case OVTK_GDF_Start_Of_Trial:
        case OVTK_GDF_Cross_On_Screen:
            m_state = EStates::Cross;                                   // Draw Reference State and cross
            break;

        case OVTK_GDF_Feedback_Continuous:
            m_state = EStates::Feedback;                                // Draw Feedback State
            break;

        default:                                                    // Modalities stimulations
            m_modality = -1;                                        // Initialize modality number
            m_vote = -1;                                            // Initialize vote
            m_amplitudes.clear();                                   // Clear the previous amplitudes
            for (auto& s : m_barScales) { s = 0; }                  // Reinit Bar Scales
            for (size_t i = 0; i < m_stimlist.size(); ++i) {
                if (m_stimlist[i] == stimulation) {
                    m_modality = int(i);
                    break;                                          // stop the loop (but not so usefull big number of modality is not so big generally)
                }
            }
            if (m_modality != -1) { m_state = EStates::Instruction; }   // If recognize stimulation Draw Modality State
            else { needRedraw = false; }                            // If not, we don't care about stimulations and we don't want to redraw
            break;
    }
    // Queue the stimulation to be sent to TCP Tagging
    m_stimuliQueue.push_back(stimulation);
    m_window->stateChanged();

    // Indicates that the window is invalidate (must be redrawn)
    if (needRedraw && m_window) {
        m_window->launchQmlRedraw();
    }
}
//---------------------------------------------------------------------------------------------------

//**************************    
//***** CALLBACKS HACK *****    
//**************************
//---------------------------------------------------------------------------------------------------
void CGrazMultiVisualization::FlushQueue()
{
    for (const auto i : m_stimuliQueue) { m_stimulusSender->sendStimulation(i); }
    m_stimuliQueue.clear();
}
//---------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CGrazMultiVisualization::initImages(const std::vector<std::string>& paths)
{
    const size_t s = paths.size();
    if (s != m_nbModality + 1) { return false; }
    if(m_window) {
        m_window->setImgsPaths(paths);
        return true;
    }

    return false;
}
//---------------------------------------------------------------------------------------------------

//****************  
//***** MISC *****  
//****************
//---------------------------------------------------------------------------------------------------
std::string CGrazMultiVisualization::infos() const
{
    std::stringstream ss;
    ss << "\n";
    ss << "Show instruction : " << (m_showInstruction ? "yes" : "no") << ", Feedback Mode : ";
    if (m_feedbackMode == 0) { ss << "Positive Only"; }
    else if (m_feedbackMode == 1) { ss << "Best Only"; }
    else if (m_feedbackMode == 2) { ss << "All"; }
    else if (m_feedbackMode == 3) { ss << "None"; }
    ss << ", Delay : " << (m_delayFeedback ? "yes" : "no") << "\n";
    ss << "Show Accuracy : " << (m_showAccuracy ? "yes" : "no") << ", Nb Predictions : " << m_nbPredictionsMin << ", Nb Modality : " << m_nbModality << "\n";
    
    ss << "Number of Images : " << m_window->imgsPaths().length() << "\n";
    ss << "Images Paths : \n";
    for (const auto& path : m_window->imgsPaths()) { ss << "\t " << path.toStdString() << " \n"; }

    ss << "Stimulation List : ";
    for (const auto& stim : m_stimlist) { ss << stim << " "; }
    ss << "\n";
    return ss.str();
}
//---------------------------------------------------------------------------------------------------
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
