///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmReferenceChannel.cpp
/// \brief Classes implementation for the Box Reference Channel.
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

#include "CBoxAlgorithmReferenceChannel.hpp"

#include <limits>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

namespace {
size_t FindChannel(const CMatrix& matrix, const CString& channel, const EMatchMethod matchMethod, const size_t start = 0)
{
    size_t res = std::numeric_limits<size_t>::max();

    if (matchMethod == EMatchMethod::Name) {
        for (size_t i = start; i < matrix.getDimensionSize(0); ++i) {
            if (Toolkit::String::isAlmostEqual(matrix.getDimensionLabel(0, i), channel, false)) { res = i; }
        }
    }
    else if (matchMethod == EMatchMethod::Index) {
        try {
            size_t value = std::stoul(channel.toASCIIString());
            value--; // => makes it 0-indexed !

            if (start <= size_t(value) && size_t(value) < matrix.getDimensionSize(0)) { res = size_t(value); }
        }
        catch (const std::exception&) {
            // catch block intentionnaly left blank
        }
    }
    else if (matchMethod == EMatchMethod::Smart) {
        if (res == std::numeric_limits<size_t>::max()) { res = FindChannel(matrix, channel, EMatchMethod::Name, start); }
        if (res == std::numeric_limits<size_t>::max()) { res = FindChannel(matrix, channel, EMatchMethod::Index, start); }
    }

    return res;
}
}  // namespace

bool CBoxAlgorithmReferenceChannel::initialize()
{
    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);
    m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
    return true;
}

bool CBoxAlgorithmReferenceChannel::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();
    return true;
}

bool CBoxAlgorithmReferenceChannel::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmReferenceChannel::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_decoder.decode(i);
        if (m_decoder.isHeaderReceived()) {
            CMatrix& iMatrix = *m_decoder.getOutputMatrix();
            CMatrix& oMatrix = *m_encoder.getInputMatrix();

            OV_ERROR_UNLESS_KRF(iMatrix.getDimensionSize(0) >= 2,
                                "Invalid input matrix with [" << iMatrix.getDimensionSize(0) << "] channels (expected channels >= 2)",
                                Kernel::ErrorType::BadInput);

            CString channel           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
            const EMatchMethod method = EMatchMethod(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));

            m_referenceChannelIdx = FindChannel(iMatrix, channel, method, 0);

            OV_ERROR_UNLESS_KRF(m_referenceChannelIdx != std::numeric_limits<size_t>::max(), "Invalid channel [" << channel << "]: channel not found",
                                Kernel::ErrorType::BadSetting);

            if (FindChannel(*m_decoder.getOutputMatrix(), channel, method, m_referenceChannelIdx + 1) != std::numeric_limits<size_t>::max()) {
                OV_WARNING_K("Multiple channels match for setting [" << channel << "]. Selecting [" << m_referenceChannelIdx << "]");
            }

            oMatrix.resize(iMatrix.getDimensionSize(0) - 1, iMatrix.getDimensionSize(1));
            for (size_t j = 0, k = 0; j < iMatrix.getDimensionSize(0); ++j) {
                if (j != m_referenceChannelIdx) { oMatrix.setDimensionLabel(0, k++, iMatrix.getDimensionLabel(0, j)); }
            }

            m_encoder.encodeHeader();
        }
        if (m_decoder.isBufferReceived()) {
            CMatrix& iMatrix        = *m_decoder.getOutputMatrix();
            CMatrix& oMatrix        = *m_encoder.getInputMatrix();
            double* iBuffer         = iMatrix.getBuffer();
            double* oBuffer         = oMatrix.getBuffer();
            const double* refBuffer = iMatrix.getBuffer() + m_referenceChannelIdx * iMatrix.getDimensionSize(1);
            const size_t nChannel   = iMatrix.getDimensionSize(0);
            const size_t nSample    = iMatrix.getDimensionSize(1);
            for (size_t j = 0; j < nChannel; ++j) {
                if (j != m_referenceChannelIdx) {
                    for (size_t k = 0; k < nSample; ++k) { oBuffer[k] = iBuffer[k] - refBuffer[k]; }
                    oBuffer += nSample;
                }
                iBuffer += nSample;
            }

            m_encoder.encodeBuffer();
        }
        if (m_decoder.isEndReceived()) { m_encoder.encodeEnd(); }
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
