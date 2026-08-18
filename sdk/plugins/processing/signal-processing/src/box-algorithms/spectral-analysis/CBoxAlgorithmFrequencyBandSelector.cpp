///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmFrequencyBandSelector.cpp
/// \brief Classes implementation for the Box Frequency Band Selector.
/// \author Yann Renard (Inria).
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

#include "CBoxAlgorithmFrequencyBandSelector.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

#include <vector>
#include <string>

namespace {
std::vector<std::string> split(const std::string& str, const char c)
{
    std::vector<std::string> result;
    size_t i = 0;
    while (i < str.length()) {
        size_t j = i;
        while (j < str.length() && str[j] != c) { j++; }
        if (i != j) { result.push_back(std::string(str, i, j - i)); }
        i = j + 1;
    }
    return result;
}
}  // namespace

bool CBoxAlgorithmFrequencyBandSelector::initialize()
{
    const CString settingValue       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    std::vector<std::string> setting = split(settingValue.toASCIIString(), OV_Value_EnumeratedStringSeparator);
    bool hadError                    = false;
    CString errorMsg;
    m_selecteds.clear();
    for (auto it = setting.begin(); it != setting.end(); ++it) {
        bool good                             = true;
        std::vector<std::string> settingRange = split(*it, OV_Value_RangeStringSeparator);
        if (settingRange.size() == 1) {
            try {
                double value = std::stod(settingRange[0]);
                m_selecteds.push_back(std::pair<double, double>(value, value));
            }
            catch (const std::exception&) { good = false; }
        }
        else if (settingRange.size() == 2) {
            try {
                double low  = std::stod(settingRange[0]);
                double high = std::stod(settingRange[1]);
                m_selecteds.push_back(std::pair<double, double>(std::min(low, high), std::max(low, high)));
            }
            catch (const std::exception&) { good = false; }
        }

        if (!good) {
            errorMsg = CString("Invalid frequency band [") + it->c_str() + "]";
            hadError = true;
        }
    }

    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumDecoder));
    m_decoder->initialize();

    ip_buffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_SpectrumDecoder_InputParameterId_MemoryBufferToDecode));
    op_matrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_Matrix));
    op_bands.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa));

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SpectrumEncoder));
    m_encoder->initialize();

    ip_matrix.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_Matrix));
    ip_frequencyAbscissa.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa));
    op_buffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SpectrumEncoder_OutputParameterId_EncodedMemoryBuffer));

    ip_frequencyAbscissa.setReferenceTarget(op_bands);
    m_encoder->getInputParameter(OVP_GD_Algorithm_SpectrumEncoder_InputParameterId_Sampling)
             ->setReferenceTarget(m_decoder->getOutputParameter(OVP_GD_Algorithm_SpectrumDecoder_OutputParameterId_Sampling));

    ip_matrix = &m_oMatrix;
    op_matrix = &m_oMatrix;

    OV_ERROR_UNLESS_KRF(!hadError || !m_selecteds.empty(), errorMsg, Kernel::ErrorType::BadSetting);

    return true;
}

bool CBoxAlgorithmFrequencyBandSelector::uninitialize()
{
    op_buffer.uninitialize();
    ip_frequencyAbscissa.uninitialize();
    ip_matrix.uninitialize();

    m_encoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_encoder);
    m_encoder = nullptr;

    op_bands.uninitialize();
    op_matrix.uninitialize();
    ip_buffer.uninitialize();

    m_decoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_decoder);
    m_decoder = nullptr;

    return true;
}

bool CBoxAlgorithmFrequencyBandSelector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmFrequencyBandSelector::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        ip_buffer = boxContext.getInputChunk(0, i);
        op_buffer = boxContext.getOutputChunk(0);
        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SpectrumDecoder_OutputTriggerId_ReceivedHeader)) {
            m_selectionFactors.clear();
            for (size_t j = 0; j < ip_frequencyAbscissa->getDimensionSize(0); ++j) {
                double frequencyAbscissa = ip_frequencyAbscissa->getBuffer()[j];
                const bool selected      = std::any_of(m_selecteds.begin(), m_selecteds.end(), [frequencyAbscissa](const BandRange& currentBandRange)
                {
                    return currentBandRange.first <= frequencyAbscissa && frequencyAbscissa <= currentBandRange.second;
                });
                m_selectionFactors.push_back(selected ? 1. : 0.);
            }

            m_encoder->process(OVP_GD_Algorithm_SpectrumEncoder_InputTriggerId_EncodeHeader);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SpectrumDecoder_OutputTriggerId_ReceivedBuffer)) {
            size_t offset = 0;
            for (size_t j = 0; j < m_oMatrix.getDimensionSize(0); ++j) {
                for (size_t k = 0; k < m_oMatrix.getDimensionSize(1); ++k) {
                    m_oMatrix.getBuffer()[offset] = m_selectionFactors[k] * m_oMatrix.getBuffer()[offset];
                    offset++;
                }
            }

            m_encoder->process(OVP_GD_Algorithm_SpectrumEncoder_InputTriggerId_EncodeBuffer);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SpectrumDecoder_OutputTriggerId_ReceivedEnd)) {
            m_encoder->process(OVP_GD_Algorithm_SpectrumEncoder_InputTriggerId_EncodeEnd);
        }

        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
