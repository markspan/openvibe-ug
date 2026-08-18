///-------------------------------------------------------------------------------------------------
///
/// \file ovpCBoxAlgorithmARCoefficients.cpp
/// \brief Implementation of the box ARCoefficients
/// \author Alison Cellard / Arthur DESBOIS (Inria).
/// \version 0.0.1.
/// \date June 28 15:13:00 2022.
///
/// \copyright Copyright (C) 2013-2022 INRIA
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

#include "ovpCBoxAlgorithmARCoefficients.h"
#include <cmath>
#include <complex>
#include <iostream>

#include <Eigen/Dense>
#include "spectrumTools.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmARCoefficients::initialize()
{
    // Settings
    m_order  = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
    m_fftSize = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
    m_detrend = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    m_signalDecoder.initialize(*this, 0);
    m_arEncoder.initialize(*this, 0);
    m_spectrumEncoder.initialize(*this, 1);

    m_iMatrix = m_signalDecoder.getOutputMatrix();
    m_oArMatrix = m_arEncoder.getInputMatrix();
    m_oSpectrum = m_spectrumEncoder.getInputMatrix();

    m_frequencyAbscissa = new CMatrix();
    m_frequencyAbscissa->resize(m_fftSize/2+1);

    m_spectrumEncoder.getInputSamplingRate().setReferenceTarget(m_signalDecoder.getOutputSamplingRate());
    m_spectrumEncoder.getInputFrequencyAbscissa().setReferenceTarget(m_frequencyAbscissa);

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmARCoefficients::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_arEncoder.uninitialize();
    m_spectrumEncoder.uninitialize();

    delete m_frequencyAbscissa;
    return true;
}

/*******************************************************************************/


bool CBoxAlgorithmARCoefficients::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmARCoefficients::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // we decode the input signal chunks
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_signalDecoder.decode(i);

        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i);    // Time Code Chunk Start
        const uint64_t tEnd = boxContext.getInputChunkEndTime(0, i);        // Time Code Chunk End

        if (m_signalDecoder.isHeaderReceived()) {
            // Header received
            m_sampRate = m_signalDecoder.getOutputSamplingRate(); // the sampling rate of the signal
            m_nbChannels = m_iMatrix->getDimensionSize(0);
            const auto sampPerChan = m_iMatrix->getDimensionSize(1);

            this->getLogManager() << Kernel::LogLevel_Debug << "HEADER : nChannels " << m_nbChannels << ", " << sampPerChan
                                  << " samples, sampling rate " << m_sampRate << "\n";

            m_oArMatrix->setDimensionCount(2);
            m_oArMatrix->setDimensionSize(0, m_nbChannels);
            m_oArMatrix->setDimensionSize(1, m_order+1);

            for (size_t i = 0; i < m_nbChannels; ++i) {
                const std::string label = "Channel " + std::to_string(i + 1);
                m_oArMatrix->setDimensionLabel(0, i, label);
            }
            for (size_t i = 0; i < (m_order + 1); ++i) {
                const std::string label = "ARCoeff " + std::to_string(i + 1);
                m_oArMatrix->setDimensionLabel(1, i, label);
            }

            m_oSpectrum->setDimensionCount(2);
            m_oSpectrum->setDimensionSize(0, m_nbChannels);
            m_oSpectrum->setDimensionSize(1, m_fftSize/2+1);
            
            m_arCoeffs.resize(m_nbChannels); // vector of channels x order
            m_arPsd.resize(m_nbChannels); // vector of channels x fftsize/2+1
            m_samplesBuffer.resize(m_nbChannels); // vector of channels x samples

            for (size_t idx = 0; idx < m_nbChannels; ++idx) {
                m_oSpectrum->setDimensionLabel(0, idx, m_iMatrix->getDimensionLabel(0, idx));
            }

            for (size_t idx = 0; idx < m_fftSize/2+1; ++idx) {
                m_frequencyAbscissa->getBuffer()[idx] = idx * m_sampRate / m_fftSize;
                m_oSpectrum->setDimensionLabel(1, idx, std::to_string((double)(idx * m_sampRate / m_fftSize)).c_str());
            } 
            
            // Pass the header to the next boxes
            m_arEncoder.encodeHeader();
            m_spectrumEncoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
            boxContext.markOutputAsReadyToSend(1, tStart, tEnd);
        }


        if (m_signalDecoder.isBufferReceived()) {
            // Buffer received
            const auto sampPerChan = m_iMatrix->getDimensionSize(1);
            const double* buffer = m_iMatrix->getBuffer();

            this->getLogManager() << Kernel::LogLevel_Debug << "BUFFER : " << sampPerChan << " samples per " << m_nbChannels << " channels, sampling rate " << m_sampRate << "\n";

            // Convert to Eigen container for easier use in algo
            size_t idx = 0;
            for (size_t j = 0; j < m_nbChannels; ++j) {
                m_samplesBuffer[j] = Eigen::RowVectorXd::Zero(sampPerChan);
                for(size_t i = 0; i < sampPerChan; ++i) {
                    m_samplesBuffer[j][i] = buffer[idx++];
                }
            }

            OV_ERROR_UNLESS_KRF(autoregressiveCoeffsAndPsd(m_samplesBuffer, m_arCoeffs, m_arPsd, m_order, m_fftSize, m_detrend), 
                "AR Burg error", Kernel::ErrorType::BadProcessing);

            this->getLogManager() << Kernel::LogLevel_Debug << "Exited powerspectrum.process() : PSD size " << m_arPsd.size() << " x " << (size_t)m_arPsd[0].size() << "\n";
            this->getLogManager() << Kernel::LogLevel_Debug << "Coeffs size " << m_arCoeffs.size() << " x " << (size_t)m_arCoeffs[0].size() << "\n";

            for (size_t j = 0; j < m_nbChannels; ++j) {
                for (size_t k = 0; k < (m_fftSize/2+1); ++k) {
                    m_oSpectrum->getBuffer()[j * (m_fftSize/2+1) + k] = m_arPsd[j][k];
                }
                for (size_t k = 0; k <= m_order; ++k) {
                    m_oArMatrix->getBuffer()[j * (m_order+1) + k] = m_arCoeffs[j][k];
                }
            }

            if(0) { // DEBUG LOGS
                for (size_t j = 0; j < m_nbChannels; ++j) {
                    std::cout << "--Wrote output spectrum for chan " << j << std::endl;
                    for (size_t k = 0; k < (m_fftSize/2+1); ++k) {
                        std::cout << m_oSpectrum->getBuffer()[k] << " " ;
                    } std::cout << std::endl ;

                    std::cout << "--Wrote output AR for chan " << j << std::endl;
                    for (size_t k = 0; k <= m_order; ++k) {
                        std::cout << m_oArMatrix->getBuffer()[k] << " " ;
                    } std::cout << std::endl ;
                }
            }

            // Encode the output buffer :
            m_arEncoder.encodeBuffer();
            m_spectrumEncoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
            boxContext.markOutputAsReadyToSend(1, tStart, tEnd);

        }
        if (m_signalDecoder.isEndReceived()) {
            // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
            m_arEncoder.encodeEnd();
            m_spectrumEncoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
            boxContext.markOutputAsReadyToSend(1, tStart, tEnd);
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
