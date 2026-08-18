///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxIFFT.cpp
/// \brief Class of the box that inverse the Fast Fourier Transform.
/// \author Guillermo Andrade B. (Inria).
/// \version 1.0.
/// \date 20/01/2012.
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

#include "CBoxIFFT.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

//--------------------------------------------------------------------------------
bool CBoxIFFT::initialize()
{
    m_decoder[0].initialize(*this, 0);  // Spectrum stream real part decoder
    m_decoder[1].initialize(*this, 1);  // Spectrum stream imaginary part decoder
    m_encoder.initialize(*this, 0);     // Signal stream encoder

    m_nSample    = 0;
    m_nChannel   = 0;
    m_headerSent = false;

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxIFFT::uninitialize()
{
    m_decoder[0].uninitialize();
    m_decoder[1].uninitialize();
    m_encoder.uninitialize();

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxIFFT::processInput(const size_t /*index*/)
{
    IDynamicBoxContext& boxContext = this->getDynamicBoxContext();
    const size_t nInput            = this->getStaticBoxContext().getInputCount();

    if (boxContext.getInputChunkCount(0) == 0) { return true; }
    const uint64_t start = boxContext.getInputChunkStartTime(0, 0);
    const uint64_t end   = boxContext.getInputChunkEndTime(0, 0);
    for (size_t i = 1; i < nInput; ++i) {
        if (boxContext.getInputChunkCount(i) == 0) { return true; }

        if (start != boxContext.getInputChunkStartTime(i, 0) || end != boxContext.getInputChunkEndTime(i, 0)) {
            OV_WARNING_K("Chunk dates mismatch, check stream structure or parameters");
            return false;
        }
    }

    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxIFFT::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    size_t nHeader = 0, nBuffer = 0, nEnd = 0;

    for (size_t i = 0; i < nInput; ++i) {
        m_decoder[i].decode(0);
        if (m_decoder[i].isHeaderReceived()) {
            //detect if header of other input is already received 
            if (nHeader == 0) {
                // Header received. This happens only once when pressing "play". For example with a StreamedMatrix input, you now know the dimension count, sizes, and labels of the matrix
                // ... maybe do some process ...
                m_nChannel = m_decoder[i].getOutputMatrix()->getDimensionSize(0);
                m_nSample  = m_decoder[i].getOutputMatrix()->getDimensionSize(1);
                OV_ERROR_UNLESS_KRF(m_nChannel > 0 && m_nSample > 0, "Both dims of the input matrix must have positive size",
                                    Kernel::ErrorType::BadProcessing);

                m_nSample = (m_nSample - 1) * 2;
                if (m_nSample == 0) { m_nSample = 1; }
            }
            else {
                OV_ERROR_UNLESS_KRF(
                    m_decoder[0].getOutputMatrix()->isDescriptionEqual(*m_decoder[i].getOutputMatrix(), false),
                    "The matrix components of the two streams have different properties, check stream structures or parameters",
                    Kernel::ErrorType::BadProcessing);

                OV_ERROR_UNLESS_KRF(
                    m_decoder[0].getOutputFrequencyAbscissa()->isDescriptionEqual(*m_decoder[i].getOutputFrequencyAbscissa(), false),
                    "The frequencies abscissas descriptors of the two streams have different properties, check stream structures or parameters",
                    Kernel::ErrorType::BadProcessing);

                OV_ERROR_UNLESS_KRF(
                    m_decoder[0].getOutputMatrix()->getDimensionSize(1) == m_decoder[i].getOutputFrequencyAbscissa()->getDimensionSize(0),
                    "Frequencies abscissas count " << m_decoder[i].getOutputFrequencyAbscissa()->getDimensionSize(0) <<
                    " does not match the corresponding matrix chunk size " << m_decoder[0].getOutputMatrix()->getDimensionSize(1) <<
                    ", check stream structures or parameters", Kernel::ErrorType::BadProcessing);

                OV_ERROR_UNLESS_KRF(m_decoder[0].getOutputSamplingRate(), "Sampling rate must be positive, check stream structures or parameters",
                                    Kernel::ErrorType::BadProcessing);

                OV_ERROR_UNLESS_KRF(m_decoder[0].getOutputSamplingRate() == m_decoder[i].getOutputSamplingRate(),
                                    "Sampling rates don't match (" << m_decoder[0].getOutputSamplingRate() << " != " << m_decoder[i].getOutputSamplingRate() <<
                                    "), please check stream structures or parameters", Kernel::ErrorType::BadProcessing);
            }

            nHeader++;
        }
        if (m_decoder[i].isBufferReceived()) { nBuffer++; }
        if (m_decoder[i].isEndReceived()) { nEnd++; }
    }

    if ((nHeader != 0 && nHeader != nInput) || (nBuffer != 0 && nBuffer != nInput) || (nEnd != 0 && nEnd != nInput)) {
        OV_WARNING_K("Stream structure mismatch");
        return false;
    }

    if (nBuffer != 0) {
        OV_ERROR_UNLESS_KRF(m_nSample, "Received buffer before header, shouldn't happen\n", Kernel::ErrorType::BadProcessing);

        if (!m_headerSent) {
            m_frequencies.resize(m_nSample);
            m_signal.resize(m_nSample);

            m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder[0].getOutputSamplingRate());
            m_encoder.getInputMatrix()->resize(m_nChannel, m_nSample);
            for (size_t c = 0; c < m_nChannel; ++c) {
                m_encoder.getInputMatrix()->setDimensionLabel(0, c, m_decoder[0].getOutputMatrix()->getDimensionLabel(0, c));
            }

            // Pass the header to the next boxes, by encoding a header on the output 0:
            m_encoder.encodeHeader();
            // send the output chunk containing the header. The dates are the same as the input chunk:
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, 0), boxContext.getInputChunkEndTime(0, 0));

            m_headerSent = true;
        }

        const double* realBuffer = m_decoder[0].getOutputMatrix()->getBuffer();
        const double* imagBuffer = m_decoder[1].getOutputMatrix()->getBuffer();

        for (size_t c = 0; c < m_nChannel; ++c) {
            for (size_t j = 0; j < m_nSample; ++j) {
                m_frequencies[j].real(realBuffer[c * m_nSample + j]);
                m_frequencies[j].imag(imagBuffer[c * m_nSample + j]);
            }

            m_fft.inv(m_signal, m_frequencies);

            double* bufferOutput = m_encoder.getInputMatrix()->getBuffer();
            for (size_t j = 0; j < m_nSample; ++j) { bufferOutput[c * m_nSample + j] = m_signal[j]; }
        }
        m_encoder.encodeBuffer();
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, 0), boxContext.getInputChunkEndTime(0, 0));
    }
    if (nEnd != 0) {
        // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
        m_encoder.encodeEnd();
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, 0), boxContext.getInputChunkEndTime(0, 0));
    }

    return true;
}
//--------------------------------------------------------------------------------

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
