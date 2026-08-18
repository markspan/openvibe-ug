///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxLSLExport.cpp
/// \brief Class of the box that export stream with LSL.
/// \author Jussi T. Lindgren (Inria).
/// \version 1.0.
/// \date 30/01/2015
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

#ifdef TARGET_HAS_ThirdPartyLSL

// Notes: This code should be kept compatible with changes to LSL Input Driver and Output Plugin in OpenViBE Acquisition Server.
#include "CBoxLSLExport.hpp"

#include <ctime>
#include <iostream>

#include <lsl/Utils.hpp>
#include <system/ovCTime.h>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

//--------------------------------------------------------------------------------
bool CBoxLSLExport::initialize()
{
    m_signalOutlet   = nullptr;
    m_stimulusOutlet = nullptr;

    m_signalDecoder.initialize(*this, 0);
    m_stimDecoder.initialize(*this, 1);

    m_signalName = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)).toASCIIString();
    m_markerName = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)).toASCIIString();

    // These are supposed to be unique, so we don't have them in the box config
    m_signalID = CIdentifier::random().str();
    m_markerID = CIdentifier::random().str();

    while (m_markerID == m_signalID) { m_markerID = CIdentifier::random().str(); } // very unlikely

    this->getLogManager() << Kernel::LogLevel_Trace << "Will create streams [" << m_signalName << ", id " << m_signalID
            << "] and [" << m_markerName << ", id " << m_markerID << "]\n";

    m_useOVTimestamps = this->getConfigurationManager().expandAsBoolean("${LSL_UseOVTimestamps}", m_useOVTimestamps);
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLExport::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_stimDecoder.uninitialize();

    if (m_signalOutlet) {
        delete m_signalOutlet;
        m_signalOutlet = nullptr;
    }
    if (m_stimulusOutlet) {
        delete m_stimulusOutlet;
        m_stimulusOutlet = nullptr;
    }

    m_buffer.clear();

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLExport::processInput(const size_t /*index*/)
{
    // ready to process !
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLExport::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Process signals
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        if (m_startTime == CTime(0)) {
            // This should be the ov time when acquisition client tagged the first chunk with [0, t], t=bufferSize/samplingRate.
            // As the true run time is not know here, we have to do with a slightly delayed estimate. (The delay is the duration
            // it took from stamping the first chunk to this point in the code)
            m_startTime = System::Time::zgetTime();
        }

        m_signalDecoder.decode(i);
        if (m_signalDecoder.isHeaderReceived() && !m_signalOutlet) {
            const size_t nChannel        = m_signalDecoder.getOutputMatrix()->getDimensionSize(0);
            const size_t samplesPerBlock = m_signalDecoder.getOutputMatrix()->getDimensionSize(1);
            const size_t frequency       = m_signalDecoder.getOutputSamplingRate();

            m_buffer.resize(nChannel);

            // Open a signal stream 
            lsl::stream_info signalInfo(m_signalName, "signal", int(nChannel), int(frequency), lsl::cf_float32, m_signalID);

            lsl::xml_element channels = signalInfo.desc().append_child("channels");
            //m_signalDecoder.getOutputMatrix()->getDimensionLabel(0, 1);

            for (size_t c = 0; c < nChannel; ++c) {
                const char* name = m_signalDecoder.getOutputMatrix()->getDimensionLabel(0, c);
                channels.append_child("channel").append_child_value("label", name).append_child_value("unit", "unknown").append_child_value("type", "signal");
            }

#ifdef DEBUG
            lsl::xml_element debug = signalInfo.desc().child("channels");
            if(debug.child("channel").child_value("label")) { std::cout << "channel label " << debug.child("channel").child_value("label") << "\n"; }
#endif

            // make a new outlet
            try { m_signalOutlet = new lsl::stream_outlet(signalInfo, int(samplesPerBlock)); }
            catch (...) {
                this->getLogManager() << "Unable to create signal outlet\n";
                return false;
            }
        }
        if (m_signalDecoder.isBufferReceived() && m_signalOutlet) {
            if (m_signalOutlet->have_consumers()) {
                const CMatrix* matrix        = m_signalDecoder.getOutputMatrix();
                const size_t nChannel        = matrix->getDimensionSize(0);
                const size_t samplesPerBlock = matrix->getDimensionSize(1);
                const double* iBuffer        = matrix->getBuffer();

                const CTime chunkStartTime(boxContext.getInputChunkStartTime(0, i));
                const CTime chunkEndTime(boxContext.getInputChunkEndTime(0, i));

                if (samplesPerBlock == 0) {
                    this->getLogManager() << Kernel::LogLevel_Error << "Unable to process signal with 0 samples per buffer \n";
                    return false;
                }
                // note: the step computed below should be exactly the same as could be obtained from the sampling rate

                // n.b. this would work more "lsl-like" if the timestamps were the real acquisition timestamps
                if (m_useOVTimestamps) {
                    const double sampleStepInSec =
                            (chunkEndTime - chunkStartTime).toSeconds() / static_cast<double>(samplesPerBlock);
                    const double chunkStartInSec = chunkStartTime.toSeconds();

                    for (size_t sample = 0; sample < samplesPerBlock; ++sample) {
                        for (size_t channel = 0; channel < nChannel; ++channel) {
                            m_buffer[channel] = static_cast<float>(iBuffer[channel * samplesPerBlock + sample]);
                        }
                        m_signalOutlet->push_sample(m_buffer, chunkStartInSec + sampleStepInSec * static_cast<double>(sample));
                    }
                }
                else {
                    const uint64_t sampleStep = (chunkEndTime - chunkStartTime).time() / uint64_t(samplesPerBlock);
                    for (size_t sample = 0; sample < samplesPerBlock; ++sample) {
                        for (size_t channel = 0; channel < nChannel; ++channel) {
                            m_buffer[channel] = static_cast<float>(iBuffer[channel * samplesPerBlock + sample]);
                        }
                        const double lslTime = LSL::getLSLRelativeTime(m_startTime + chunkStartTime + CTime(sample * sampleStep));
                        m_signalOutlet->push_sample(m_buffer, lslTime);
                    }
                }
            }
        }
        if (m_signalDecoder.isEndReceived()) {
            if (m_signalOutlet) {
                delete m_signalOutlet;
                m_signalOutlet = nullptr;
            }
        }
    }

    // Process stimuli -> LSL markers. 
    // Note that stimuli with identifiers not fitting to int will be mangled by a static cast.
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        if (m_startTime == CTime(0)) {
            // Initialisation done here too as the box may only receive stimulations
            m_startTime = System::Time::zgetTime();
        }

        m_stimDecoder.decode(i);
        if (m_stimDecoder.isHeaderReceived() && !m_stimulusOutlet) {
            // Open a stimulus stream
            lsl::stream_info info(m_markerName, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, m_markerID);

            info.desc().append_child("channels").append_child("channel").append_child_value("label", "Stimulations").append_child_value("type", "marker");

            try { m_stimulusOutlet = new lsl::stream_outlet(info); }
            catch (...) {
                this->getLogManager() << "Unable to create marker outlet\n";
                return false;
            }
        }
        if (m_stimDecoder.isBufferReceived() && m_stimulusOutlet) {
            // Output stimuli
            if (m_stimulusOutlet->have_consumers()) {
                const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();

                for (size_t s = 0; s < stimSet->size(); ++s) {
                    const int code    = int(stimSet->getId(s));
                    const double date = m_useOVTimestamps ? CTime(stimSet->getDate(s)).toSeconds()
                                            : LSL::getLSLRelativeTime(m_startTime + CTime(stimSet->getDate(s)));

                    m_stimulusOutlet->push_sample(&code, date);
                }
            }
        }
        if (m_stimDecoder.isEndReceived()) {
            if (m_stimulusOutlet) {
                delete m_stimulusOutlet;
                m_stimulusOutlet = nullptr;
            }
        }
    }
    return true;
}
//--------------------------------------------------------------------------------

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE

#endif
