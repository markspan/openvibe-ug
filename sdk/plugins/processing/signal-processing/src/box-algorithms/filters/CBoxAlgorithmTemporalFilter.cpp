///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmTemporalFilter.cpp
/// \brief Classes implementation for the Box Temporal Filter.
/// \author Thibaut Monseigne (Inria).
/// \version 2.0.
/// \date 14/10/2021
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmTemporalFilter.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

//--------------------------------------------------------------------------------
// Filter Definition
typedef Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandPass<32>, 1, Dsp::DirectFormII> CButterworthBandPass;
typedef Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::BandStop<32>, 1, Dsp::DirectFormII> CButterworthBandStop;
typedef Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::HighPass<32>, 1, Dsp::DirectFormII> CButterworthHighPass;
typedef Dsp::SmoothedFilterDesign<Dsp::Butterworth::Design::LowPass<32>, 1, Dsp::DirectFormII> CButterworthLowPass;

//--------------------------------------------------------------------------------
void CBoxAlgorithmTemporalFilter::setParameter(const size_t id)
{
    m_parameters[id][1] = double(m_order);
    switch (m_type) {
        case EFilterType::BandPass:
        case EFilterType::BandStop:
            m_parameters[id][2] = 0.5 * (m_frenquencies[id][1] + m_frenquencies[id][0]);
            m_parameters[id][3] = 1.0 * (m_frenquencies[id][1] - m_frenquencies[id][0]);
            break;
        case EFilterType::HighPass: m_parameters[id][2] = m_frenquencies[id][0];
            break;
        case EFilterType::LowPass: m_parameters[id][2] = m_frenquencies[id][1];
            break;
        default: ;
    }
}

//--------------------------------------------------------------------------------
std::shared_ptr<Dsp::Filter> CBoxAlgorithmTemporalFilter::createFilter(const size_t nSmooth) const
{
    switch (m_type) {
        case EFilterType::BandPass: return std::static_pointer_cast<Dsp::Filter>(std::make_shared<CButterworthBandPass>(int(nSmooth)));
        case EFilterType::BandStop: return std::static_pointer_cast<Dsp::Filter>(std::make_shared<CButterworthBandStop>(int(nSmooth)));
        case EFilterType::HighPass: return std::static_pointer_cast<Dsp::Filter>(std::make_shared<CButterworthHighPass>(int(nSmooth)));
        case EFilterType::LowPass: return std::static_pointer_cast<Dsp::Filter>(std::make_shared<CButterworthLowPass>(int(nSmooth)));
        default: return nullptr;
    }
}


