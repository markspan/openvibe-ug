#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCWindowingFunctions.h"

#include <iostream>

#include <itpp/itcomm.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void CWindowingFunctions::setSampleBuffer(const double* buffer) const
{
    itpp::vec windows = itpp::ones(int(m_SamplesPerBuffer));

    if (m_WindowMethod == EWindowMethod::Hamming) { windows = itpp::hamming(int(m_SamplesPerBuffer)); }
    else if (m_WindowMethod == EWindowMethod::Hanning) { windows = itpp::hanning(int(m_SamplesPerBuffer)); }
    else if (m_WindowMethod == EWindowMethod::Hann) { windows = itpp::hann(int(m_SamplesPerBuffer)); }
    else if (m_WindowMethod == EWindowMethod::Blackman) { windows = itpp::blackman(int(m_SamplesPerBuffer)); }
    else if (m_WindowMethod == EWindowMethod::Triangular) { windows = itpp::triang(int(m_SamplesPerBuffer)); }
    else if (m_WindowMethod == EWindowMethod::SquareRoot) { windows = itpp::sqrt_win(int(m_SamplesPerBuffer)); }

    for (size_t i = 0; i < m_NChannel; ++i)
    {
        for (size_t j = 0; j < m_SamplesPerBuffer; ++j) { m_Buffer[i * m_SamplesPerBuffer + j] = double(buffer[i * m_SamplesPerBuffer + j]) * windows(int(j)); }
    }
}

bool CWindowingFunctions::initialize()
{
    //reads the plugin settings
    const CString method = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_WindowMethod       = EWindowMethod(this->getTypeManager().getEnumerationEntryValueFromName(OVP_TypeId_WindowMethod, method));

    m_Decoder = new Toolkit::TSignalDecoder<CWindowingFunctions>(*this, 0);
    m_Encoder = new Toolkit::TSignalEncoder<CWindowingFunctions>(*this, 0);

    return true;
}

bool CWindowingFunctions::uninitialize()
{
    m_Decoder->uninitialize();
    delete m_Decoder;
    m_Encoder->uninitialize();
    delete m_Encoder;

    return true;
}

bool CWindowingFunctions::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CWindowingFunctions::process()
{
    IDynamicBoxContext* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    // Process input data
    for (size_t i = 0; i < boxContext->getInputChunkCount(0); ++i)
    {
        size_t chunkSize;
        const uint8_t* buffer;
        boxContext->getInputChunk(0, i, m_LastChunkStartTime, m_LastChunkEndTime, chunkSize, buffer);


        m_Decoder->decode(i);
        if (m_Decoder->isHeaderReceived())
        {
            CMatrix* iMatrix = m_Decoder->getOutputMatrix();
            CMatrix* oMatrix = m_Encoder->getInputMatrix();

            oMatrix->copy(*iMatrix);

            m_Buffer           = oMatrix->getBuffer();
            m_SamplesPerBuffer = oMatrix->getDimensionSize(1);
            m_NChannel         = oMatrix->getDimensionSize(0);

            const size_t sampling             = m_Decoder->getOutputSamplingRate();
            m_Encoder->getInputSamplingRate() = sampling;

            m_Encoder->encodeHeader();
            boxContext->markOutputAsReadyToSend(i, m_LastChunkStartTime, m_LastChunkEndTime);
        }
        if (m_Decoder->isBufferReceived())
        {
            CMatrix* iMatrix = m_Decoder->getOutputMatrix();
            m_Buffer         = m_Encoder->getInputMatrix()->getBuffer();
            setSampleBuffer(iMatrix->getBuffer());

            m_Encoder->encodeBuffer();
            boxContext->markOutputAsReadyToSend(i, m_LastChunkStartTime, m_LastChunkEndTime);
        }
        if (m_Decoder->isEndReceived())
        {
            m_Encoder->encodeEnd();
            boxContext->markOutputAsReadyToSend(i, m_LastChunkStartTime, m_LastChunkEndTime);
        }


        boxContext->markInputAsDeprecated(0, i);
        //m_pReader->processData(buffer, chunkSize);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyITPP
