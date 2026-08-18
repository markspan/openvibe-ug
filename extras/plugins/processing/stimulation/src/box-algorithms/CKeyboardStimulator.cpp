///-------------------------------------------------------------------------------------------------
/// 
/// \file CKeyboardStimulator.cpp
/// \author Bruno Renier (INRIA/IRISA).
/// \version 1.1.
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

#include "CKeyboardStimulator.hpp"

#include <fstream>
#include <string>
#include <sstream>

#include <tcptagging/IStimulusSender.h>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

//! Callback for the close window button
//static void keyboard_stimulator_gtk_widget_do_nothing(GtkWidget* widget) { }

// Called when a key is pressed on the keyboard
static gboolean KeyPressCB(GtkWidget* /*widget*/, const GdkEventKey* key, gpointer data)
{
    reinterpret_cast<CKeyboardStimulator*>(data)->ProcessKey(key->keyval, true);
    return true;
}

// Called when a key is released on the keyboard
static gboolean KeyReleaseCB(GtkWidget* /*widget*/, const GdkEventKey* key, gpointer data)
{
    reinterpret_cast<CKeyboardStimulator*>(data)->ProcessKey(key->keyval, false);
    return true;
}

/**
 * Called when a key has been pressed.
 * \param key The gdk value to the pressed key.
 * \param state state of the pressed key
 * */
void CKeyboardStimulator::ProcessKey(const guint key, const bool state)
{
    //if there is one entry, adds the stimulation to the list of stims to be sent
    if (m_keyToStimulation.count(key) != 0 && state != m_keyToStimulation[key].status) {
        if (state) {
            // getLogManager() << Kernel::LogLevel_Trace << "Pressed key code " << (size_t)key << "\n";
            m_stimulusSender->sendStimulation(m_keyToStimulation[key].press);
            m_stimulationToSend.push_back(m_keyToStimulation[key].press);
        }
        else {
            // getLogManager() << Kernel::LogLevel_Trace << "Released key code " << (size_t)key << "\n";
            if (m_sendKeyReleaseStimCode) {
                m_stimulusSender->sendStimulation(m_keyToStimulation[key].release);
                m_stimulationToSend.push_back(m_keyToStimulation[key].release);
            }
        }
        m_keyToStimulation[key].status = state;
    }
    else {
        // this->getLogManager() << Kernel::LogLevel_Warning << "Unhandled key code " << (size_t)key << "\n";
        m_unknownKeyPressed = true;
        m_unknownKeyCode    = size_t(key);
    }
}

/**
 * Parse the configuration file and creates the Key/Stimulation associations.
 * \param filename The name of the configuration file.
 * \return True if the file was correctly parsed.
 * */
bool CKeyboardStimulator::ParseConfigurationFile(const char* filename)
{
    std::ifstream file;
    file.open(filename);

    if (!file) { return false; }

    std::string keyName, meaning, stimPress, stimRelease;
    std::string line;
    bool meaningFound = false;
    //reads all the couples key name/stim
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        ss >> keyName >> stimPress >> stimRelease;

        std::getline(ss, meaning);
        if (meaning.size() >= 3) {
        meaning = meaning.substr(2, meaning.size() - 3);
                meaningFound = true;
    }

        key_t key;
        key.meaning = meaning;
        key.press   = std::stoul(stimPress, nullptr, 16);
        key.release = std::stoul(stimRelease, nullptr, 16);
        key.status  = false;

        m_keyToStimulation[gdk_keyval_from_name(keyName.c_str())] = key;
    }

    if (m_displayMeaning && !meaningFound) {
        getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning << "No meaning found for any key in configuration file!\n";
        m_displayMeaning = false;
    }

    file.close();

    return true;
}

