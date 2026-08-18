#if defined(TARGET_HAS_ThirdPartyFFTW3) // required by wavelet2s

#include "CBoxAlgorithmInverse_DWT.hpp"
#include "../../../contrib/packages/wavelet2d/wavelet2s.h"

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmInverse_DWT::initialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();
    m_algoInfoDecoder.initialize(*this, 0);

    m_waveletType        = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)).toASCIIString();
    m_decompositionLevel = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)).toASCIIString();

    m_algoXDecoder = new Toolkit::TSignalDecoder<CBoxAlgorithmInverse_DWT> [nInput];

    for (size_t o = 0; o < nInput - 1; ++o) { m_algoXDecoder[o].initialize(*this, o + 1); }

    m_encoder.initialize(*this, 0);
    return true;
}

bool CBoxAlgorithmInverse_DWT::uninitialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    m_algoInfoDecoder.uninitialize();

    for (size_t o = 0; o < nInput - 1; ++o) { m_algoXDecoder[o].uninitialize(); }
    delete[] m_algoXDecoder;

    m_encoder.uninitialize();

    return true;
}


bool CBoxAlgorithmInverse_DWT::processInput(const size_t /*index*/)
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    //Check if all inputs have some information to decode

    bool readyToProcess = true;
    for (size_t i = 0; i < this->getStaticBoxContext().getInputCount(); ++i) {
        readyToProcess &= (boxContext.getInputChunkCount(i) == 1);
    }

    if (readyToProcess) {
        getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    }
    return true;
}

bool CBoxAlgorithmInverse_DWT::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //    size_t J = std::atoi(m_decompositionLevel);
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    std::vector<size_t> nChannels(nInput);
    std::vector<size_t> nSamples(nInput);

    std::vector<std::vector<double>> flag;
    std::vector<std::vector<size_t>> length;
    //Decode the first input (Informations)
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_algoInfoDecoder.decode(i);

        if(m_algoInfoDecoder.isBufferReceived()) {

            nChannels[0] = m_algoInfoDecoder.getOutputMatrix()->getDimensionSize(0);
            nSamples[0]  = m_algoInfoDecoder.getOutputMatrix()->getDimensionSize(1);

            const double* buffer = m_algoInfoDecoder.getOutputMatrix()->getBuffer();

            length.resize(nChannels[0]);
            flag.resize(nChannels[0]);

            for (size_t j = 0; j < nChannels[0]; ++j) {
                size_t f = 0;
                for (size_t l = 0; l < size_t(buffer[0]); ++l) {
                    length[j].push_back(size_t(buffer[l + 1]));
                    f = l;
                }
                for (size_t l = 0; l < size_t(buffer[f + 2]); ++l) { flag[j].push_back(std::floor(buffer[f + 3 + l])); }
            }
        }
    }

    //If Informations is decoded
    std::vector<std::vector<double>> dwtop;
    std::vector<std::vector<double>> idwtOutput;
    //Decode each decomposition level
    for (size_t o = 0; o < nInput - 1; ++o) {
        //Decode data of channels
        for (size_t i = 0; i < boxContext.getInputChunkCount(o + 1); ++i) {
            m_algoXDecoder[o].decode(i);
            if (m_algoXDecoder[o].isBufferReceived()) {
                nChannels[o + 1] = m_algoXDecoder[o].getOutputMatrix()->getDimensionSize(0);
                nSamples[o + 1] = m_algoXDecoder[o].getOutputMatrix()->getDimensionSize(1);
                const double *buffer = m_algoXDecoder[o].getOutputMatrix()->getBuffer();

                //dwtop is the dwt coefficients
                dwtop.resize(nChannels[0]);

                //Store input data (dwt coefficients) in just one vector (dwtop)
                for (size_t j = 0; j < nChannels[0]; ++j) {
                    for (size_t k = 0; k < nSamples[o + 1]; ++k) {
                        dwtop[j].push_back(buffer[k + j * nSamples[o + 1]]);
                    }
                }

                //Check if received informations about dwt box are coherent with inverse dwt box settings
                if (!length[0].empty() && o == nInput - 2) {
                    //Check if quantity of samples received are the same
                    if (length[0].at(nInput - 1) == dwtop[0].size()) {
                        //Resize idwt vector
                        idwtOutput.resize(nChannels[0]);

                        //Calculate idwt for each channel
                        for (size_t j = 0; j < nChannels[0]; ++j) {
                            idwt(dwtop[j], flag[j], m_waveletType, idwtOutput[j], length[j]);
                        }

                        //Only first time :
                        if (!m_hasHeader) {
                            m_encoder.getInputSamplingRate() = 2 * m_algoXDecoder[o].getOutputSamplingRate();
                            m_encoder.getInputMatrix()->resize(nChannels[0], length[0].at(nInput - 1));

                            for (size_t j = 0; j < nChannels[0]; j++) {
                                m_encoder.getInputMatrix()->setDimensionLabel(0, j, m_algoXDecoder[o].getOutputMatrix()->getDimensionLabel(0, j));
                            }

                            m_encoder.encodeHeader();
                            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(o, i), boxContext.getInputChunkEndTime(o, i));
                            m_hasHeader = true;
                        }

                        //Encode resultant signal to output
                        for (size_t j = 0; j < nChannels[0]; ++j) {
                            for (size_t k = 0; k < size_t(idwtOutput[j].size()); ++k) {
                                m_encoder.getInputMatrix()->getBuffer()[k + j * size_t(idwtOutput[j].size())] = idwtOutput[j][k];
                            }
                        }

                        m_encoder.encodeBuffer();
                        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(o, i), boxContext.getInputChunkEndTime(o, i));
                    }
                }
            }
        }
    }
    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViB
#endif
