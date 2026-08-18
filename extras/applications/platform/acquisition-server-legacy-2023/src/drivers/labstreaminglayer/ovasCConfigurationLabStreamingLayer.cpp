#if defined(TARGET_HAS_ThirdPartyLSL)

#include "ovasCConfigurationLabStreamingLayer.h"

#include <lsl_cpp.h>
#include "ovasIHeader.h"

#include <iostream>
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {

CConfigurationLabStreamingLayer::CConfigurationLabStreamingLayer(IDriverContext& ctx, const char* gtkBuilderFilename, IHeader& header, bool& limitSpeed,
                                                                 CString& signalStream, CString& signalStreamID, CString& markerStream, CString& markerStreamID,
                                                                 uint32_t& fallbackSampling)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_limitSpeed(limitSpeed), m_signalStream(signalStream),
      m_signalStreamID(signalStreamID), m_markerStream(markerStream), m_markerStreamID(markerStreamID), m_fallbackSampling(fallbackSampling) {}

bool CConfigurationLabStreamingLayer::preConfigure()
{
    if (! CConfigurationBuilder::preConfigure()) { return false; }

    GtkToggleButton* buttonSpeedLimit = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_limit_speed"));
    gtk_toggle_button_set_active(buttonSpeedLimit, m_limitSpeed ? TRUE : FALSE);

    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_signal_stream"));
    if (!comboBox) { return false; }

    GtkComboBox* markerComboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_marker_stream"));
    if (!markerComboBox) { return false; }

    GtkEntry* fallbackSamplingEntry = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_fallback_sampling_frequency"));
    if (!fallbackSamplingEntry) { return false; }

    m_signalIdxs.clear();
    m_markerIdxs.clear();

    // Allow operation without a marker stream
    gtk_combo_box_append_text(markerComboBox, "None");
    gtk_combo_box_set_active(markerComboBox, 0);
    m_markerIdxs.push_back(-1);

    m_streams = lsl::resolve_streams(1.0);

    // See if any of the streams can be interpreted as signal or marker
    uint32_t nStreams       = 0;
    uint32_t nMarkerStreams = 0;
    bool exactSignalMatch   = false; // If we can match both name and ID, stop at that. Otherwise we auto-accept the 'name only' match.
    bool exactMarkerMatch   = false;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Discovered " << m_streams.size() << " streams in total\n";

    for (size_t i = 0; i < m_streams.size(); ++i) {
        if (m_streams[i].channel_format() == lsl::cf_float32) {
            std::stringstream ss;
            ss << m_streams[i].name() << " / " << m_streams[i].source_id();

            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << i << ". Discovered signal stream " << m_streams[i].name() << ", id " << ss.str() << "\n";

            gtk_combo_box_append_text(comboBox, ss.str().c_str());
            m_signalIdxs.push_back(static_cast<int>(i));
            if ((m_signalStream == CString(m_streams[i].name().c_str()) && !exactSignalMatch) || !nStreams) {
                if (m_signalStreamID == CString(m_streams[i].source_id().c_str())) { exactSignalMatch = true; }
                gtk_combo_box_set_active(comboBox, nStreams);
            }
            nStreams++;
        }
        else if (m_streams[i].channel_format() == lsl::cf_int32) {
            std::stringstream ss;
            ss << m_streams[i].name().c_str() << " / " << m_streams[i].source_id().c_str();

            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << i << ". Discovered marker stream " << m_streams[i].name() << ", id " << ss.str() << "\n";

            gtk_combo_box_append_text(markerComboBox, ss.str().c_str());
            m_markerIdxs.push_back(i);
            if ((m_markerStream == CString(m_streams[i].name().c_str()) && !exactMarkerMatch) || !nMarkerStreams) {
                if (m_markerStreamID == CString(m_streams[i].source_id().c_str())) {
                    // If we can match both name and ID, stop at that. Otherwise we accept the 'name only' match.
                    exactMarkerMatch = true;
                }
                gtk_combo_box_set_active(markerComboBox, nMarkerStreams + 1);
            }
            nMarkerStreams++;
        }
        else {
            // Only float and int are currently supported for signals and markers respectively
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << i << ". Discovered stream with channel format " << m_streams[i].channel_format()
                    << " of stream [" << m_streams[i].name() << "] which is not supported, skipped.\n";
        }
    }

    if (m_fallbackSampling == 0) { gtk_entry_set_text(fallbackSamplingEntry, ""); }
    else {
        const std::string buffer = std::to_string(m_fallbackSampling);
        gtk_entry_set_text(fallbackSamplingEntry, buffer.c_str());
    }

    return true;
}

bool CConfigurationLabStreamingLayer::postConfigure()
{
    if (m_applyConfig) {
        GtkToggleButton* buttonSpeedLimit = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "checkbutton_limit_speed"));
        m_limitSpeed                      = gtk_toggle_button_get_active(buttonSpeedLimit) ? true : false;

        // Retrieve signal stream info
        GtkComboBox* comboBoxSignal = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_signal_stream"));
        if (!comboBoxSignal) {
            m_applyConfig = false;
            CConfigurationBuilder::postConfigure(); // close window etc
            return false;
        }

        if (m_signalIdxs.empty()) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "LSL: Cannot proceed without a signal stream.\n";
            m_applyConfig = false;
            CConfigurationBuilder::postConfigure(); // close window etc
            return false;
        }

        const int signalIdx = m_signalIdxs[gtk_combo_box_get_active(comboBoxSignal)];

        m_signalStream   = m_streams[signalIdx].name().c_str();
        m_signalStreamID = m_streams[signalIdx].source_id().c_str();

        GtkComboBox* comboBoxMarker = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_marker_stream"));
        if (!comboBoxMarker) {
            m_applyConfig = false;
            CConfigurationBuilder::postConfigure(); // close window etc
            return false;
        }

        // Retrieve marker stream info
        const int markerIdx = m_markerIdxs[gtk_combo_box_get_active(comboBoxMarker)];
        if (markerIdx >= 0) {
            m_markerStream   = m_streams[markerIdx].name().c_str();
            m_markerStreamID = m_streams[markerIdx].source_id().c_str();
        }
        else {
            m_markerStream   = "None";
            m_markerStreamID = "";
        }

        // Retrieve fallback sampling rate
        GtkEntry* entry   = GTK_ENTRY(gtk_builder_get_object(m_builder, "entry_fallback_sampling_frequency"));
        uint32_t sampling = 0;
        if (sscanf(gtk_entry_get_text(entry), "%u", &sampling) == 1) { m_fallbackSampling = uint32_t(sampling); }
        else { m_fallbackSampling = 0; }

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Binding to [" << m_signalStream << ", id " << m_signalStreamID << "] and ["
                << m_markerStream << ", id " << m_markerStreamID << "]\n";
    }

    // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
    if (!CConfigurationBuilder::postConfigure()) { return false; }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif  // TARGET_HAS_ThirdPartyLSL