bool CKeyboardStimulator::initialize()
{
    // Parses box settings to find input file's name
    const CString fileName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_displayMeaning = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_displayStimulationCodes = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_sendKeyReleaseStimCode = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    //CString tcpTaggingHostAddress = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    //CString tcpTaggingHostPort = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    const CString tcpTaggingHostAddress("localhost");
    const CString tcpTaggingHostPort("15361");

    if (!ParseConfigurationFile(fileName.toASCIIString())) {
        getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning << "Problem while parsing configuration file!\n";
        m_error = true;
        return false;
    }

    m_encoder.initialize(*this, 0);

    //const std::string red("#602020");
    const std::string green("#206020");
    const std::string blue("#202060");

    std::stringstream ss;
    ss << "\nUse your keyboard to send stimulations\nAvailable keys are :\n\n";
    for (auto i = m_keyToStimulation.begin(); i != m_keyToStimulation.end(); ++i) {
        ss << "<span size=\"large\">\t";
        ss << "<span style=\"italic\" foreground=\"" << green << "\">" << gdk_keyval_name(i->first) << "</span>";
        ss << "\t";
        if (m_displayMeaning) {
            ss << "<span style=\"italic\" foreground=\"" << blue << "\">"
                    << i->second.meaning << "</span>";
            ss << "\t";
        }
        if (m_displayStimulationCodes) {
            ss << "Pressed : <span style=\"italic\" foreground=\"" << blue << "\">"
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, i->second.press) << "</span>";
            ss << "\t";
            ss << "Released : <span style=\"italic\" foreground=\"" << blue << "\">"
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, i->second.release) << "</span>";
        }
            ss << "\t</span>\n";
    }

    GtkBuilder* builder =
            gtk_builder_new(); // glade_xml_new(Directories::getDataDir() + "/plugins/stimulation/keyboard-stimulator.ui", nullptr, nullptr);
    gtk_builder_add_from_file(builder, Directories::getDataDir() + "/plugins/stimulation/keyboard-stimulator.ui", nullptr);
    gtk_builder_connect_signals(builder, nullptr);

    m_widget = GTK_WIDGET(gtk_builder_get_object(builder, "keyboard_stimulator-eventbox"));

    gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(builder, "keyboard_stimulator-label")), ss.str().c_str());

    g_signal_connect(m_widget, "key-press-event", G_CALLBACK(KeyPressCB), this);
    g_signal_connect(m_widget, "key-release-event", G_CALLBACK(KeyReleaseCB), this);
    g_object_unref(builder);

    m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    m_visualizationCtx->setWidget(*this, m_widget);

    //TCP TAGGING
    m_stimulusSender = TCPTagging::CreateStimulusSender();
    if (!m_stimulusSender->connect(tcpTaggingHostAddress, tcpTaggingHostPort) && tcpTaggingHostAddress.toASCIIString()[0] != 0) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unable to connect to AS's TCP Tagging plugin, stimuli wont be forwarded.\n";
    }

    return true;
}

bool CKeyboardStimulator::uninitialize()
{
    if (m_stimulusSender) {
        delete m_stimulusSender;
        m_stimulusSender = nullptr;
    }

    m_encoder.uninitialize();

    if (m_widget) {
        g_object_unref(m_widget);
        m_widget = nullptr;
    }

    if (m_visualizationCtx) {
        this->releasePluginObject(m_visualizationCtx);
        m_visualizationCtx = nullptr;
    }

    return true;
}

bool CKeyboardStimulator::processClock(Kernel::CMessageClock& msg)
{
    if (m_error) { return false; }

    if (m_unknownKeyPressed) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Unhandled key code " << m_unknownKeyCode << "\n";
        m_unknownKeyPressed = false;
    }

    const uint64_t currentTime = msg.getTime();

    if (currentTime == 0) {
        m_encoder.encodeHeader();
        getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
    }

    if (currentTime != m_previousActivationTime) {
        Kernel::IBoxIO* boxIO = getBoxAlgorithmContext()->getDynamicBoxContext();

        const CStimulationSet* stimulationSet = m_encoder.getInputStimulationSet();
        stimulationSet->clear();        // The encoder may retain the buffer from the previous round, clear it

        for (size_t i = 0; i < m_stimulationToSend.size(); ++i) { stimulationSet->push_back(m_stimulationToSend[i], currentTime, 0); }
        m_stimulationToSend.clear();

        m_encoder.encodeBuffer();

        boxIO->markOutputAsReadyToSend(0, m_previousActivationTime, currentTime);
        getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    }

    m_previousActivationTime = currentTime;

    return true;
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
#endif
