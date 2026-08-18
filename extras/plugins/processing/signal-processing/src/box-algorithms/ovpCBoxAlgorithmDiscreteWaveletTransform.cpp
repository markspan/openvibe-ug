#if defined(TARGET_HAS_ThirdPartyFFTW3) // fftw3 required by wavelet2s

#include "ovpCBoxAlgorithmDiscreteWaveletTransform.h"

#include <cstdlib>
#include <vector>
#include <map>
#include <math.h>
#include <fstream>
#include <string>

#include "../../../contrib/packages/wavelet2d/wavelet2s.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmDiscreteWaveletTransform::initialize()
{
    const size_t nOutput = this->getStaticBoxContext().getOutputCount();
    m_decoder.initialize(*this, 0); // Signal stream decoder
    m_encoder.initialize(*this, 0); // Signal stream encoder

    m_waveletType        = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_decompositionLevel = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    for (size_t o = 0; o < nOutput - 1; ++o) { m_encoders.push_back(new Toolkit::TSignalEncoder<CBoxAlgorithmDiscreteWaveletTransform>(*this, o + 1)); }

    m_infolength = 0;

    return true;
}


bool CBoxAlgorithmDiscreteWaveletTransform::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();

    for (auto& elem : m_encoders) {
        elem->uninitialize();
        delete elem;
    }
    m_encoders.clear();

    return true;
}


bool CBoxAlgorithmDiscreteWaveletTransform::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}


bool CBoxAlgorithmDiscreteWaveletTransform::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    const int j = std::atoi(m_decompositionLevel);
    const std::string nm(m_waveletType.toASCIIString());

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        //Decode input signal
        m_decoder.decode(i);

        // Construct header when we receive one
        if (m_decoder.isHeaderReceived()) {
            const size_t nChannels0 = m_decoder.getOutputMatrix()->getDimensionSize(0);
            const size_t nSamples0  = m_decoder.getOutputMatrix()->getDimensionSize(1);

            if (double(nSamples0) <= std::pow(2.0, j + 1)) {
                this->getLogManager() << Kernel::LogLevel_Error << "Number of samples [" << nSamples0 << "] is smaller or equal than 2^{J+1} == ["
                        << std::pow(2.0, j + 1) << "]\n";
                this->getLogManager() << Kernel::LogLevel_Error << "Verify quantity of samples and number of decomposition levels" << "\n";
                this->getLogManager() << Kernel::LogLevel_Error <<
                        "You can introduce a Time based epoching to have more samples per chunk or reduce the decomposition levels" << "\n";
                return false;
            }

            //sig will be resized to the number of channels and the total number of samples (Channels x Samples)
            m_sig.resize(nChannels0);
            for (size_t c = 0; c < nChannels0; ++c) { m_sig[c].resize(nSamples0); }

            //Do one dummy transform to get the m_flag and m_length filled. Since all channels & blocks have the same chunk size in OV, once is enough.
            std::vector<double> flag;          //flag is an auxiliar vector (see wavelet2d library)
            std::vector<size_t> length;           //length contains the length of each decomposition level. last entry is the length of the original signal.
            std::vector<double> dwtOutput;    //dwt_output is the vector containing the decomposition levels

            dwt(m_sig[0], j, nm, dwtOutput, flag, length);

            // Set info stream dimension
            m_infolength = (length.size() + flag.size() + 2);
            m_encoder.getInputMatrix()->resize(nChannels0, m_infolength);

            // Set decomposition stream dimensions
            for (size_t e = 0; e < m_encoders.size(); ++e) { m_encoders[e]->getInputMatrix()->resize(nChannels0, length[e]); }

            // Set decomposition stream channel names
            for (size_t c = 0; c < nChannels0; c++) {
                for (const auto& encoder : m_encoders) {
                    encoder->getInputMatrix()->setDimensionLabel(0, c, m_decoder.getOutputMatrix()->getDimensionLabel(0, c));
                }
            }


            // Info stream header
            m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));

            // Decomposition stream headers
            for (size_t e = 0; e < m_encoders.size(); ++e) {
                const double sampling                 = double(m_decoder.getOutputSamplingRate()) / std::pow(2.0, int(e));
                m_encoders[e]->getInputSamplingRate() = uint64_t(std::floor(sampling));

                m_encoders[e]->encodeHeader();
                boxContext.markOutputAsReadyToSend(e + 1, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            }
        }

        if (m_decoder.isBufferReceived()) {
            const CMatrix* matrix = m_decoder.getOutputMatrix();
            const double* buffer0 = matrix->getBuffer();

            const size_t nChannels0 = matrix->getDimensionSize(0);
            const size_t nSamples0  = matrix->getDimensionSize(1);

            //sig will store the samples of the different channels
            for (size_t c = 0; c < nChannels0; ++c)    //Number of EEG channels
            {
                for (size_t s = 0; s < nSamples0; ++s)    //Number of Samples per Chunk
                {
                    m_sig[c][s] = (buffer0[s + c * nSamples0]);
                }
            }

            // Due to how wavelet2s works, we'll have to have the output variables empty before each call.
            std::vector<std::vector<double>> flag;
            std::vector<std::vector<size_t>> length;
            std::vector<std::vector<double>> dwtOutput;
            flag.resize(nChannels0);
            length.resize(nChannels0);
            dwtOutput.resize(nChannels0);

            //Calculation of wavelets coefficients for each channel.
            for (size_t c = 0; c < nChannels0; ++c) { dwt(m_sig[c], j, nm, dwtOutput[c], flag[c], length[c]); }

            //Transmission of some information (flag and legth) to the inverse dwt box
            //@fixme since the data dimensions do not change runtime, it should be sufficient to send this only once
            for (size_t c = 0; c < nChannels0; ++c) {
                size_t f                                                      = 0;
                m_encoder.getInputMatrix()->getBuffer()[f + c * m_infolength] = double(length[c].size());
                for (size_t l = 0; l < length[c].size(); ++l) {
                    m_encoder.getInputMatrix()->getBuffer()[l + 1 + c * m_infolength] = double(length[c][l]);
                    f                                                                 = l;
                }
                m_encoder.getInputMatrix()->getBuffer()[f + 2 + c * m_infolength] = double(flag[c].size());
                for (size_t l = 0; l < flag[c].size(); ++l) { m_encoder.getInputMatrix()->getBuffer()[f + 3 + l + c * m_infolength] = flag[c][l]; }
            }

            //Decode the dwt coefficients of each decomposition level to separate channels
            for (size_t c = 0; c < nChannels0; ++c) {
                for (size_t e = 0, vectorPos = 0; e < m_encoders.size(); ++e) {
                    double* oBuffer = m_encoders[e]->getInputMatrix()->getBuffer();

                    // loop levels
                    for (size_t l = 0; l < size_t(length[c][e]); ++l) { oBuffer[l + c * length[c][e]] = dwtOutput[c][l + vectorPos]; }

                    vectorPos = vectorPos + length[c][e];
                }
            }

            m_encoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));

            for (size_t e = 0; e < m_encoders.size(); ++e) {
                m_encoders[e]->encodeBuffer();
                boxContext.markOutputAsReadyToSend(e + 1, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            }
        }

        if (m_decoder.isEndReceived()) {
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));

            for (size_t e = 0; e < m_encoders.size(); ++e) {
                m_encoders[e]->encodeEnd();
                boxContext.markOutputAsReadyToSend(e + 1, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            }
        }
    }

    return true;
}


}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif
