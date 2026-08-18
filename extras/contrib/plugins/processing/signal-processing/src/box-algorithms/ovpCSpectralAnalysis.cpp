#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCSpectralAnalysis.h"

#include <iostream>

#include <itpp/itstat.h>
#include <itpp/itsignal.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CSpectralAnalysis::initialize()
{
    //reads the plugin settings
    const CString setting     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const uint64_t components = this->getTypeManager().getBitMaskEntryCompositionValueFromName(OVP_TypeId_SpectralComponent, setting);

    m_amplitudeSpectrum = ((components & uint64_t(ESpectralComponent::Amplitude)) > 0);
    m_phaseSpectrum     = ((components & uint64_t(ESpectralComponent::Phase)) > 0);
    m_realPartSpectrum  = ((components & uint64_t(ESpectralComponent::RealPart)) > 0);
    m_imagPartSpectrum  = ((components & uint64_t(ESpectralComponent::ImaginaryPart)) > 0);

    m_decoder.initialize(*this, 0);
    for (size_t i = 0; i < 4; ++i) { m_encoders[i].initialize(*this, i); }

    return true;
}

bool CSpectralAnalysis::uninitialize()
{
    for (size_t i = 0; i < 4; ++i) { m_encoders[i].uninitialize(); }
    m_decoder.uninitialize();

    return true;
}