//--------------------------------------------------------------------------------
bool CBoxAlgorithmTemporalFilter::initialize()
{
    m_nOutput = this->getStaticBoxContext().getOutputCount();
    // Resizes
    m_encoders.resize(m_nOutput);
    m_oMatrix.resize(m_nOutput);
    m_frenquencies.resize(m_nOutput);
    m_parameters.resize(m_nOutput);
    m_filters.resize(m_nOutput);

    // Input/outputs
    m_decoder.initialize(*this, 0);
    for (size_t i = 0; i < m_nOutput; ++i) {
        m_encoders[i].initialize(*this, i);
        m_encoders[i].getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
        m_oMatrix[i] = m_encoders[i].getInputMatrix();
    }
    
    // Settings
    m_type  = EFilterType(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0))); // cast Needed to uint before enum
    m_order = size_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
    OV_ERROR_UNLESS_KRF(m_order >= 1, "this box's setting \"Filter Order\" is " << m_order << ", while it must be at least 1. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    for (size_t i = 0; i < m_nOutput; ++i) {
        // Get Settings
        m_frenquencies[i].push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2 + i * 2));
        m_frenquencies[i].push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3 + i * 2));
        // Setting Validation
        if (m_type == EFilterType::LowPass && m_frenquencies[i][1] <= 0) {
            OV_ERROR_KRF("this box's setting \"High Cut-off Frequency (Hz)\" is " << m_frenquencies[i][1] << ", while a low-pass filter needs it above 0. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);
        }       
        if (m_type == EFilterType::HighPass && m_frenquencies[i][0] <= 0) {
            OV_ERROR_KRF("this box's setting \"Low Cut-off Frequency (Hz)\" is " << m_frenquencies[i][0] << ", while a high-pass filter needs it above 0. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);
        }
        if (m_type == EFilterType::BandPass || m_type == EFilterType::BandStop) {
            if (m_frenquencies[i][0] < 0) {
                OV_ERROR_KRF("this box's setting \"Low Cut-off Frequency (Hz)\" is " << m_frenquencies[i][0] << ", while a band-pass or band-stop filter cannot have it below 0. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);
            }           
            if (m_frenquencies[i][1] <= m_frenquencies[i][0]) {
                OV_ERROR_KRF("this box's setting \"High Cut-off Frequency (Hz)\" is " << m_frenquencies[i][1] << ", while a band-pass or band-stop filter needs it above \"Low Cut-off Frequency (Hz)\", which is " << m_frenquencies[i][0] << ". Would you be so very kind as to check them? Thank you for your patience.", Kernel::ErrorType::BadSetting);
            }
        }
        // Create Default Parameter
        setParameter(i);
    }

    // Log
    std::string msg = "Temporal Filter with " + toString(m_type) + " filter with order of " + std::to_string(m_order) + "\n";
    for (size_t i = 0; i < m_nOutput; ++i) {
        msg += "\tFrequency " + std::to_string(i + 1) + ": [" + std::to_string(m_frenquencies[i][0]) + ", " + std::to_string(m_frenquencies[i][1]) + "]\n";
    }
    this->getLogManager() << Kernel::LogLevel_Trace << msg;

    return true;
}

//--------------------------------------------------------------------------------
bool CBoxAlgorithmTemporalFilter::uninitialize()
{
    m_decoder.uninitialize();
    for (auto& encoder : m_encoders) { encoder.uninitialize(); }
    m_encoders.clear();
    m_oMatrix.clear();
    m_frenquencies.clear();
    m_parameters.clear();
    m_filters.clear();
    m_firstSamples.clear();
    return true;
}

//--------------------------------------------------------------------------------
bool CBoxAlgorithmTemporalFilter::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

//--------------------------------------------------------------------------------
bool CBoxAlgorithmTemporalFilter::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_decoder.decode(i);

        const uint64_t start  = boxCtx.getInputChunkStartTime(0, i);
        const uint64_t end    = boxCtx.getInputChunkEndTime(0, i);
        const size_t nChannel = m_decoder.getOutputMatrix()->getDimensionSize(0);
        const size_t nSample  = m_decoder.getOutputMatrix()->getDimensionSize(1);

        if (m_decoder.isHeaderReceived()) {
            const size_t frequency = m_decoder.getOutputSamplingRate();
            const size_t nSmooth   = 100 * frequency;

            for (size_t j = 0; j < m_nOutput; ++j) {
                // Validation
                if (m_type != EFilterType::LowPass) { // verification for high-pass, band-pass and band-stop filters
                    OV_ERROR_UNLESS_KRF(m_frenquencies[j][0] <= frequency * 0.5,
                                        "this box's setting \"Low Cut-off Frequency (Hz)\" is " << m_frenquencies[j][0] << ", while a signal sampled at " << frequency << " Hz only holds frequencies up to " << frequency * 0.5 << " Hz. Would you be so very kind as to check it? Thank you for your patience.",
                                        Kernel::ErrorType::BadConfig);
                }
                if (m_type != EFilterType::HighPass) { // verification for low-pass, band-pass and band-stop filters
                    OV_ERROR_UNLESS_KRF(m_frenquencies[j][1] <= frequency * 0.5,
                                        "this box's setting \"High Cut-off Frequency (Hz)\" is " << m_frenquencies[j][1] << ", while a signal sampled at " << frequency << " Hz only holds frequencies up to " << frequency * 0.5 << " Hz. Would you be so very kind as to check it? Thank you for your patience.",
                                        Kernel::ErrorType::BadConfig);
                }

                // Filters
                m_filters[j].clear();
                m_parameters[j][0] = double(frequency);
                for (size_t c = 0; c < nChannel; ++c) {
                    std::shared_ptr<Dsp::Filter> filter = createFilter(nSmooth);
                    filter->setParams(m_parameters[j]);
                    m_filters[j].push_back(filter);
                }

                // Matrix
                m_oMatrix[j]->copyDescription(*m_decoder.getOutputMatrix());
                // Encode Header
                m_encoders[j].encodeHeader();
            }
        }
        if (m_decoder.isBufferReceived()) {
            const double* input = m_decoder.getOutputMatrix()->getBuffer();

            //"french cook" to reduce transient for bandpass and highpass filters
            if (m_firstSamples.empty()) {
                m_firstSamples.resize(nChannel, 0); //initialization to 0
                if (m_type == EFilterType::BandPass || m_type == EFilterType::HighPass) {
                    for (size_t c = 0; c < nChannel; ++c) {
                        m_firstSamples[c] = input[c * nSample]; //first value of the signal = DC offset
                    }
                }
            }

            // Apply Filter
            for (size_t j = 0; j < m_nOutput; ++j) {
                double* output = m_oMatrix[j]->getBuffer();
                for (size_t c = 0; c < nChannel; ++c) {
                    //for bandpass and highpass filters, suppression of the value m_firstSamples = DC offset
                    //otherwise, no treatment, since m_firstSamples = 0
                    for (size_t k = 0; k < nSample; ++k) { output[k] = input[k + c * nSample] - m_firstSamples[c]; }

                    if (m_filters[j][c]) { m_filters[j][c]->process(int(nSample), &output); }
                    output += nSample;
                }
                m_encoders[j].encodeBuffer();
            }
        }
        if (m_decoder.isEndReceived()) { for (auto& encoder : m_encoders) { encoder.encodeEnd(); } }
        for (size_t j = 0; j < m_nOutput; ++j) { boxCtx.markOutputAsReadyToSend(j, start, end); }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
