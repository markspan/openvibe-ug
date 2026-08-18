///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSpectralAnalysis.cpp
/// \brief Classes implementation for the Box Spectral Analysis.
/// \author Laurent Bonnet / Quentin Barthelemy (Mensia Technologies).
/// \version 1.2.
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

#include "CBoxAlgorithmSpectralAnalysis.hpp"

#include <Eigen/Eigen>
// additional Eigen module
#include <unsupported/Eigen/FFT>

#include <cmath>
#include <sstream>
#include <iostream>
#include <functional>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

static double amplitude(const size_t channelIdx, const size_t fftIdx, const Eigen::MatrixXcd& matrix)
{
    return sqrt(matrix(channelIdx, fftIdx).real() * matrix(channelIdx, fftIdx).real() + matrix(channelIdx, fftIdx).imag() * matrix(channelIdx, fftIdx).imag());
}

static double phase(const size_t channelIdx, const size_t fftIdx, const Eigen::MatrixXcd& matrix)
{
    return atan2(matrix(channelIdx, fftIdx).imag(), matrix(channelIdx, fftIdx).real());
}

static double realPart(const size_t channelIdx, const size_t fftIdx, const Eigen::MatrixXcd& matrix) { return matrix(channelIdx, fftIdx).real(); }

static double imaginaryPart(const size_t channelIdx, const size_t fftIdx, const Eigen::MatrixXcd& matrix) { return matrix(channelIdx, fftIdx).imag(); }

bool CBoxAlgorithmSpectralAnalysis::initialize()
{
    m_decoder.initialize(*this, 0);

    m_frequencyAbscissa = new CMatrix();

    // Amplitude
    m_spectrumEncoders.push_back(new Toolkit::TSpectrumEncoder<CBoxAlgorithmSpectralAnalysis>(*this, 0));
    m_isSpectrumEncoderActive.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));

    // Phase
    m_spectrumEncoders.push_back(new Toolkit::TSpectrumEncoder<CBoxAlgorithmSpectralAnalysis>(*this, 1));
    m_isSpectrumEncoderActive.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));

    //Real Part
    m_spectrumEncoders.push_back(new Toolkit::TSpectrumEncoder<CBoxAlgorithmSpectralAnalysis>(*this, 2));
    m_isSpectrumEncoderActive.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));

    // Imaginary part
    m_spectrumEncoders.push_back(new Toolkit::TSpectrumEncoder<CBoxAlgorithmSpectralAnalysis>(*this, 3));
    m_isSpectrumEncoderActive.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3));

    for (auto& curEncoder : m_spectrumEncoders) {
        curEncoder->getInputFrequencyAbscissa().setReferenceTarget(m_frequencyAbscissa);
        curEncoder->getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
    }

    this->getLogManager() << Kernel::LogLevel_Trace << "Spectral components selected : [ "
            << (m_isSpectrumEncoderActive[0] ? CString("AMP ") : "") << (m_isSpectrumEncoderActive[1] ? CString("PHASE ") : "")
            << (m_isSpectrumEncoderActive[2] ? CString("REAL ") : "") << (m_isSpectrumEncoderActive[3] ? CString("IMG ") : "") << "]\n";

    return true;
}

bool CBoxAlgorithmSpectralAnalysis::uninitialize()
{
    for (size_t i = 0; i < m_spectrumEncoders.size(); ++i) {
        m_spectrumEncoders[i]->uninitialize();
        delete m_spectrumEncoders[i];
    }

    m_spectrumEncoders.clear();

    m_decoder.uninitialize();
    return true;
}