bool CSpectralAnalysis::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CSpectralAnalysis::process()
{
    Kernel::IBoxIO* context  = getBoxAlgorithmContext()->getDynamicBoxContext();
    const size_t nInputChunk = context->getInputChunkCount(0);
    char frequencyBandName[1024];

    for (size_t idx = 0; idx < nInputChunk; ++idx)
    {
        m_lastChunkStartTime = context->getInputChunkStartTime(0, idx);
        m_lastChunkEndTime   = context->getInputChunkEndTime(0, idx);

        m_decoder.decode(idx);

        if (m_decoder.isHeaderReceived())//dealing with the signal header
        {
            //get signal info
            m_nSample  = m_decoder.getOutputMatrix()->getDimensionSize(1);
            m_nChannel = m_decoder.getOutputMatrix()->getDimensionSize(0);
            m_sampling = size_t(m_decoder.getOutputSamplingRate());

            if (m_nSample == 0)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Chunk size appears to be 0, not supported.\n";
                return false;
            }
            if (m_nChannel == 0)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Channel count appears to be 0, not supported.\n";
                return false;
            }
            if (m_sampling == 0)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Sampling rate appears to be 0, not supported.\n";
                return false;
            }

            //we need two matrices for the spectrum encoders, the Frequency bands and the one inherited form streamed matrix (see doc for details)
            CMatrix* frequencyBands = new CMatrix();
            CMatrix* streamedMatrix = new CMatrix();

            // For real signals, if N is sample count, bins [0,N/2] (inclusive) contain non-redundant information, i.e. N/2+1 entries.
            m_halfFFTSize    = m_nSample / 2 + 1;
            m_nFrequencyBand = m_halfFFTSize;

            streamedMatrix->copyDescription(*m_decoder.getOutputMatrix());
            streamedMatrix->setDimensionSize(1, m_nFrequencyBand);
            frequencyBands->resize(m_nFrequencyBand);
            double* buffer = frequencyBands->getBuffer();

            // @fixme would be more proper to use 'bins', one bin with a hz tag per array entry
            for (size_t j = 0; j < m_nFrequencyBand; ++j)
            {
                buffer[j] = j * (double(m_sampling) / m_nSample);
                sprintf(frequencyBandName, "%lg", buffer[j]);
                streamedMatrix->setDimensionLabel(0, j, frequencyBandName);//set the names of the frequency bands
            }


            for (size_t j = 0; j < 4; ++j)
            {
                //copy the information for each encoder
                m_encoders[j].getInputFrequencyAbscissa()->copy(*frequencyBands);
                m_encoders[j].getInputMatrix()->copy(*streamedMatrix);
                m_encoders[j].getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
            }

            if (m_amplitudeSpectrum)
            {
                m_encoders[0].encodeHeader();
                context->markOutputAsReadyToSend(0, m_lastChunkStartTime, m_lastChunkEndTime);
            }
            if (m_phaseSpectrum)
            {
                m_encoders[1].encodeHeader();
                context->markOutputAsReadyToSend(1, m_lastChunkStartTime, m_lastChunkEndTime);
            }
            if (m_realPartSpectrum)
            {
                m_encoders[2].encodeHeader();
                context->markOutputAsReadyToSend(2, m_lastChunkStartTime, m_lastChunkEndTime);
            }
            if (m_imagPartSpectrum)
            {
                m_encoders[3].encodeHeader();
                context->markOutputAsReadyToSend(3, m_lastChunkStartTime, m_lastChunkEndTime);
            }

            delete frequencyBands;
            delete streamedMatrix;
        }
        if (m_decoder.isBufferReceived())
        {
            //get input buffer
            const double* buffer = m_decoder.getOutputMatrix()->getBuffer();
            //do the processing
            itpp::vec x(m_nSample);
            itpp::cvec y(m_nSample);
            itpp::cvec z(m_nChannel * m_halfFFTSize);

            for (size_t i = 0; i < m_nChannel; ++i)
            {
                for (size_t j = 0; j < m_nSample; ++j) { x[j] = double(*(buffer + i * m_nSample + j)); }

                y = fft_real(x);

                //test block
                // itpp::vec h = ifft_real(y);
                // std::cout << "Fx: " << x.size() << ", x=" << x << "\n" << "FF: " << y.size() << ", y=" << y << "\n" << "Fr: " << h.size() << ", x'=" << h << "\n";

                for (size_t k = 0; k < m_halfFFTSize; ++k) { z[k + i * m_halfFFTSize] = y[k]; }
            }

            if (m_amplitudeSpectrum)
            {
                CMatrix* matrix = m_encoders[0].getInputMatrix();
                double* buf     = matrix->getBuffer();
                for (size_t i = 0; i < m_nChannel * m_halfFFTSize; ++i) { *(buf + i) = sqrt(real(z[i]) * real(z[i]) + imag(z[i]) * imag(z[i])); }
                m_encoders[0].encodeBuffer();
                context->markOutputAsReadyToSend(0, m_lastChunkStartTime, m_lastChunkEndTime);
            }
            if (m_phaseSpectrum)
            {
                CMatrix* matrix = m_encoders[1].getInputMatrix();
                double* buf     = matrix->getBuffer();
                for (size_t i = 0; i < m_nChannel * m_halfFFTSize; ++i) { *(buf + i) = imag(z[i]) / real(z[i]); }
                m_encoders[1].encodeBuffer();
                context->markOutputAsReadyToSend(1, m_lastChunkStartTime, m_lastChunkEndTime);
            }
            if (m_realPartSpectrum)
            {
                CMatrix* matrix = m_encoders[2].getInputMatrix();
                double* buf     = matrix->getBuffer();
                for (size_t i = 0; i < m_nChannel * m_halfFFTSize; ++i) { *(buf + i) = real(z[i]); }
                m_encoders[2].encodeBuffer();
                context->markOutputAsReadyToSend(2, m_lastChunkStartTime, m_lastChunkEndTime);
            }
            if (m_imagPartSpectrum)
            {
                CMatrix* matrix = m_encoders[3].getInputMatrix();
                double* buf     = matrix->getBuffer();
                for (size_t i = 0; i < m_nChannel * m_halfFFTSize; ++i) { *(buf + i) = imag(z[i]); }
                m_encoders[3].encodeBuffer();
                context->markOutputAsReadyToSend(3, m_lastChunkStartTime, m_lastChunkEndTime);
            }
        }
        context->markInputAsDeprecated(0, idx);
    }
    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif
