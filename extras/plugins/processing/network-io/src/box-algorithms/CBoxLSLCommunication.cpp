///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxLSLCommunication.cpp
/// \brief Class of the generic box that communicates with LSL.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 24/02/2021
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxLSLCommunication.hpp"

#include <lsl/Utils.hpp>
#include <ctime>
#include <iostream>
#include <unordered_set>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {

//--------------------------------------------------------------------------------
bool CBoxLSLCommunication::initialize()
{
    const Kernel::IBox& boxCtx = getStaticBoxContext();
    m_nInput                   = boxCtx.getInputCount();
    m_nOutput                  = boxCtx.getOutputCount();

    //---------- Check Input/Output ----------
    m_firstStimInput  = -1;
    m_firstStimOutput = -1;
    CIdentifier type;
    for (size_t i = 0; i < boxCtx.getInputCount(); ++i) {
        boxCtx.getInputType(i, type);
        if (m_firstStimInput == -1 && type == OV_TypeId_Stimulations) { m_firstStimInput = i; }
        OV_ERROR_UNLESS_KRF(!(m_firstStimInput != -1 && type != OV_TypeId_Stimulations),
                            "You must put All stimulations streams at the end of the Input list.", Kernel::ErrorType::BadInput);
    }
    for (size_t i = 0; i < boxCtx.getOutputCount(); ++i) {
        boxCtx.getOutputType(i, type);
        if (m_firstStimOutput == -1 && type == OV_TypeId_Stimulations) { m_firstStimOutput = i; }
        OV_ERROR_UNLESS_KRF(!(m_firstStimOutput != -1 && type != OV_TypeId_Stimulations),
                            "You must put All stimulations streams at the end of the Output list.", Kernel::ErrorType::BadOutput);
    }

    //---------- Initialize Input/Output ----------
    m_decoders.resize(m_firstStimInput == -1 ? m_nInput : m_firstStimInput);
    m_stimDecoders.resize(m_nInput - m_decoders.size());
    m_encoders.resize(m_firstStimOutput == -1 ? m_nOutput : m_firstStimOutput);
    m_stimEncoders.resize(m_nOutput - m_encoders.size());
    for (size_t i = 0; i < m_nInput; ++i) {
        boxCtx.getInputType(i, type);
        if (type == OV_TypeId_Stimulations) { m_stimDecoders[i - m_firstStimInput].initialize(*this, i); }
        else {
            if (type == OV_TypeId_Signal) { m_decoders[i] = new Toolkit::TSignalDecoder<CBoxLSLCommunication>(*this, i); }
            else if (type == OV_TypeId_StreamedMatrix) { m_decoders[i] = new Toolkit::TStreamedMatrixDecoder<CBoxLSLCommunication>(*this, i); }
            else if (type == OV_TypeId_Spectrum) { m_decoders[i] = new Toolkit::TSpectrumDecoder<CBoxLSLCommunication>(*this, i); }
            else if (type == OV_TypeId_FeatureVector) { m_decoders[i] = new Toolkit::TFeatureVectorDecoder<CBoxLSLCommunication>(*this, i); }
        }
    }

    for (size_t i = 0; i < m_nOutput; ++i) {
        boxCtx.getOutputType(i, type);
        if (type == OV_TypeId_Stimulations) { m_stimEncoders[i - m_firstStimOutput].initialize(*this, i); }
        else if (type == OV_TypeId_StreamedMatrix) { m_encoders[i].initialize(*this, i); }
    }

    //---------- Initialize Pointers ----------
    m_iMatrix.reserve(m_decoders.size());
    m_oMatrix.reserve(m_encoders.size());
    m_iStimSet.reserve(m_stimDecoders.size());
    m_oStimSet.reserve(m_stimEncoders.size());
    for (auto& decoder : m_decoders) { m_iMatrix.push_back(decoder.getOutputMatrix()); }
    for (auto& encoder : m_encoders) { m_oMatrix.push_back(encoder.getInputMatrix()); }
    for (auto& decoder : m_stimDecoders) { m_iStimSet.push_back(decoder.getOutputStimulationSet()); }
    for (auto& encoder : m_stimEncoders) { m_oStimSet.push_back(encoder.getInputStimulationSet()); }


    //---------- Initialize LSL Stream (OV to Unity) ----------
    m_outlets.resize(m_nInput);
    m_inlets.resize(m_nOutput);
    m_buffers.resize(m_nOutput);

    //---------- Names, Ids and times ----------
    m_names.reserve(boxCtx.getSettingCount());
    std::unordered_set<std::string> tmp;    // Force Unique elements but elements are pushed in random place
    for (size_t i = 0; i < boxCtx.getSettingCount(); ++i) {
        const std::string name = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i)).toASCIIString();
        const auto good        = tmp.insert(name);
        if (good.second) { m_names.push_back(name); }
        else { OV_ERROR_KRF("the stream name [" << name << "] is used by more than one of this box's settings, while each stream needs a name of its own. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting); }
    }

    tmp.clear();
    while (tmp.size() < m_nInput) { tmp.insert(CIdentifier::random().str()); }
    m_ids.assign(tmp.begin(), tmp.end());               // Only for Emissions streams

    m_lastOutputTimes = std::vector<uint64_t>(m_nOutput, 0);    // Only for received streams and OV continuous times

    //---------- Logs ----------
    std::stringstream msg;
    msg << "LSL Communication box with "
            << m_nInput << " inputs (" << (m_firstStimInput == -1 ? 0 : m_nInput - m_firstStimInput) << " stimulations) and "
            << m_nOutput << " outputs (" << (m_firstStimOutput == -1 ? 0 : m_nOutput - m_firstStimOutput) << " stimulations).\n";
    for (size_t i = 0; i < m_names.size(); ++i) {
        if (i < m_nInput) { msg << "\tStream for input " << (i + 1) << " : " << m_names[i] << " (id : " << m_ids[i] << ")\n"; }
        else { msg << "\tStream for output " << (i + 1 - m_nInput) << " : " << m_names[i] << "\n"; }
    }
    this->getLogManager() << Kernel::LogLevel_Trace << msg.str();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLCommunication::uninitialize()
{
    //---------- Uninitialize Input/Output ----------
    for (auto& decoder : m_decoders) { decoder.uninitialize(); }
    for (auto& encoder : m_encoders) { encoder.uninitialize(); }
    for (auto& decoder : m_stimDecoders) { decoder.uninitialize(); }
    for (auto& encoder : m_stimEncoders) { encoder.uninitialize(); }

    //---------- Delete LSL Stream Inlet/Outlet ----------
    for (auto& outlet : m_outlets) { delete outlet; }
    for (auto& inlet : m_inlets) { delete inlet; }

    //---------- Clear Vector ----------
    m_decoders.clear();
    m_encoders.clear();
    m_stimDecoders.clear();
    m_stimEncoders.clear();

    m_iMatrix.clear();
    m_oMatrix.clear();
    m_iStimSet.clear();
    m_oStimSet.clear();

    m_outlets.clear();
    m_inlets.clear();

    m_names.clear();
    m_ids.clear();
    m_lastOutputTimes.clear();
    m_buffers.clear();

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLCommunication::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (m_nOutput != 0) { getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLCommunication::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxLSLCommunication::process()
{
    Kernel::IBoxIO& boxCtx     = this->getDynamicBoxContext();
    const uint64_t currentTime = getPlayerContext().getCurrentTime();

    //---------- Emission Part ----------
    for (size_t i = 0; i < m_nInput; ++i) {
        const size_t idx = i < m_firstStimInput ? i : i - m_firstStimInput;
        for (size_t c = 0; c < boxCtx.getInputChunkCount(i); ++c) {
            if (i < m_firstStimInput) {
                m_decoders[idx].decode(c);
                if (m_decoders[idx].isHeaderReceived() && !m_outlets[i]) {
                    const size_t nChannel = m_iMatrix[idx]->getDimensionSize(0);
                    CIdentifier type;
                    getStaticBoxContext().getInputType(idx, type);
                    size_t frequency = size_t(lsl::IRREGULAR_RATE);
                    if (type == OV_TypeId_Signal) { frequency = m_decoders[idx].getOutputSamplingRate(); }

                    lsl::stream_info signalInfo = LSL::createSignalStreamInfo(m_names[i], m_ids[i], m_iMatrix[idx], frequency);
                    // make a new outlet
                    try { m_outlets[i] = new lsl::stream_outlet(signalInfo, int(nChannel)); }
                    catch (...) {
                        this->getLogManager() << Kernel::LogLevel_Error << "Unable to create signal outlet\n";
                        return false;
                    }
                    this->getLogManager() << Kernel::LogLevel_Trace << "Created stream [" << m_names[i] << "]";
                }
                if (m_decoders[idx].isBufferReceived() && m_outlets[i]) {
                    LSL::sendSignal(m_outlets[i], m_iMatrix[idx], boxCtx.getInputChunkStartTime(i, c), boxCtx.getInputChunkEndTime(i, c));
                }
                if (m_decoders[idx].isEndReceived()) {}
            }
            else {
                m_stimDecoders[idx].decode(c);
                if (m_stimDecoders[idx].isHeaderReceived() && !m_outlets[i]) {
                    // Open a stimulus stream
                    lsl::stream_info info = LSL::createStimulationStreamInfo(m_names[i], m_ids[i]);

                    try { m_outlets[i] = new lsl::stream_outlet(info); }
                    catch (...) {
                        this->getLogManager() << Kernel::LogLevel_Error << "Unable to create marker outlet\n";
                        return false;
                    }
                }
                if (m_stimDecoders[idx].isBufferReceived() && m_outlets[i]) { LSL::sendStimulation(m_outlets[i], m_iStimSet[idx]); }
                if (m_stimDecoders[idx].isEndReceived()) { }
            }
        }
    }

    //---------- Reception Part ----------
    for (size_t i = 0; i < m_nOutput; ++i) {
        if (!m_inlets[i]) {
            const std::string& name     = m_names[i + m_nInput];
            const lsl::stream_info info = LSL::findStreamInfo(name, "", 0); // timeout of 0 to avoid blocking OpenViBE
            if (info.name() == name) {
                m_inlets[i] = new lsl::stream_inlet(info);
                try { m_inlets[i]->open_stream(); }
                catch (...) {
                    this->getLogManager() << Kernel::LogLevel_Error << "Failed to open stream with name [" << name << "]\n";
                    return false;
                }
                this->getLogManager() << Kernel::LogLevel_Trace << "We have open stream [" << name << "]\n";

                if (i < m_firstStimOutput) {                    // Header for matrix streams encoded
                    // Resize matrix. Only 1D Matrix can be received with LSL by default. For more dimensions, specific code must written.
                    // You must stack samples and when you have enough fill your 2D matrix.
                    // But you must know before the size of the second dimension or have a square matrix.
                    const size_t nChannel = info.channel_count();
                    m_buffers[i].resize(nChannel);
                    m_oMatrix[i]->resize(nChannel, 1);
                    m_encoders[i].encodeHeader();
                }
                else {                                          // Header for stimulation streams encoded
                    m_buffers[i].resize(1);
                    m_oStimSet[i - m_firstStimOutput]->clear(); // reset stimulation output
                    m_stimEncoders[i - m_firstStimOutput].encodeHeader();
                }
                boxCtx.markOutputAsReadyToSend(i, m_lastOutputTimes[i], m_lastOutputTimes[i]);  // Makes the output available
            }
        }
        else {
            double time;
            try { time = m_inlets[i]->pull_sample(m_buffers[i], 0.0); } // Timeout to 0.0 to avoid lag (OpenViBE can't have background task)
            catch (...) {
                this->getLogManager() << Kernel::LogLevel_Error << "Failed to get sample.\n";
                return false;
            }
            if (std::abs(time) > 0.0) {
                if (i < m_firstStimOutput) {                    // Buffer for matrix streams encoded
                    m_oMatrix[i]->setBuffer(m_buffers[i]);
                    m_encoders[i].encodeBuffer();
                }
                else {                                          // Buffer for stimulation streams encoded
                    m_oStimSet[i - m_firstStimOutput]->clear(); // Reset stimulation output
                    m_oStimSet[i - m_firstStimOutput]->push_back(uint64_t(m_buffers[i][0]), currentTime, 0);
                    m_stimEncoders[i - m_firstStimOutput].encodeBuffer();
                }
                boxCtx.markOutputAsReadyToSend(i, m_lastOutputTimes[i], currentTime);   // Makes the output available
                m_lastOutputTimes[i] = currentTime;
            }
        }
    }
    return true;
}
//--------------------------------------------------------------------------------

}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
