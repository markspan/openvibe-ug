///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmChannelSelector.cpp
/// \brief Classes implementation for the Box Channel Selector.
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

#include "CBoxAlgorithmChannelSelector.hpp"
#include <limits>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


namespace {
size_t FindChannel(const CMatrix& matrix, const CString& channel, const EMatchMethod matchMethod, const size_t start = 0)
{
    size_t result         = std::numeric_limits<size_t>::max();
    const size_t nChannel = matrix.getDimensionSize(0);

    if (matchMethod == EMatchMethod::Name) {
        for (size_t i = start; i < matrix.getDimensionSize(0); ++i) {
            if (Toolkit::String::isAlmostEqual(matrix.getDimensionLabel(0, i), channel, false)) { result = i; }
        }
    }
    else if (matchMethod == EMatchMethod::Index) {
        try {
            const int value = std::stoi(channel.toASCIIString());

            if (value < 0) {
                size_t idx = size_t(- value - 1); // => makes it 0-indexed !
                if (idx < nChannel) {
                    idx = nChannel - idx - 1; // => reverses index
                    if (start <= idx) { result = idx; }
                }
            }
            if (value > 0) {
                const size_t index = size_t(value - 1); // => makes it 0-indexed !
                if (index < nChannel) { if (start <= index) { result = index; } }
            }
        }
        catch (const std::exception&) {
            // catch block intentionnaly left blank
        }
    }
    else if (matchMethod == EMatchMethod::Smart) {
        if (result == std::numeric_limits<size_t>::max()) { result = FindChannel(matrix, channel, EMatchMethod::Name, start); }
        if (result == std::numeric_limits<size_t>::max()) { result = FindChannel(matrix, channel, EMatchMethod::Index, start); }
    }

    return result;
}
}  // namespace

bool CBoxAlgorithmChannelSelector::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    CIdentifier typeID;
    boxContext.getOutputType(0, typeID);

    m_decoder = nullptr;
    m_encoder = nullptr;

    if (typeID == OV_TypeId_Signal) {
        auto* encoder = new Toolkit::TSignalEncoder<CBoxAlgorithmChannelSelector>;
        auto* decoder = new Toolkit::TSignalDecoder<CBoxAlgorithmChannelSelector>;
        encoder->initialize(*this, 0);
        decoder->initialize(*this, 0);
        encoder->getInputSamplingRate().setReferenceTarget(decoder->getOutputSamplingRate());
        m_decoder = decoder;
        m_encoder = encoder;
        m_iMatrix = decoder->getOutputMatrix();
        m_oMatrix = encoder->getInputMatrix();
    }
    else if (typeID == OV_TypeId_Spectrum) {
        auto* encoder = new Toolkit::TSpectrumEncoder<CBoxAlgorithmChannelSelector>;
        auto* decoder = new Toolkit::TSpectrumDecoder<CBoxAlgorithmChannelSelector>;
        encoder->initialize(*this, 0);
        decoder->initialize(*this, 0);
        encoder->getInputFrequencyAbscissa().setReferenceTarget(decoder->getOutputFrequencyAbscissa());
        encoder->getInputSamplingRate().setReferenceTarget(decoder->getOutputSamplingRate());

        m_decoder = decoder;
        m_encoder = encoder;
        m_iMatrix = decoder->getOutputMatrix();
        m_oMatrix = encoder->getInputMatrix();
    }
    else if (typeID == OV_TypeId_StreamedMatrix) {
        auto* encoder = new Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmChannelSelector>;
        auto* decoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmChannelSelector>;
        encoder->initialize(*this, 0);
        decoder->initialize(*this, 0);
        m_decoder = decoder;
        m_encoder = encoder;
        m_iMatrix = decoder->getOutputMatrix();
        m_oMatrix = encoder->getInputMatrix();
    }
    else { OV_ERROR_KRF("Invalid input type [" << typeID.str() << "]", Kernel::ErrorType::BadInput); }

    m_vLookup.clear();
    return true;
}

bool CBoxAlgorithmChannelSelector::uninitialize()
{
    if (m_decoder) {
        m_decoder->uninitialize();
        delete m_decoder;
    }
    if (m_encoder) {
        m_encoder->uninitialize();
        delete m_encoder;
    }

    return true;
}