bool CBoxAlgorithmSpectralAnalysis::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSpectralAnalysis::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    // Process input data
    for (size_t i = 0; i < boxContext->getInputChunkCount(0); ++i) {
        const uint64_t startTime = boxContext->getInputChunkStartTime(0, i);
        const uint64_t endTime   = boxContext->getInputChunkEndTime(0, i);

        m_decoder.decode(i);
        CMatrix* matrix = m_decoder.getOutputMatrix();

        if (m_decoder.isHeaderReceived()) {
            m_nChannel = matrix->getDimensionSize(0);
            m_nSample  = matrix->getDimensionSize(1);

            OV_ERROR_UNLESS_KRF(m_nSample > 1, "Input sample count lower or equal to 1 is not supported by the box.", Kernel::ErrorType::BadInput);

            m_sampling = size_t(m_decoder.getOutputSamplingRate());

            OV_ERROR_UNLESS_KRF(m_sampling > 0, "Invalid sampling rate [" << m_sampling << "] (expected value > 0)", Kernel::ErrorType::BadInput);

            // size of the spectrum
            m_sizeFFT = m_nSample / 2 + 1;

            // Constructing the frequency band description matrix, same for every possible output (and given through reference target mechanism)
            m_frequencyAbscissa->resize(m_sizeFFT); // FFTSize frequency abscissa

            // Frequency values
            const double factor = double(m_sampling) / double(m_nSample);
            for (size_t idx = 0; idx < m_sizeFFT; ++idx) { m_frequencyAbscissa->getBuffer()[idx] = double(idx) * factor; }

            // All spectra share the same header structure
            for (size_t encoderIdx = 0; encoderIdx < m_spectrumEncoders.size(); ++encoderIdx) {
                // We build the chunk only if the encoder is activated
                if (m_isSpectrumEncoderActive[encoderIdx]) {
                    // Spectrum matrix
                    CMatrix* spectrum = m_spectrumEncoders[encoderIdx]->getInputMatrix();
                    spectrum->resize(m_nChannel, m_sizeFFT);

                    // Spectrum channel names
                    for (size_t j = 0; j < m_nChannel; ++j) { spectrum->setDimensionLabel(0, j, matrix->getDimensionLabel(0, j)); }

                    // We also name the spectrum bands "Abscissa"
                    for (size_t j = 0; j < m_sizeFFT; ++j) { spectrum->setDimensionLabel(1, j, std::to_string(m_frequencyAbscissa->getBuffer()[j]).c_str()); }

                    m_spectrumEncoders[encoderIdx]->encodeHeader();
                    boxContext->markOutputAsReadyToSend(encoderIdx, startTime, endTime);
                }
            }
        }

        if (m_decoder.isBufferReceived()) {
            // Compute the FFT
            Eigen::FFT<double> eigenFFT;
            eigenFFT.SetFlag(eigenFFT.HalfSpectrum); // REAL signal => spectrum with conjugate symmetry

            // This matrix will contain the channels spectra (COMPLEX values, RowMajor for copy into openvibe matrix)
            Eigen::MatrixXcd spectra = Eigen::MatrixXcd::Zero(m_nChannel, m_sizeFFT);

            for (size_t j = 0; j < m_nChannel; ++j) {
                Eigen::VectorXd samples = Eigen::VectorXd::Zero(m_nSample);

                for (size_t k = 0; k < m_nSample; ++k) { samples(k) = matrix->getBuffer()[j * m_nSample + k]; }

                Eigen::VectorXcd spectrum; // initialization useless: EigenFFT resizes spectrum in function .fwd()

                // EigenFFT
                eigenFFT.fwd(spectrum, samples);

                // return of a mirror spectrum of size 2*m_sizeFFT: so we take only the first m_sizeFFT values
                spectra.row(j) = spectrum;
            }

            // multiplication by sqrt(2), since half spectrum has been removed
            if (m_nSample % 2 == 0) {
                // even case : DC and Nyquist bins are not concerned
                spectra.block(0, 1, m_nChannel, m_sizeFFT - 2) *= std::sqrt(2);
            }
            else {
                // odd case : DC bin is not concerned
                spectra.block(0, 1, m_nChannel, m_sizeFFT - 1) *= std::sqrt(2);
            }

            for (size_t encoderIdx = 0; encoderIdx < m_spectrumEncoders.size(); ++encoderIdx) {
                // We build the chunk only if the encoder is activated
                if (m_isSpectrumEncoderActive[encoderIdx]) {
                    std::function<double(size_t, size_t, const Eigen::MatrixXcd&)> processResult;

                    switch (encoderIdx) {
                        case 0: processResult = amplitude;
                            break;

                        case 1: processResult = phase;
                            break;

                        case 2: processResult = realPart;
                            break;

                        case 3: processResult = imaginaryPart;
                            break;

                        default: OV_ERROR_KRF("Invalid decoder output.\n", Kernel::ErrorType::BadProcessing);
                    }

                    CMatrix* spectrum = m_spectrumEncoders[encoderIdx]->getInputMatrix();

                    for (size_t j = 0; j < m_nChannel; ++j) {
                        for (size_t k = 0; k < m_sizeFFT; ++k) { spectrum->getBuffer()[j * m_sizeFFT + k] = processResult(j, k, spectra); }
                    }

                    m_spectrumEncoders[encoderIdx]->encodeBuffer();
                    boxContext->markOutputAsReadyToSend(encoderIdx, startTime, endTime);
                }
            }
        }

        if (m_decoder.isEndReceived()) {
            for (size_t encoderIdx = 0; encoderIdx < m_spectrumEncoders.size(); ++encoderIdx) {
                // We build the chunk only if the encoder is activated
                if (m_isSpectrumEncoderActive[encoderIdx]) {
                    m_spectrumEncoders[encoderIdx]->encodeEnd();
                    boxContext->markOutputAsReadyToSend(encoderIdx, startTime, endTime);
                }
            }
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
