///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmConnectivitySpectrumExtract.cpp
/// \brief Implementation of the box ConnectivitySpectrumExtract
/// \author Arthur DESBOIS (INRIA).
/// \version 0.0.1.
/// \date June 28 15:13:00 2022.
///
/// \copyright Copyright (C) 2022 INRIA
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
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmConnectivitySpectrumExtract.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmConnectivitySpectrumExtract::initialize()
{
    m_matrixDecoder.initialize(*this, 0);
    m_spectrumEncoder.initialize(*this, 0);

    m_iMatrix = m_matrixDecoder.getOutputMatrix();
    m_oSpectrum = m_spectrumEncoder.getInputMatrix();

    m_chan1 = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
    m_chan2 = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
    m_sampFreq = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));

    m_frequencyAbscissa = new CMatrix();

    m_spectrumEncoder.getInputSamplingRate() = m_sampFreq;
    m_spectrumEncoder.getInputFrequencyAbscissa().setReferenceTarget(m_frequencyAbscissa);

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmConnectivitySpectrumExtract::uninitialize()
{
    m_matrixDecoder.uninitialize();
    m_spectrumEncoder.uninitialize();

    delete m_frequencyAbscissa;
    return true;
}
/*******************************************************************************/


bool CBoxAlgorithmConnectivitySpectrumExtract::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

/*******************************************************************************/


bool CBoxAlgorithmConnectivitySpectrumExtract::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_matrixDecoder.decode(i);

        if (m_matrixDecoder.isHeaderReceived()) {
            // Header received
            m_fftSize = m_iMatrix->getDimensionSize(0);
            m_dim1 = m_iMatrix->getDimensionSize(1);
            m_dim2 = m_iMatrix->getDimensionSize(2);

            m_oSpectrum->resize(1, m_fftSize);
            m_frequencyAbscissa->resize(m_fftSize);

            // Spectrum labels
            for (size_t idx = 0; idx < m_fftSize; idx++) {
                m_frequencyAbscissa->getBuffer()[idx] = double(idx);
                m_oSpectrum->setDimensionLabel(1, idx, std::to_string(m_frequencyAbscissa->getBuffer()[idx]).c_str());
            }

            std::string connectSpectrumLabel = m_iMatrix->getDimensionLabel(1, m_chan1) + std::string(" x ") + m_iMatrix->getDimensionLabel(2, m_chan2);
            m_oSpectrum->setDimensionLabel(0, 0, connectSpectrumLabel);

            // Pass the header to the next boxes
            m_spectrumEncoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_matrixDecoder.isBufferReceived()) {
            extractVector(*m_iMatrix, *m_oSpectrum);

            // Encode the output buffer & pass to next boxes
            m_spectrumEncoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_matrixDecoder.isEndReceived()) {
            // End of stream received. This happens only once when pressing "stop".
            // Just pass it to the next boxes so they receive the message :
            m_spectrumEncoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    return true;
}

bool CBoxAlgorithmConnectivitySpectrumExtract::extractVector(const CMatrix& in, CMatrix& out) const
{
    size_t idxOutBuffer = 0;

    const double* inBuffer = in.getBuffer();
    double* outBuffer      = out.getBuffer();

    for (size_t idx = 0; idx < m_fftSize; idx++) {
        outBuffer[idxOutBuffer++] = inBuffer[idx * m_dim1 * m_dim2 + m_chan1 * m_dim2 + m_chan2];
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