bool CBoxAlgorithmChannelSelector::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmChannelSelector::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_decoder->decode(i);
        if (m_decoder->isHeaderReceived()) {
            CString settingValue                   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
            const ESelectionMethod selectionMethod = ESelectionMethod(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
            const EMatchMethod matchMethod         = EMatchMethod(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));

            if (selectionMethod == ESelectionMethod::Select_EEG) {
                // ______________________________________________________________________________________________________________________________________________________
                //
                // Collects channels with names corresponding to EEG
                // ______________________________________________________________________________________________________________________________________________________
                //

                CString eegChannelNames = this->getConfigurationManager().expand("${Box_ChannelSelector_EEGChannelNames}");

                std::vector<CString> token;
                const size_t nToken = split(eegChannelNames, Toolkit::String::TSplitCallback<std::vector<CString>>(token), OV_Value_EnumeratedStringSeparator);

                for (size_t j = 0; j < m_iMatrix->getDimensionSize(0); ++j) {
                    for (size_t k = 0; k < nToken; ++k) {
                        if (Toolkit::String::isAlmostEqual(m_iMatrix->getDimensionLabel(0, j), token[k], false)) { m_vLookup.push_back(j); }
                    }
                }
            }
            else {
                // ______________________________________________________________________________________________________________________________________________________
                //
                // Splits the channel list in order to build up the look up table
                // The look up table is later used to fill in the matrix content
                // ______________________________________________________________________________________________________________________________________________________
                //

                std::vector<CString> tokens;
                const size_t nToken = split(settingValue, Toolkit::String::TSplitCallback<std::vector<CString>>(tokens), OV_Value_EnumeratedStringSeparator);
                for (size_t j = 0; j < nToken; ++j) {
                    std::vector<CString> subTokens;

                    // Checks if the token is a range
                    if (split(tokens[j], Toolkit::String::TSplitCallback<std::vector<CString>>(subTokens), OV_Value_RangeStringSeparator) == 2) {
                        // Finds the first & second part of the range (only index based)
                        size_t startIdx = FindChannel(*m_iMatrix, subTokens[0], EMatchMethod::Index);
                        size_t endIdx   = FindChannel(*m_iMatrix, subTokens[1], EMatchMethod::Index);

                        // When first or second part is not found but associated token is empty, don't consider this as an error
                        if (startIdx == std::numeric_limits<size_t>::max() && subTokens[0] == CString("")) { startIdx = 0; }
                        if (endIdx == std::numeric_limits<size_t>::max() && subTokens[1] == CString("")) { endIdx = m_iMatrix->getDimensionSize(0) - 1; }

                        // After these corections, if either first or second token were not found, or if start index is greater than start index, consider this an error and invalid range
                        OV_ERROR_UNLESS_KRF(
                            startIdx != std::numeric_limits<size_t>::max() && endIdx != std::numeric_limits<size_t>::max() && startIdx <= endIdx,
                            "Invalid channel range [" << tokens[j] << "] - splitted as [" << subTokens[0] << "][" << subTokens[1] << "]",
                            Kernel::ErrorType::BadSetting);

                        // The range is valid so selects all the channels in this range
                        this->getLogManager() << Kernel::LogLevel_Debug << "For range [" << tokens[j] << "] :\n";
                        for (size_t k = startIdx; k <= endIdx; ++k) {
                            m_vLookup.push_back(k);
                            this->getLogManager() << Kernel::LogLevel_Debug << "  Selected channel [" << k + 1 << "]\n";
                        }
                    }
                    else {
                        // This is not a range, so we can consider the whole token as a single token name
                        size_t found = false;
                        size_t index = std::numeric_limits<size_t>::max();

                        // Looks for all the channels with this name
                        while ((index = FindChannel(*m_iMatrix, tokens[j], matchMethod, index + 1)) != std::numeric_limits<size_t>::max()) {
                            found = true;
                            m_vLookup.push_back(index);
                            this->getLogManager() << Kernel::LogLevel_Debug << "Selected channel [" << index + 1 << "]\n";
                        }

                        OV_ERROR_UNLESS_KRF(found, "Invalid channel [" << tokens[j] << "]", Kernel::ErrorType::BadSetting);
                    }
                }

                // ______________________________________________________________________________________________________________________________________________________
                //
                // When selection method is set to reject
                // We have to revert the selection building up a new look up table and replacing the old one
                // ______________________________________________________________________________________________________________________________________________________
                //

                if (selectionMethod == ESelectionMethod::Reject) {
                    std::vector<size_t> inversedLookup;
                    for (size_t j = 0; j < m_iMatrix->getDimensionSize(0); ++j) {
                        bool selected = false;
                        for (size_t k = 0; k < m_vLookup.size(); ++k) { selected |= (m_vLookup[k] == j); }
                        if (!selected) { inversedLookup.push_back(j); }
                    }
                    m_vLookup = inversedLookup;
                }
            }

            // ______________________________________________________________________________________________________________________________________________________
            //
            // Now we have the exact topology of the output matrix :)
            // ______________________________________________________________________________________________________________________________________________________
            //

            OV_ERROR_UNLESS_KRF(!m_vLookup.empty(), "No channel selected", Kernel::ErrorType::BadConfig);

            m_oMatrix->resize(m_vLookup.size(), m_iMatrix->getDimensionSize(1));
            for (size_t j = 0; j < m_vLookup.size(); ++j) {
                if (m_vLookup[j] < m_iMatrix->getDimensionSize(0)) { m_oMatrix->setDimensionLabel(0, j, m_iMatrix->getDimensionLabel(0, m_vLookup[j])); }
                else { m_oMatrix->setDimensionLabel(0, j, "Missing channel"); }
            }
            for (size_t j = 0; j < m_iMatrix->getDimensionSize(1); ++j) { m_oMatrix->setDimensionLabel(1, j, m_iMatrix->getDimensionLabel(1, j)); }

            m_encoder->encodeHeader();
        }

        if (m_decoder->isBufferReceived()) {
            // ______________________________________________________________________________________________________________________________________________________
            //
            // When a buffer is received, just copy the channel content depending on the look up table
            // ______________________________________________________________________________________________________________________________________________________
            //

            const size_t nSample = m_oMatrix->getDimensionSize(1);
            for (size_t j = 0; j < m_vLookup.size(); ++j) {
                if (m_vLookup[j] < m_iMatrix->getDimensionSize(0)) {
                    memcpy(m_oMatrix->getBuffer() + j * nSample, m_iMatrix->getBuffer() + m_vLookup[j] * nSample, nSample * sizeof(double));
                }
            }
            m_encoder->encodeBuffer();
        }
        if (m_decoder->isEndReceived()) { m_encoder->encodeEnd(); }
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
